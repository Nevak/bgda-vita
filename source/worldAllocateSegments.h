/*
 * World Texture Decoder - Simplified implementation based on BGDA Explorer
 * Original C# code by Ian Brown (GPL v3)
 *
 * This decoder handles the custom texture compression used in Dark Alliance/Soul Calibur
 * The compression uses:
 * - 256-color palettes (RGBA, swizzled)
 * - Huffman coding for palette indices
 * - Back-references to previously decoded pixels
 * - 16x16 block-based decoding
 *
 * ============================================================================
 * TEXTURE FORMAT STRUCTURE:
 * ============================================================================
 *
 * .tex file contains texture chunks:
 * - Each chunk starts with: int32 num_textures
 * - Followed by 0x40 bytes of padding
 * - Then texture entries (0x38 bytes each)
 *
 * Each texture entry contains:
 * - width, height (uint16 each)
 * - data_offset (relative to chunk start)
 *
 * Compressed data structure:
 * - int32: palette offset (relative to chunk start)
 * - 4 bytes of metadata
 * - Block list: [x0,y0,x1,y1, offset, ...] terminated by 0xFF
 *
 * Palette structure (at palette_offset):
 * - 256 RGBA entries (1024 bytes) - SWIZZLED
 * - +0x400: Table0 (prediction table, 2048 bytes)
 * - +0xC00: Huffman tables (3 tables for variable-length decoding)
 *
 * Each 16x16 block is Huffman-coded:
 * - Pixel values 0x00-0xFF: Direct palette index
 * - Pixel values 0x100-0x104: Back-references (copy previous pixels)
 * - Pixel values ≥0x105: Prediction based on previous pixel + table0
 *
 * ============================================================================
 * IMPROVEMENTS OVER ORIGINAL IMPLEMENTATION:
 * ============================================================================
 *
 * 1. Clear separation of concerns (decode functions vs GPU upload)
 * 2. Eliminated confusing offset calculations with proper delta tracking
 * 3. Pre-computed Huffman lookup table for 8-bit prefixes (fast path)
 * 4. Simplified bitstream reading
 * 5. Documented all magic constants and offsets
 * 6. Removed global state and made functions reusable
 */

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
#ifdef USE_TEXCACHE
#include "texcache.h"
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#endif

#define PROFILE_TEX_DECOMP

// External declarations
extern void* malloc(size_t size);
extern void free(void* ptr);
extern void* calloc(size_t num, size_t size);
extern void* memcpy(void* dst, const void* src, size_t n);
extern void* memset(void* dst, int val, size_t n);
extern int sprintf(char* str, const char* format, ...);

#ifdef PROFILE_TEX_DECOMP
extern uint64_t sceKernelGetProcessTimeWide(void);
#endif

// Direct function addresses (instead of hooked versions)
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

// Helper for saturation
static inline uint8_t UnsignedSaturate8(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (uint8_t)value;
}

// World header structure - CRITICAL: padding must match exact offsets!
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


/**
 * Read 16-bit value from bitstream at arbitrary bit position
 */
static inline uint16_t read_bits(const uint8_t *data, int bit_position, int num_bits) {
    int byte_pos = bit_position / 8;
    int bit_offset = bit_position & 7;

    // Read 3 bytes and extract the bits we need
    uint32_t value = (data[byte_pos] << 16) | (data[byte_pos + 1] << 8) | data[byte_pos + 2];
    value >>= (24 - bit_offset - num_bits);
    value &= (1 << num_bits) - 1;

    return (uint16_t)value;
}

/**
 * Decode Huffman lookup table for fast 8-bit prefix decoding
 * This pre-computes values for all 8-bit prefixes, allowing O(1) lookup
 * for short codes (1-8 bits)
 * @param huff_out - flat array of 512 shorts: [val0, numBits0, val1, numBits1, ...]
 */
