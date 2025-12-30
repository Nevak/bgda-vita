#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils/macros.h"
#include "utils/logger.h"
// #ifdef PROFILE_TEX_DECOMP
// #include "libperf.h"
// #endif

//#define PROFILE_TEX_DECOMP

extern void* malloc(size_t size);
extern void free(void* ptr);
extern void* calloc(size_t num, size_t size);
extern void* memcpy(void* dst, const void* src, size_t n);
extern void* memset(void* dst, int val, size_t n);
extern int sprintf(char* str, const char* format, ...);
extern void glFinish(void);  // OpenGL sync - wait for GPU to complete all operations

#ifdef PROFILE_TEX_DECOMP
extern uint64_t sceKernelGetProcessTimeWide(void);
#endif

// Direct function addresses for worldAllocateSegments
extern uintptr_t lockLoadingMutex_addr;
extern uintptr_t releaseLoadingMutex_addr;
extern uintptr_t lumpLoad_addr;
extern uintptr_t machHostOpen_addr;
extern uintptr_t machHostRead_addr;
extern uintptr_t machHostSeek_addr;
extern uintptr_t machHostClose_addr;
extern uintptr_t lowestPowerof2NotLessThan_addr;
extern uintptr_t D3DDevice_CreatePalette2_addr;
extern uintptr_t D3DPalette_Lock2_addr;
extern uintptr_t D3DDevice_CreateTexture2_addr;
extern uintptr_t D3DTexture_LockRect_addr;
extern uintptr_t D3DTexture_UnlockRect_addr;

static inline uint8_t UnsignedSaturate8(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (uint8_t)value;
}

typedef struct _worldHeader _worldHeader;
struct _worldHeader {
    int element_count;           // at 0x00 - Element count
    uint8_t padding1[0x20];   // 0x04 to 0x24 (32 bytes)
    uint32_t elemen_array_start;    // at 0x24 - Element array start
    uint8_t padding2[0x30];   // 0x28 to 0x58 (48 bytes)
    int tex_start;         // at 0x58 - Texture start
    int tex_end;         // at 0x5c - Texture end
    uint8_t padding3[0x04];   // 0x60 to 0x64 (4 bytes)
    uint32_t chunks;    // at 0x64 - Chunk info array
    uint8_t padding4[0x0c];   // 0x68 to 0x74 (12 bytes)
    int isAllocated;        // at 0x74 - Already allocated flag
};

// Palette entry (RGBA)
typedef struct {
    uint8_t r, g, b, a;
} PalEntry;

// Huffman value cache for fast 8-bit lookups
typedef struct {
    int16_t val;      // Decoded value
    int16_t numBits;  // Number of bits used (0 = needs longer decode)
} HuffVal;

// Texture entry in YAK file chunk
typedef struct {
    uint16_t width;
    uint16_t height;
    int32_t compressed_data_offset;
    int32_t compressed_data_length;
    // ... other fields as needed
} TexEntry;

// Back-reference table for pixel commands 0x100-0x104
static const int BACK_JUMP_TABLE[] = {-1, -16, -17, -15, -2};

// Global Huffman table offsets (like original's DAT_00132508, etc.)
// These are set once per texture and reused for all blocks
static int g_table0_start;
static int g_table1_start;
static int g_table2_start;
static int g_table3_start;

// World texture entry structure (0x38 bytes per entry)
typedef struct {
    uint16_t width;           // +0x00
    uint16_t height;          // +0x02
    uint32_t field_0x04;      // +0x04
    uint32_t data_offset;     // +0x08: Offset to compressed data
    uint32_t field_0x0c;      // +0x0C
    uint32_t texture_ptr;     // +0x10: GPU texture (SWAPPED!)
    uint32_t palette_ptr;     // +0x14: GPU palette (SWAPPED!)
    uint32_t last_used_frame; // +0x18: Frame timestamp for texture aging
    uint8_t  padding[0x1C];   // +0x1C to +0x37 (remaining 28 bytes)
} WorldTexEntry;

// Texture chunk info (stored in worldHeader->field88_0x64)
typedef struct {
    uint32_t tex_data_offset; // Offset in .tex file (or 0 if unused)
    uint32_t flags;           // Non-zero if chunk is valid
} TexChunkInfo;