static inline __attribute__((always_inline)) void decode_huff_table(const uint8_t * __restrict file_data, int table_offset, int16_t * __restrict huff_out) {
    int table1_len_val = *(int32_t*)(file_data + table_offset);
    int table1_len = table1_len_val * 2;
    int table1_start = table_offset + 4;
    int table2_start = table1_start + table1_len;
    int table3_start = table2_start + 0x48;

    // logv_error("    decode_huff: table_off=0x%08X, t1_len_val=%d, t1_len=%d, t1_start=0x%08X\n",
    //            table_offset, table1_len_val, table1_len, table1_start);

    for (int i = 0; i < 256; i++) {
        int bit = 1;
        uint32_t a = (uint32_t)i >> (8 - bit);  // USE UNSIGNED
        int v = *(int32_t*)(file_data + table3_start + (bit * 4));

        // Find the bit length needed to decode this prefix
        while (v < (int)a && bit <= 8) {
            bit++;
            a = (uint32_t)i >> (8 - bit);
            v = *(int32_t*)(file_data + table3_start + (bit * 4));
        }

        if (bit <= 8) {
            // Short code - can be decoded with 8-bit lookup
            int val = *(int32_t*)(file_data + table2_start + (bit * 4));
            int table1_index = (int)a + val;
            huff_out[i * 2] = *(int16_t*)(file_data + table1_start + (table1_index * 2));  // val
            huff_out[i * 2 + 1] = (int16_t)bit;  // numBits
        } else {
            // Long code - needs full decode
            huff_out[i * 2] = 0;     // val
            huff_out[i * 2 + 1] = 0;  // numBits = 0 means needs full decode
        }
    }
}

/**
 * Decode a texture from YAK file chunk
 *
 * @param file_data     Full file data
 * @param entry_offset  Offset to texture entry in chunk
 * @param chunk_offset  Offset to chunk start (for RTA) or entry (for BGDA)
 * @param is_rta        True for Return to Arms format, false for Dark Alliance
 * @return              Decoded texture data (8-bit palette indices), or NULL on error
 */
static inline __attribute__((always_inline)) uint8_t* decode_texture(
    const uint8_t *file_data,
    int entry_offset,
    int chunk_offset,
    bool is_rta,
    uint16_t *out_width,
    uint16_t *out_height,
    PalEntry **out_palette
) {
    // Delta offset depends on game version
    int delta_offset = is_rta ? chunk_offset : entry_offset;

    // Read texture header
    uint16_t pixel_width = *(uint16_t*)(file_data + entry_offset);
    uint16_t pixel_height = *(uint16_t*)(file_data + entry_offset + 2);
    int32_t header8 = *(int32_t*)(file_data + entry_offset + 0x8);
    int compressed_data_offset = header8 + delta_offset;

    // Read palette offset and decode palette
    int pal_offset = *(int32_t*)(file_data + compressed_data_offset) + delta_offset;
    PalEntry *palette = (PalEntry*)malloc(256 * sizeof(PalEntry));

    // Read raw palette data (16x16 RGBA entries)
    for (int i = 0; i < 256; i++) {
        palette[i].r = file_data[pal_offset + i * 4];
        palette[i].g = file_data[pal_offset + i * 4 + 1];
        palette[i].b = file_data[pal_offset + i * 4 + 2];
        palette[i].a = file_data[pal_offset + i * 4 + 3];
    }


    // Decode Huffman table
    HuffVal *huff_values = (HuffVal*)malloc(256 * sizeof(HuffVal));
    memset(huff_values, 0, 256 * sizeof(HuffVal));
    decode_huff_table(file_data, pal_offset + 0xc00, huff_values);

    // Allocate output buffer (round up to 16-pixel boundaries)
    int width = (pixel_width + 0x0f) & ~0x0f;
    int height = (pixel_height + 0x0f) & ~0x0f;
    uint8_t *output = (uint8_t*)calloc(width * height, 1);

    // Pre-calculate Huffman table offsets ONCE per texture (not per block!)
    int table0_start = pal_offset + 0x400;
    int table_offset = table0_start + 0x800;
    int table1_len_val = *(int32_t*)(file_data + table_offset);
    int table1_len = table1_len_val * 2;
    int table1_start = table_offset + 4;
    int table2_start = table1_start + table1_len;
    int table3_start = table2_start + 0x48;

    // Decode blocks
    int p = compressed_data_offset + 4;
    while (file_data[p] != 0xFF) {
        int x0 = file_data[p];
        int y0 = file_data[p + 1];
        int x1 = file_data[p + 2];
        int y1 = file_data[p + 3];
        p += 4;

        for (int y_block = y0; y_block <= y1; y_block++) {
            for (int x_block = x0; x_block <= x1; x_block++) {
                int block_data_start = *(int32_t*)(file_data + p) + delta_offset;
                decode_block(file_data, block_data_start,
                           table0_start, table1_start, table2_start, table3_start,
                           huff_values, x_block, y_block, width, output);
                p += 4;
            }
        }
    }

    free(huff_values);

    *out_width = width;
    *out_height = height;
    *out_palette = palette;

    return output;
}

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

/**
 * Main function to allocate and decode all world textures
 */
void worldAllocateSegments(_worldHeader *worldHeader) {
    log_error("=== worldAllocateSegments START ===\n");

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

    // Skip if already allocated
    if (worldHeader->isAllocated != 0) {
        //log_error("worldAllocateSegments: already allocated, skipping\n");
        return;
    }
    worldHeader->isAllocated = 1;

    // Get world name (needed for both cache and normal path)
    char *world_name = (char*)LOC(0x0054cad8);
    logv_error("worldAllocateSegments: world_name pointer=0x%08X\n", (uint32_t)world_name);

    // Load world lumps if needed (ALWAYS run this, even when using cache)
    int element_count = worldHeader->element_count;
    //logv_error("worldAllocateSegments: element_count=%d, elemen_array_start=0x%08X\n",
             //  element_count, worldHeader->elemen_array_start);

    if (element_count > 0) {
        for (int i = 0; i < element_count; i++) {
            uintptr_t element_addr = worldHeader->elemen_array_start + i * 0x38;
            uint32_t *element = (uint32_t*)element_addr;
            uint16_t flags = *(uint16_t*)((char*)element + 0x30);

            //logv_error("  Element[%d]: addr=0x%08X, element[0]=0x%08X, flags=0x%04X\n",
                      //i, element_addr, element[0], flags);

            if (flags & 0x800) {
                char lump_name[80];
                char *name = (char*)element[0];  // Single dereference - element[0] IS the pointer
                //logv_error("  Loading lump with name pointer=0x%08X\n", (uint32_t)name);
                sprintf(lump_name, "%s.lmp", name);
                ((int (*)(char*))lumpLoad_addr)(lump_name);
            }
        }
    }

#ifdef USE_TEXCACHE
    // Try loading from cache first
    if (texcache_load_world(world_name, worldHeader)) {
        log_error("=== worldAllocateSegments COMPLETE (from cache) ===\n");
        return;
    }
    log_error("Cache miss, decompressing textures...\n");
#endif

    // Get texture range and chunk array
    int tex_start = worldHeader->tex_start;  // 4949
    int tex_end = worldHeader->tex_end;    // 5050
    TexChunkInfo *chunks = (TexChunkInfo*)worldHeader->chunks;

    // Load .tex file
#ifdef PROFILE_TEX_DECOMP
    time_start = sceKernelGetProcessTimeWide();
#endif
    char tex_path[80];

    sprintf(tex_path, "res\\%s.tex", world_name);

    int file = ((int (*)(char*, char*))machHostOpen_addr)(tex_path, "rb");
    int file_size = ((int (*)(int, int, int))machHostSeek_addr)(file, 0, 2);

    void *tex_data = malloc(file_size);

    ((int (*)(int, int, int))machHostSeek_addr)(file, 0, 0);
    ((int (*)(int, void*, int))machHostRead_addr)(file, tex_data, file_size);
    ((int (*)(int))machHostClose_addr)(file);
    
#ifdef PROFILE_TEX_DECOMP
    time_end = sceKernelGetProcessTimeWide();
    time_file_io += (time_end - time_start);
#endif

    if (tex_end < tex_start) {
        free(tex_data);
        return;
    }

    // Process each texture chunk
    uint8_t *temp_buffer = NULL;
    int16_t huff_table[512];  // Reusable Huffman table buffer - flat array like original! [val, numBits, val, numBits, ...]
    uint8_t block_buffer[256];  // Reusable decode buffer (like original's rectPtr/decompressBlock)

#ifdef USE_TEXCACHE
    // Cache building infrastructure - write incrementally to disk
    SceUID cache_fd = -1;
    uint32_t cache_size = 0;
    uint32_t chunks_cached = 0;

    // Create cache directory and open file
    sceIoMkdir(TEXCACHE_DIR, 0777);
    char cache_path[256];
    sprintf(cache_path, TEXCACHE_DIR "%s.cache", world_name);
    cache_fd = sceIoOpen(cache_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);

    if (cache_fd >= 0) {
        // Write placeholder header (will update at end)
        TexCacheFileHeader cache_header = {
            .magic = TEXCACHE_MAGIC,
            .version = TEXCACHE_VERSION,
            .num_chunks = 0,
            .total_size = 0
        };
        sceIoWrite(cache_fd, &cache_header, sizeof(cache_header));
        log_error("Opened cache file for incremental writing\n");
    } else {
        logv_error("Failed to open cache file: %d\n", cache_fd);
    }
#endif

    for (int chunk_idx = 0; chunk_idx <= (tex_end - tex_start); chunk_idx++) {
        TexChunkInfo *chunk = &chunks[chunk_idx];

        if (chunk->flags == 0 || chunk->tex_data_offset == 0) {
            continue;
        }

        // IMPORTANT: Save the original chunk offset before we overwrite it!
        uint32_t chunk_file_offset = chunk->tex_data_offset;

        // Read chunk header
        uint8_t *chunk_data = (uint8_t*)tex_data + chunk_file_offset;
        int num_textures = *(int32_t*)chunk_data;

        // Skip if no textures in this chunk
        if (num_textures <= 0) {
            chunk->tex_data_offset = 0;
            continue;
        }

#ifdef USE_TEXCACHE
        // Mark chunk start position in file
        SceOff chunk_header_pos = -1;
        uint32_t chunk_tex_count = 0;

        if (cache_fd >= 0) {
            // Remember position and write placeholder chunk header
            chunk_header_pos = sceIoLseek(cache_fd, 0, SCE_SEEK_CUR);
            TexCacheChunk chunk_hdr = { .chunk_idx = chunk_idx, .num_textures = 0 };
            sceIoWrite(cache_fd, &chunk_hdr, sizeof(chunk_hdr));
        }
#endif

        // Allocate texture entry array
        // Layout: count at +0x00 (4 bytes), Entry0 at +0x04, Entry1 at +0x3C, ...
        uint8_t *entries_mem = (uint8_t*)malloc(num_textures * 0x38 + 4);
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
            // Compressed data offset is relative to entry position
            int data_offset = file_entry->data_offset + entry_file_offset;
            uint8_t *comp_data = (uint8_t*)tex_data + data_offset;

            // Palette offset (stored in comp_data) is also relative to entry position
            int pal_offset = *(int32_t*)comp_data + entry_file_offset;
            uint8_t *pal_data = (uint8_t*)tex_data + pal_offset;

            // Build Huffman lookup table INLINE (like original - no function call!)
#ifdef PROFILE_TEX_DECOMP
            time_start = sceKernelGetProcessTimeWide();
#endif
            int huff_table_offset = pal_offset + 0xc00;
            int ht_table1_len_val = *(int32_t*)((uint8_t*)tex_data + huff_table_offset);
            int ht_table1_len = ht_table1_len_val * 2;
            int ht_table1_start = huff_table_offset + 4;
            int ht_table2_start = ht_table1_start + ht_table1_len;
            int ht_table3_start = ht_table2_start + 0x48;

            for (int i = 0; i < 256; i++) {
                int bit = 1;
                uint32_t a = (uint32_t)i >> (8 - bit);
                int v = *(int32_t*)((uint8_t*)tex_data + ht_table3_start + (bit * 4));

                while (v < (int)a && bit <= 8) {
                    bit++;
                    a = (uint32_t)i >> (8 - bit);
                    v = *(int32_t*)((uint8_t*)tex_data + ht_table3_start + (bit * 4));
                }

                if (bit <= 8) {
                    int val = *(int32_t*)((uint8_t*)tex_data + ht_table2_start + (bit * 4));
                    int table1_index = (int)a + val;
                    huff_table[i * 2] = *(int16_t*)((uint8_t*)tex_data + ht_table1_start + (table1_index * 2));
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
            int table1_len_val = *(int32_t*)((uint8_t*)tex_data + table_offset);
            int table1_len = table1_len_val * 2;
            g_table1_start = table_offset + 4;
            g_table2_start = g_table1_start + table1_len;
            g_table3_start = g_table2_start + 0x48;

            // Calculate texture dimensions (power of 2, min 64)
            // Use file_entry for width/height since entry 0 has count overlapping these fields
            int width = ((int (*)(int))lowestPowerof2NotLessThan_addr)(file_entry->width);
            if (width < 0x40) width = 0x40;
            int height = ((int (*)(int))lowestPowerof2NotLessThan_addr)(file_entry->height);

            int total_pixels = width * height;

            // Allocate NEW buffer for each texture like original (don't reuse!)
            if (temp_buffer) free(temp_buffer);
            temp_buffer = (uint8_t*)malloc(total_pixels + 0x19000);  // Extra space like original
            memset(temp_buffer, 0, total_pixels);  // Clear like original

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
                            // Use restrict to help compiler aliasing analysis
                            uint16_t *__restrict block_data_ptr = (uint16_t*)(file_data + block_data_start);
                            int32_t *__restrict table3_ptr = (int32_t*)(file_data + g_table3_start);
                            int32_t *__restrict table2_ptr = (int32_t*)(file_data + g_table2_start);
                            int16_t *__restrict table1_ptr = (int16_t*)(file_data + g_table1_start);

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
                                    prev_pixel = file_data[g_table0_start + (pix_cmd - 0x105) + (prev_pixel << 3)];
                                    block_buffer[cur_pix8] = prev_pixel;
                                }

                                cur_pix8++;
                                i++;
                            } while (i != 0x100);
#ifdef PROFILE_TEX_DECOMP
                            uint64_t time_huff_end = sceKernelGetProcessTimeWide();
                            time_huff_decode += (time_huff_end - time_huff_start);
#endif

                            // Bulk copy 16x16 block to output buffer (fully unrolled like original)
#ifdef PROFILE_TEX_DECOMP
                            uint64_t time_copy_start = sceKernelGetProcessTimeWide();
#endif
                            int block_x = x_block * 16;
                            int block_y = y_block * 16;
                            uint8_t *src = block_buffer;
                            uint8_t *dst = &temp_buffer[block_y * width + block_x];

                            // Fully unrolled copy - compiler will use NEON
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8); dst += width; src += 16;
                            *(uint64_t*)(dst) = *(uint64_t*)(src); *(uint64_t*)(dst + 8) = *(uint64_t*)(src + 8);
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

            // Upload to GPU
#ifdef PROFILE_TEX_DECOMP
            time_start = sceKernelGetProcessTimeWide();
#endif
            ((void (*)(bool))lockLoadingMutex_addr)(true);

            // Create D3D palette
            void *d3d_palette = ((void* (*)(int))D3DDevice_CreatePalette2_addr)(0);
            uint32_t *pal_lock = (uint32_t*)((int (*)(void*, int))D3DPalette_Lock2_addr)(d3d_palette, 0);

            // Ultra-fast: copy palette directly from file (GBRA format), let shader handle color swizzling
            memcpy(pal_lock, pal_data, 1024);  // 256 entries * 4 bytes

            entry->palette_ptr = (uint32_t)d3d_palette;  // +0x14

            void *d3d_texture = ((void* (*)(int, int, uint32_t, int, uint32_t, uint32_t, uint32_t))D3DDevice_CreateTexture2_addr)(width, height, 1, 0, 0, 0x8b, 3);

            int lock_rect[2];
            ((void (*)(void*, uint32_t, int*, int*, int))D3DTexture_LockRect_addr)(d3d_texture, 0, lock_rect, NULL, 0);
            memcpy((void*)lock_rect[1], temp_buffer, total_pixels);
            ((int (*)(void*, int))D3DTexture_UnlockRect_addr)(d3d_texture, 0);

            entry->texture_ptr = (uint32_t)d3d_texture;  // +0x10
            // Initialize to high value to prevent discard (game discards after 30 frames of non-use)
            entry->last_used_frame = 0x7FFFFFFF;  // +0x18: Max int to never discard

            //logv_error("    Stored: entry=0x%08X, tex=0x%08X @ +0x10, pal=0x%08X @ +0x14, frame=0x%08X @ +0x18\n",(uint32_t)entry, entry->texture_ptr, entry->palette_ptr, entry->last_used_frame);

            // Verify the pointers are non-null
            if (!entry->texture_ptr || !entry->palette_ptr) {
                logv_error("    WARNING: NULL pointer! tex=%p pal=%p\n", entry->texture_ptr, entry->palette_ptr);
            }

            ((void (*)(void))releaseLoadingMutex_addr)();
#ifdef PROFILE_TEX_DECOMP
            time_end = sceKernelGetProcessTimeWide();
            time_gpu_upload += (time_end - time_start);
#endif

#ifdef USE_TEXCACHE
            // Append texture to cache file
            if (cache_fd >= 0) {
                uint32_t tex_palette_size = 256 * 4;  // 256 RGBA entries

                // Write texture header
                TexCacheTexture tex_hdr = {
                    .width = width,
                    .height = height,
                    .pixel_data_size = total_pixels,
                    .has_palette = 1
                };
                sceIoWrite(cache_fd, &tex_hdr, sizeof(tex_hdr));

                // Write pixel data
                sceIoWrite(cache_fd, temp_buffer, total_pixels);

                // Write palette
                sceIoWrite(cache_fd, pal_data, tex_palette_size);

                chunk_tex_count++;
            }
#endif
        }