int alignDimension(int dimension) {
    int alignment = 32;
    int aligned = (dimension + (alignment - 1)) & ~(alignment - 1);
    if (aligned < alignment) 
		aligned = alignment;
    return aligned;
}

/**
 * Main function to allocate and decode all world textures
 * This is essentially the decompiled worldAllocateSegments function from ghidra with more readable names and comments added by claude AI
 * It has been patched in a few ways mainly to reduce memory usage on Vita:
 *  - It doesn't read the entire .tex file into memory at once like the android port does.
 *    Reading the entire .tex file causes OOM crashes on Vita due to limited memory.
 *    Instead we read and decode one chunk at a time directly from the file stream. This doesn't seem to impact performance significantly.
 *  - It uses a fixed-size temporary buffer for decoding texture blocks instead of allocating per-chunk
 *  - Optionally can downscale large textures to reduce GPU memory usage ( see #define MAX_TEXTURE_DIM )
 */
void worldAllocateSegments(_worldHeader *worldHeader) {

	logv_debug("[0x%X] === worldAllocateSegments START ===", sceKernelGetThreadId());

#ifdef PROFILE_TEX_DECOMP
    uint64_t time_file_io = 0;
    uint64_t time_huff_table = 0;
    uint64_t time_decode_blocks = 0;
    uint64_t time_huff_decode = 0;
    uint64_t time_block_copy = 0;
    uint64_t time_gpu_upload = 0;
    uint64_t time_start, time_end;
    int total_blocks_decoded = 0;
    int total_textures = 0;
#endif

    if (worldHeader->isAllocated != 0) {
        //log_error("worldAllocateSegments: already allocated, skipping");
        return;
    }

    worldHeader->isAllocated = 1;

    // Get world name (needed for both cache and normal path)
    char *world_name = (char*)LOC(0x0054cad8);
    logv_debug("worldAllocateSegments: world_name pointer=0x%08X", (uint32_t)world_name);

    // Load world lumps if needed (ALWAYS run this, even when using cache)
    int element_count = worldHeader->element_count;
    //logv_error("worldAllocateSegments: element_count=%d, elemen_array_start=0x%08X",
             //  element_count, worldHeader->elemen_array_start);

    if (element_count > 0) {
        for (int i = 0; i < element_count; i++) {
            uintptr_t element_addr = worldHeader->elemen_array_start + i * 0x38;
            uint32_t *element = (uint32_t*)element_addr;
            uint16_t flags = *(uint16_t*)((char*)element + 0x30);

            //logv_error("  Element[%d]: addr=0x%08X, element[0]=0x%08X, flags=0x%04X",
                      //i, element_addr, element[0], flags);

            if (flags & 0x800) {
                char lump_name[80];
                char *name = (char*)element[0];  // Single dereference - element[0] IS the pointer
                //logv_error("  Loading lump with name pointer=0x%08X", (uint32_t)name);
                sprintf(lump_name, "%s.lmp", name);
                ((int (*)(char*))lumpLoad_addr)(lump_name);
            }
        }
    }

    // Get texture range and chunk array
    int tex_start = worldHeader->tex_start;  // 4949
    int tex_end = worldHeader->tex_end;    // 5050
    TexChunkInfo *chunks = (TexChunkInfo*)worldHeader->chunks;

#ifdef PROFILE_TEX_DECOMP
    time_start = sceKernelGetProcessTimeWide();
#endif
    char tex_path[80];
    sprintf(tex_path, "res\\%s.tex", world_name);

    // Open .tex file for streaming (read chunks one at a time)
    // Open file and get size
    int file = ((int (*)(char*, char*))machHostOpen_addr)(tex_path, "rb");
    int file_size = ((int (*)(int, int, int))machHostSeek_addr)(file, 0, 2);

#ifdef PROFILE_TEX_DECOMP
    time_end = sceKernelGetProcessTimeWide();
    time_file_io += (time_end - time_start);
#endif

    if (tex_end < tex_start) {
        ((int (*)(int))machHostClose_addr)(file);
        return;
    }

    // Save original chunk file offsets AND pre-calculate sizes
    int num_chunks = tex_end - tex_start + 1;
    uint32_t *chunk_offsets = (uint32_t*)malloc(num_chunks * sizeof(uint32_t));
    uint32_t *chunk_sizes = (uint32_t*)malloc(num_chunks * sizeof(uint32_t));

    if (!chunk_offsets || !chunk_sizes) {
        logv_error("FATAL: Failed to allocate chunk tracking arrays (%d chunks)", num_chunks);
        if (chunk_offsets) free(chunk_offsets);
        if (chunk_sizes) free(chunk_sizes);
        ((int (*)(int))machHostClose_addr)(file);
        return;
    }

    for (int i = 0; i < num_chunks; i++) {
        chunk_offsets[i] = chunks[i].tex_data_offset;
    }

    // Pre-calculate all chunk sizes (do this once, not per-chunk!)
    uint32_t max_chunk_size = 0;
    for (int i = 0; i < num_chunks; i++) {
        uint32_t chunk_offset = chunk_offsets[i];
        if (chunk_offset == 0) {
            chunk_sizes[i] = 0;
            continue;
        }

        // Find next chunk boundary
        uint32_t next_offset = file_size;
        for (int j = 0; j < num_chunks; j++) {
            uint32_t scan_offset = chunk_offsets[j];
            if (scan_offset > chunk_offset && scan_offset < next_offset) {
                next_offset = scan_offset;
            }
        }

        uint32_t chunk_size = next_offset - chunk_offset;
        chunk_sizes[i] = chunk_size;
        if (chunk_size > max_chunk_size) {
            max_chunk_size = chunk_size;
        }
    }

    // Check if there are any chunks to process
    if (max_chunk_size == 0) {
        log_error("No chunks to process (max_chunk_size = 0)");
        free(chunk_offsets);
        free(chunk_sizes);
        ((int (*)(int))machHostClose_addr)(file);
        return;
    }

    //logv_error("Max chunk size: %u bytes (%.2f MB)", max_chunk_size, max_chunk_size / (1024.0 * 1024.0));
    void *tex_data = malloc(max_chunk_size);
    if (!tex_data) {
        logv_error("FATAL: Failed to allocate %u bytes for streaming buffer", max_chunk_size);
        free(chunk_offsets);
        free(chunk_sizes);
        ((int (*)(int))machHostClose_addr)(file);
        return;
    }

    // Process each texture chunk
    // Allocate fixed-size temp buffer once (max texture size: 1024x1024)
    int max_tex_size = 1024 * 1024;  // 65536 pixels
    uint8_t *temp_buffer = (uint8_t*)malloc(max_tex_size + 0x19000);
    if (!temp_buffer) {
        log_error("FATAL: Failed to allocate temp_buffer");
        free(chunk_offsets);
        free(chunk_sizes);
        free(tex_data);
        ((int (*)(int))machHostClose_addr)(file);
        return;
    }

    // Track maximum texture dimensions encountered
    int max_width_seen = 0;
    int max_height_seen = 0;
    uint64_t total_gpu_memory = 0;  // Track total GPU memory allocated

    int16_t huff_table[512];  // Reusable Huffman table buffer - flat array like original! [val, numBits, val, numBits, ...]
    uint8_t block_buffer[256];  // Reusable decode buffer (like original's rectPtr/decompressBlock)


    for (int chunk_idx = 0; chunk_idx <= (tex_end - tex_start); chunk_idx++) {
        TexChunkInfo *chunk = &chunks[chunk_idx];

        // Use saved offset, not chunk->tex_data_offset (which may be overwritten!)
        uint32_t chunk_file_offset = chunk_offsets[chunk_idx];

        if (chunk->flags == 0 || chunk_file_offset == 0) {
            continue;
        }

        // Use pre-calculated chunk size
        uint32_t chunk_size = chunk_sizes[chunk_idx];

        // Safety check: never read more than allocated buffer
        if (chunk_size > max_chunk_size) {
            logv_error("FATAL: Chunk %d size (%u) exceeds max (%u)!", chunk_idx, chunk_size, max_chunk_size);
            continue;
        }

        // Read chunk data into fixed-size streaming buffer
#ifdef PROFILE_TEX_DECOMP
        time_start = sceKernelGetProcessTimeWide();
#endif
        //logv_error("Chunk %d: offset=0x%x, size=%u bytes", chunk_idx, chunk_file_offset, chunk_size);

        ((int (*)(int, int, int))machHostSeek_addr)(file, chunk_file_offset, 0);
        ((void (*)(int, void*, int))machHostRead_addr)(file, tex_data, chunk_size);
#ifdef PROFILE_TEX_DECOMP
        time_end = sceKernelGetProcessTimeWide();
        time_file_io += (time_end - time_start);
#endif

        // Process chunk (adjust all offsets to be relative to buffer start)
        uint8_t *chunk_data = (uint8_t*)tex_data;
        int num_textures = *(int32_t*)chunk_data;
        //logv_error("  num_textures = %d", num_textures);

        // Skip if no textures in this chunk
        if (num_textures <= 0) {
            chunk->tex_data_offset = 0;
            // DON'T modify chunk_offsets - we need original values for size calculations!
            //log_error("  Skipping chunk (no textures)");
            continue;
        }

        // Sanity check
        if (num_textures < 0 || num_textures > 1000) {
            logv_error("  FATAL: Invalid num_textures (%d) - chunk data corrupted?", num_textures);
            chunk->tex_data_offset = 0;
            continue;
        }

        // Allocate texture entry array
        // Layout: count at +0x00 (4 bytes), Entry0 at +0x04, Entry1 at +0x3C, ...
        uint8_t *entries_mem = (uint8_t*)malloc(num_textures * 0x38 + 4);
        if (!entries_mem) {
            logv_error("FATAL: Failed to allocate entries_mem for chunk %d (%d textures, %u bytes)",
                      chunk_idx, num_textures, num_textures * 0x38 + 4);
            chunk->tex_data_offset = 0;
            continue;  // Skip this chunk
        }
        chunk->tex_data_offset = (uint32_t)entries_mem;  // Store pointer

        // Store count at offset 0
        *(int32_t*)entries_mem = num_textures;

        // Copy texture entries starting at offset +4
        memcpy(entries_mem + 4, chunk_data + 0x40, num_textures * 0x38);

        // Decode each texture
        for (int tex_idx = 0; tex_idx < num_textures; tex_idx++) {
            // Note: entries are accessed at multiples of 0x38 from base (not +4!)
            // The count at offset 0 overlaps with first entry's width/height
            WorldTexEntry *entry = (WorldTexEntry*)(entries_mem + tex_idx * 0x38);

            // IMPORTANT: For BGDA, offsets in entry are relative to entry position in file!
            // Entry is at: chunk_file_offset + 0x40 + tex_idx * 0x38
            uint32_t entry_file_offset = chunk_file_offset + 0x40 + tex_idx * 0x38;

            // Read width/height from SOURCE file data (not allocated buffer, which has count at offset 0 for entry 0)
            WorldTexEntry *file_entry = (WorldTexEntry*)(chunk_data + 0x40 + tex_idx * 0x38);

            // IMPORTANT: Read ALL offsets from file_entry, not entry! Entry 0 has count overlapping fields.
            // Compressed data offset is relative to entry position (convert to chunk-relative)
            int data_offset = file_entry->data_offset + entry_file_offset;
            uint8_t *comp_data = (uint8_t*)tex_data + (data_offset - chunk_file_offset);

            // Palette offset (stored in comp_data) is also relative to entry position (convert to chunk-relative)
            int pal_offset = *(int32_t*)comp_data + entry_file_offset;
            uint8_t *pal_data = (uint8_t*)tex_data + (pal_offset - chunk_file_offset);

            // Build Huffman lookup table INLINE (like original - no function call!)
#ifdef PROFILE_TEX_DECOMP
            time_start = sceKernelGetProcessTimeWide();
#endif
            int huff_table_offset = pal_offset + 0xc00;
            int ht_table1_len_val = *(int32_t*)((uint8_t*)tex_data + (huff_table_offset - chunk_file_offset));
            int ht_table1_len = ht_table1_len_val * 2;
            int ht_table1_start = huff_table_offset + 4;
            int ht_table2_start = ht_table1_start + ht_table1_len;
            int ht_table3_start = ht_table2_start + 0x48;

            for (int i = 0; i < 256; i++) {
                int bit = 1;
                uint32_t a = (uint32_t)i >> (8 - bit);
                int v = *(int32_t*)((uint8_t*)tex_data + (ht_table3_start + (bit * 4) - chunk_file_offset));

                while (v < (int)a && bit <= 8) {
                    bit++;
                    a = (uint32_t)i >> (8 - bit);
                    v = *(int32_t*)((uint8_t*)tex_data + (ht_table3_start + (bit * 4) - chunk_file_offset));
                }

                if (bit <= 8) {
                    int val = *(int32_t*)((uint8_t*)tex_data + (ht_table2_start + (bit * 4) - chunk_file_offset));
                    int table1_index = (int)a + val;
                    huff_table[i * 2] = *(int16_t*)((uint8_t*)tex_data + (ht_table1_start + (table1_index * 2) - chunk_file_offset));
                    huff_table[i * 2 + 1] = (int16_t)bit;
                } else {
                    huff_table[i * 2] = 0;
                    huff_table[i * 2 + 1] = 0;
                }
            }
#ifdef PROFILE_TEX_DECOMP
            time_end = sceKernelGetProcessTimeWide();
            time_huff_table += (time_end - time_start);
#endif

            // Pre-calculate Huffman table offsets ONCE per texture - store in globals like original
            g_table0_start = pal_offset + 0x400;
            int table_offset = g_table0_start + 0x800;
            int table1_len_val = *(int32_t*)((uint8_t*)tex_data + (table_offset - chunk_file_offset));
            int table1_len = table1_len_val * 2;
            g_table1_start = table_offset + 4;
            g_table2_start = g_table1_start + table1_len;
            g_table3_start = g_table2_start + 0x48;

            // Use actual texture dimensions (NPOT textures supported by vitaGL)
            // memcpy-based block copy handles any width without alignment issues
            // Use file_entry for width/height since entry 0 has count overlapping these fields
            int width = alignDimension(file_entry->width);
            int height = alignDimension(file_entry->height);
            

            // Track maximum dimensions
            if (width > max_width_seen) max_width_seen = width;
            if (height > max_height_seen) max_height_seen = height;

            int total_pixels = width * height;

            // Reuse temp_buffer - just clear it
            memset(temp_buffer, 0, total_pixels);

            // Decode texture blocks - INLINED like original (no function calls!)
#ifdef PROFILE_TEX_DECOMP
            time_start = sceKernelGetProcessTimeWide();
            int blocks_decoded = 0;
#endif
            uint8_t *block_list = (uint8_t*)comp_data + 4;
            uint8_t *file_data = (uint8_t*)tex_data;

            while (*block_list != 0xFF) {
                int x0 = block_list[0];
                int y0 = block_list[1];
                int x1 = block_list[2];
                int y1 = block_list[3];
                block_list += 4;

                // Process blocks only if valid range (like Ghidra does)
                if (x0 <= x1) {
                    // Y loop uses max(y1, y0) as end (like Ghidra)
                    int y_end = (y1 >= y0) ? y1 : y0;
                    int y_block = y0;
                    do {
                        int x_block = x0;
                        do {
                            // Block offsets are also relative to entry position
                            int block_data_start = *(int32_t*)block_list + entry_file_offset;
                            block_list += 4;

                            // === INLINED BLOCK DECODE (like original) ===
                            int cur_pix8 = 0;
                            int start_bit = 0;
                            int prev_pixel = 0;

                            // Decode 256 pixels sequentially
#ifdef PROFILE_TEX_DECOMP
                            uint64_t time_huff_start = sceKernelGetProcessTimeWide();
#endif
                            // Pre-calculate ALL base pointers outside loop (once per block!)
                            // Use restrict to help compiler aliasing analysis (adjust to chunk-relative offsets)
                            uint16_t *__restrict block_data_ptr = (uint16_t*)(file_data + (block_data_start - chunk_file_offset));
                            int32_t *__restrict table3_ptr = (int32_t*)(file_data + (g_table3_start - chunk_file_offset));
                            int32_t *__restrict table2_ptr = (int32_t*)(file_data + (g_table2_start - chunk_file_offset));
                            int16_t *__restrict table1_ptr = (int16_t*)(file_data + (g_table1_start - chunk_file_offset));

                            int i = 0;
                            do {
                                // Read 16-bit value from bitstream (minimize temporaries)
                                int word_offset = start_bit >> 4;
                                uint32_t word = ((((uint32_t)block_data_ptr[word_offset] << 16) |
                                                  block_data_ptr[word_offset + 1]) >>
                                                 (16 - (start_bit & 0x0f))) & 0xFFFF;

                                // Fast 8-bit Huffman decode (cache huff_idx calculation)
                                int huff_idx = (word >> 8) << 1;
                                int pix_cmd = huff_table[huff_idx];
                                int bits = huff_table[huff_idx + 1];

                                if (bits == 0) {
                                    // Slow path - full decode (9-16 bits)
                                    int bit = 9;
                                    uint32_t a = word >> 7;

                                    while (table3_ptr[bit] < (int)a && bit <= 16) {
                                        bit++;
                                        a = word >> (16 - bit);
                                    }

                                    pix_cmd = table1_ptr[table2_ptr[bit] + (int)a];
                                    bits = bit;
                                }

                                start_bit += bits;

                                // Decode pixel command (no bounds checking, minimize temporaries)
                                if (pix_cmd <= 0xff) {
                                    block_buffer[cur_pix8] = pix_cmd;
                                    prev_pixel = pix_cmd;
                                } else if (pix_cmd <= 0x104) {
                                    prev_pixel = block_buffer[cur_pix8 + BACK_JUMP_TABLE[pix_cmd - 0x100]];
                                    block_buffer[cur_pix8] = prev_pixel;
                                } else {
                                    prev_pixel = file_data[(g_table0_start - chunk_file_offset) + (pix_cmd - 0x105) + (prev_pixel << 3)];
                                    block_buffer[cur_pix8] = prev_pixel;
                                }

                                cur_pix8++;
                                i++;
                            } while (i != 0x100);
#ifdef PROFILE_TEX_DECOMP
                            uint64_t time_huff_end = sceKernelGetProcessTimeWide();
                            time_huff_decode += (time_huff_end - time_huff_start);
#endif

                            // Bulk copy 16x16 block to output buffer
                            // Use memcpy to handle any width (POT or NPOT) - handles unaligned access
#ifdef PROFILE_TEX_DECOMP
                            uint64_t time_copy_start = sceKernelGetProcessTimeWide();
#endif
                            int block_x = x_block * 16;
                            int block_y = y_block * 16;
                            uint8_t *src = block_buffer;
                            uint8_t *dst = &temp_buffer[block_y * width + block_x];

                            // Copy 16 bytes per row, 16 rows - memcpy handles unaligned widths
                            for (int row = 0; row < 16; row++) {
                                memcpy(dst, src, 16);
                                dst += width;
                                src += 16;
                            }
#ifdef PROFILE_TEX_DECOMP
                            uint64_t time_copy_end = sceKernelGetProcessTimeWide();
                            time_block_copy += (time_copy_end - time_copy_start);
                            blocks_decoded++;
#endif
                            x_block++;
                        } while (x_block <= x1);
                        y_block++;
                    } while (y_block <= y_end);
                }  // end if (x0 <= x1)
            }
#ifdef PROFILE_TEX_DECOMP
            time_end = sceKernelGetProcessTimeWide();
            time_decode_blocks += (time_end - time_start);
            total_blocks_decoded += blocks_decoded;
            total_textures++;
#endif
                
            if (height < 32 || width < 32)
            {
                logv_error(" Loading SMALL tex %dx%d", width, height);
            }
            // Downsample textures larger than the maximum to save GPU memory (preserving aspect ratio)
            #define MAX_TEXTURE_DIM 256
            if (width > MAX_TEXTURE_DIM || height > MAX_TEXTURE_DIM) {
                // Calculate uniform scale factor based on larger dimension
               // int max_dim = (width > height) ? width : height;
               // float scale = (float)max_dim / MAX_TEXTURE_DIM;
                float scale = 2.0f;

                // Calculate new dimensions preserving aspect ratio
                int new_width = (int)(width / scale + 0.5f);   // Round to nearest
                int new_height = (int)(height / scale + 0.5f);
                int new_total_pixels = new_width * new_height;

                if (new_height < 64 || new_width < 64) {
                    //  logv_error("SKIPPING    Downsampling %dx%d -> %dx%d (scale %.2fx, aspect %.3f -> %.3f)",
                    //        width, height, new_width, new_height, scale,
                    //        (float)width/(float)height, (float)new_width/(float)new_height);                    

                    goto skip_downsample;
                }

                logv_error("    Downsampling %dx%d -> %dx%d (scale %.2fx, aspect %.3f -> %.3f)",
                           width, height, new_width, new_height, scale,
                           (float)width/(float)height, (float)new_width/(float)new_height);

                // Allocate temporary buffer for downsampled texture
                uint8_t *downsampled = (uint8_t*)malloc(new_total_pixels);
                if (!downsampled) {
                    logv_error("    ERROR: Failed to allocate %d bytes for downsampling!", new_total_pixels);
                    goto skip_downsample;
                }

                // Downsample using fixed-point coordinate mapping (avoids division in inner loop)
                // Use 16.16 fixed-point format for sub-pixel precision
                int x_step = (width << 16) / new_width;   // Fixed-point step per destination pixel
                int y_step = (height << 16) / new_height;

                for (int y = 0; y < new_height; y++) {
                    int src_y = (y * y_step) >> 16;  // Calculate once per row
                    // Clamp to valid range to prevent buffer overruns
                    if (src_y >= height) src_y = height - 1;
                    int src_row_offset = src_y * width;

                    for (int x = 0; x < new_width; x++) {
                        int src_x = (x * x_step) >> 16;
                        // Clamp to valid range
                        if (src_x >= width) src_x = width - 1;
                        downsampled[y * new_width + x] = temp_buffer[src_row_offset + src_x];
                    }
                }

                // Copy downsampled data back to temp_buffer
                memcpy(temp_buffer, downsampled, new_total_pixels);
                free(downsampled);

                // Update dimensions for GPU upload
                width = new_width;
                height = new_height;
                total_pixels = new_total_pixels;

                logv_error("    Downsampling complete: final dimensions %dx%d", width, height);

            skip_downsample:;
            }

#ifdef PROFILE_TEX_DECOMP
            time_start = sceKernelGetProcessTimeWide();
#endif
            ((void (*)(bool))lockLoadingMutex_addr)(true);

            // Create D3D palette
            void *d3d_palette = ((void* (*)(int))D3DDevice_CreatePalette2_addr)(0);
            uint32_t *pal_lock = (uint32_t*)((int (*)(void*, int))D3DPalette_Lock2_addr)(d3d_palette, 0);

            // Apply saturation adjustment like original game (using constants from 0x00132030 and 0x00132034)
            // These values control color saturation/brightness
            float fVar2 = *(float*)LOC(0x00132030);  // Average factor (typically ~0.333 for 1/3)
            float fVar3 = *(float*)LOC(0x00132034);  // Saturation factor (1.0 = normal, >1.0 = more saturated)

            for (int i = 0; i < 256; i++) {
                uint8_t b = pal_data[i * 4 + 0];
                uint8_t g = pal_data[i * 4 + 1];
                uint8_t r = pal_data[i * 4 + 2];
                uint8_t a = pal_data[i * 4 + 3];

                // Calculate average (grayscale)
                float avg = (float)(r + g + b) * fVar2;

                // Apply saturation adjustment: avg + (color - avg) * saturation
                int colorR = (int)(avg + ((float)r - avg) * fVar3 + 0.5f);
                int colorG = (int)(avg + ((float)g - avg) * fVar3 + 0.5f);
                int colorB = (int)(avg + ((float)b - avg) * fVar3 + 0.5f);

                // Saturate to 8-bit
                uint8_t satR = UnsignedSaturate8(colorR);
                uint8_t satG = UnsignedSaturate8(colorG);
                uint8_t satB = UnsignedSaturate8(colorB);

                pal_lock[i] = satR | (satG << 8) | (satB << 16) | (a << 24);  // Output as RGBA
            }

            entry->palette_ptr = (uint32_t)d3d_palette;  // +0x14

            void *d3d_texture = ((void* (*)(int, int, uint32_t, int, uint32_t, uint32_t, uint32_t))D3DDevice_CreateTexture2_addr)(width, height, 1, 0, 0, 0x8b, 3);

            int lock_rect[2];
            ((void (*)(void*, uint32_t, int*, int*, int))D3DTexture_LockRect_addr)(d3d_texture, 0, lock_rect, NULL, 0);
            memcpy((void*)lock_rect[1], temp_buffer, total_pixels);
            ((int (*)(void*, int))D3DTexture_UnlockRect_addr)(d3d_texture, 0);

            entry->texture_ptr = (uint32_t)d3d_texture;  // +0x10
            // Initialize to high value to prevent discard (game discards after 30 frames of non-use)
            entry->last_used_frame = 0;

            // Track GPU memory: texture data + header (0x14) + palette (1024 bytes)
            total_gpu_memory += (total_pixels + 0x14 + 1024);

            //logv_error("    Stored: entry=0x%08X, tex=0x%08X @ +0x10, pal=0x%08X @ +0x14, frame=0x%08X @ +0x18",(uint32_t)entry, entry->texture_ptr, entry->palette_ptr, entry->last_used_frame);

            // Verify the pointers are non-null
            if (!entry->texture_ptr || !entry->palette_ptr) {
                logv_error("    WARNING: NULL pointer! tex=%p pal=%p", entry->texture_ptr, entry->palette_ptr);
            }

            ((void (*)(void))releaseLoadingMutex_addr)();
#ifdef PROFILE_TEX_DECOMP
            time_end = sceKernelGetProcessTimeWide();
            time_gpu_upload += (time_end - time_start);
#endif
        }
    

        logv_debug("  Chunk (%d/%d) complete: %d  textures loaded",chunk_idx, tex_end - tex_start, num_textures);
    }

    log_debug("=== All chunks processed ===");

    logv_debug("will clear temp_buffer: %p", temp_buffer);
    if (temp_buffer) free(temp_buffer);

    log_debug("will clear tex_data");
    if (tex_data) free(tex_data);

    log_debug("will clear chunk_offsets");
    free(chunk_offsets);

    log_debug("will clear chunk_size");
    free(chunk_sizes);

    log_debug("Closing file");
    ((int (*)(int))machHostClose_addr)(file);
    log_debug("File closed");

    logv_debug("Max texture dimensions encountered: %dx%d (%d pixels)",
               max_width_seen, max_height_seen, max_width_seen * max_height_seen);
    logv_debug("Total GPU memory allocated: %llu bytes (%.2f MB)",
               total_gpu_memory, total_gpu_memory / (1024.0 * 1024.0));

#ifdef PROFILE_TEX_DECOMP
    log_debug("=== PROFILER RESULTS ===");
    logv_debug("Textures:        %d", total_textures);
    logv_debug("Max dimensions:  %dx%d (%d pixels)", max_width_seen, max_height_seen, max_width_seen * max_height_seen);
    logv_debug("GPU memory:      %llu bytes (%.2f MB, avg %.1f KB per texture)",
               total_gpu_memory, total_gpu_memory / (1024.0 * 1024.0),
               total_textures > 0 ? total_gpu_memory / (1024.0 * total_textures) : 0.0f);
    logv_debug("Blocks decoded:  %d (avg %.1f per texture)", total_blocks_decoded, total_textures > 0 ? (float)total_blocks_decoded / total_textures : 0.0f);
    logv_debug("File I/O:        %llu us", time_file_io);
    logv_debug("Huff Table:      %llu us", time_huff_table);
    logv_debug("Decode Blocks:   %llu us", time_decode_blocks);
    logv_debug("  Huff Decode:   %llu us (%.1f us per block)", time_huff_decode, total_blocks_decoded > 0 ? (float)time_huff_decode / total_blocks_decoded : 0.0f);
    logv_debug("  Block Copy:    %llu us (%.1f us per block)", time_block_copy, total_blocks_decoded > 0 ? (float)time_block_copy / total_blocks_decoded : 0.0f);
    logv_debug("GPU Upload:      %llu us", time_gpu_upload);
    log_debug("========================");
#endif

    log_debug("=== worldAllocateSegments COMPLETE ===");
}