#ifdef USE_TEXCACHE
        // Update chunk header with actual texture count
        if (cache_fd >= 0 && chunk_tex_count > 0) {
            SceOff current_pos = sceIoLseek(cache_fd, 0, SCE_SEEK_CUR);
            sceIoLseek(cache_fd, chunk_header_pos, SCE_SEEK_SET);
            TexCacheChunk chunk_hdr = { .chunk_idx = chunk_idx, .num_textures = chunk_tex_count };
            sceIoWrite(cache_fd, &chunk_hdr, sizeof(chunk_hdr));
            sceIoLseek(cache_fd, current_pos, SCE_SEEK_SET);
            chunks_cached++;
        }
#endif

        logv_error("  Chunk (%d/%d) complete: %d  textures loaded\n",chunk_idx, tex_end - tex_start, num_textures);
    }

    log_error("=== All chunks processed ===\n");

#ifdef USE_TEXCACHE
    log_error("=== Finalizing cache ===\n");
    logv_error("chunks_cached=%u\n", chunks_cached);

    if (cache_fd >= 0) {
        if (chunks_cached > 0) {
            // Get final file size
            SceOff final_pos = sceIoLseek(cache_fd, 0, SCE_SEEK_CUR);
            uint32_t total_size = (uint32_t)(final_pos - sizeof(TexCacheFileHeader));

            // Update header with final values
            sceIoLseek(cache_fd, 0, SCE_SEEK_SET);
            TexCacheFileHeader final_header = {
                .magic = TEXCACHE_MAGIC,
                .version = TEXCACHE_VERSION,
                .num_chunks = chunks_cached,
                .total_size = total_size
            };
            sceIoWrite(cache_fd, &final_header, sizeof(final_header));

            logv_error("Cache saved: %u bytes, %u chunks\n", total_size, chunks_cached);
        }
        sceIoClose(cache_fd);
        log_error("Cache file closed\n");
    }
#endif

    // Cleanup
    log_error("Before free(temp_buffer)\n");
    if (temp_buffer) free(temp_buffer);
    log_error("After free(temp_buffer)\n");

    log_error("Before free(tex_data)\n");
    free(tex_data);
    log_error("After free(tex_data)\n");

#ifdef PROFILE_TEX_DECOMP
    log_error("=== PROFILER RESULTS ===\n");
    logv_error("Textures:        %d\n", total_textures);
    logv_error("Blocks decoded:  %d (avg %.1f per texture)\n", total_blocks_decoded, total_textures > 0 ? (float)total_blocks_decoded / total_textures : 0.0f);
    logv_error("File I/O:        %llu us\n", time_file_io);
    logv_error("Huff Table:      %llu us\n", time_huff_table);
    logv_error("Decode Blocks:   %llu us\n", time_decode_blocks);
    logv_error("  Huff Decode:   %llu us (%.1f us per block)\n", time_huff_decode, total_blocks_decoded > 0 ? (float)time_huff_decode / total_blocks_decoded : 0.0f);
    logv_error("  Block Copy:    %llu us (%.1f us per block)\n", time_block_copy, total_blocks_decoded > 0 ? (float)time_block_copy / total_blocks_decoded : 0.0f);
    logv_error("GPU Upload:      %llu us\n", time_gpu_upload);
    log_error("========================\n");
#endif

    log_error("=== worldAllocateSegments COMPLETE ===\n");
}
