#include "texcache.h"
#include "utils/logger.h"
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// _worldHeader structure definition (matching worldAllocateSegments.h)
struct _worldHeader {
    int element_count;           // at 0x00
    uint8_t padding1[0x20];      // 0x04 to 0x24
    uint32_t elemen_array_start; // at 0x24
    uint8_t padding2[0x30];      // 0x28 to 0x58
    int tex_start;               // at 0x58
    int tex_end;                 // at 0x5c
    uint8_t padding3[0x04];      // 0x60 to 0x64
    uint32_t chunks;             // at 0x64
    uint8_t padding4[0x0c];      // 0x68 to 0x74
    int isAllocated;             // at 0x74
};

// External functions (from worldAllocateSegments.h)
extern void *D3DDevice_CreatePalette2_addr;
extern void *D3DPalette_Lock2_addr;
extern void *D3DDevice_CreateTexture2_addr;
extern void *D3DTexture_LockRect_addr;
extern void *D3DTexture_UnlockRect_addr;
extern void *lockLoadingMutex_addr;
extern void *releaseLoadingMutex_addr;

// Helper to create GPU texture from cached data
static void create_gpu_texture_from_cache(int *entries, int tex_idx,
                                          uint16_t width, uint16_t height,
                                          const uint8_t *pixel_data,
                                          const uint32_t *palette_data) {
    typedef struct {
        uint16_t width;            // +0x00
        uint16_t height;           // +0x02
        uint32_t unk[3];           // +0x04 to +0x0F (12 bytes)
        uint32_t texture_ptr;      // +0x10
        uint32_t palette_ptr;      // +0x14
        uint32_t last_used_frame;  // +0x18
    } WorldTexEntryRuntime;

    // Note: entries are accessed at multiples of 0x38 from base (not +4!)
    // The count at offset 0 overlaps with first entry's width/height
    WorldTexEntryRuntime *entry = (WorldTexEntryRuntime*)((char*)entries + tex_idx * 0x38);

    // Set width and height (game reads these!)
    entry->width = width;
    entry->height = height;

    // Lock mutex (per texture, like normal path)
    ((void (*)(bool))lockLoadingMutex_addr)(true);

    // Create palette
    void *d3d_palette = ((void* (*)(int))D3DDevice_CreatePalette2_addr)(0);
    uint32_t *pal_lock = (uint32_t*)((int (*)(void*, int))D3DPalette_Lock2_addr)(d3d_palette, 0);

    // Copy palette
    memcpy(pal_lock, palette_data, 256 * sizeof(uint32_t));

    entry->palette_ptr = (uint32_t)d3d_palette;  // +0x14

    // Create texture
    void *d3d_texture = ((void* (*)(int, int, uint32_t, int, uint32_t, uint32_t, uint32_t))
                         D3DDevice_CreateTexture2_addr)(width, height, 1, 0, 0, 0x8b, 3);

    // Lock and upload pixel data
    int lock_rect[2];
    ((void (*)(void*, uint32_t, int*, int*, int))D3DTexture_LockRect_addr)(d3d_texture, 0, lock_rect, NULL, 0);
    memcpy((void*)lock_rect[1], pixel_data, width * height);
    ((int (*)(void*, int))D3DTexture_UnlockRect_addr)(d3d_texture, 0);

    entry->texture_ptr = (uint32_t)d3d_texture;  // +0x10
    // Initialize to high value to prevent discard (game discards after 30 frames of non-use)
    entry->last_used_frame = 0x7FFFFFFF;  // +0x18

    // Release mutex (per texture, like normal path)
    ((void (*)(void))releaseLoadingMutex_addr)();
}

// Load world from cache
bool texcache_load_world(const char *world_name, _worldHeader *worldHeader) {
    char cache_path[256];
    sprintf(cache_path, TEXCACHE_DIR "%s.cache", world_name);

    // Check if cache exists
    SceUID fd = sceIoOpen(cache_path, SCE_O_RDONLY, 0);
    if (fd < 0) {
        logv_error("Cache doesn't exist: %s", cache_path);
        return false;  // Cache doesn't exist
    }

    // Read header
    TexCacheFileHeader header;
    if (sceIoRead(fd, &header, sizeof(header)) != sizeof(header)) {
        log_error("Cache header invalid");
        sceIoClose(fd);
        return false;
    }

    // Validate magic/version
    if (header.magic != TEXCACHE_MAGIC || header.version != TEXCACHE_VERSION) {
        logv_error("Invalid cache for %s (magic=0x%08X, version=%u)\n",
                   world_name, header.magic, header.version);
        sceIoClose(fd);
        return false;
    }

    logv_error("Loading %s from cache (%u bytes, %u chunks)\n",
               world_name, header.total_size, header.num_chunks);

    // Read and process cache incrementally (don't load entire file into memory)

    int tex_start = worldHeader->tex_start;
    int tex_end = worldHeader->tex_end;

    // TexChunkInfo structure (8 bytes per entry)
    typedef struct {
        uint32_t tex_data_offset;
        uint32_t flags;
    } TexChunkInfo;

    TexChunkInfo *chunks = (TexChunkInfo*)worldHeader->chunks;

    //logv_error("tex_start=%d, tex_end=%d, chunks=0x%08X\n", tex_start, tex_end, (uint32_t)chunks);

    // IMPORTANT: Zero out ALL chunk entries first (game iterates through all of them)
    int num_chunks = tex_end - tex_start + 1;
    for (int i = 0; i < num_chunks; i++) {
        chunks[i].tex_data_offset = 0;
        chunks[i].flags = 0;
    }
    //log_error("Zeroed all chunk entries\n");

    // Allocate buffers for reading (reused for each texture)
    uint8_t *pixel_buffer = NULL;
    uint32_t pixel_buffer_size = 0;
    uint32_t palette_data[256];  // Reusable palette buffer

    // Read chunks from cache (sparse - only chunks that had textures)
    for (uint32_t i = 0; i < header.num_chunks; i++) {
       // logv_error("Reading cache chunk %u/%u\n", i + 1, header.num_chunks);

        // Read chunk header
        TexCacheChunk chunk_hdr;
        if (sceIoRead(fd, &chunk_hdr, sizeof(chunk_hdr)) != sizeof(chunk_hdr)) {
            //log_error("Failed reading chunk header\n");
            break;
        }

       // logv_error("  Chunk idx=%u has %u textures\n", chunk_hdr.chunk_idx, chunk_hdr.num_textures);

        if (chunk_hdr.num_textures == 0) continue;

        // Allocate entry array for this chunk (zero it to avoid garbage data!)
        //log_error("  Allocating entry array\n");
        int *entries = (int*)calloc(1, chunk_hdr.num_textures * 0x38 + 4);
        *(int*)entries = chunk_hdr.num_textures;

        // Store entries pointer in chunks array (use the stored chunk_idx for sparse array!)
        chunks[chunk_hdr.chunk_idx].tex_data_offset = (uint32_t)entries;

        //logv_error("  entries=0x%08X, stored at chunks[%u]\n", (uint32_t)entries, chunk_hdr.chunk_idx);

        // Save the count (at offset 0) before we potentially overwrite it
        uint32_t saved_count = *(uint32_t*)entries;

        // For each texture in chunk
        for (uint32_t tex_idx = 0; tex_idx < chunk_hdr.num_textures; tex_idx++) {
            //logv_error("  Reading texture %u/%u\n", tex_idx + 1, chunk_hdr.num_textures);

            // Read texture header
            TexCacheTexture tex_hdr;
            if (sceIoRead(fd, &tex_hdr, sizeof(tex_hdr)) != sizeof(tex_hdr)) {
                //logv_error("Failed reading texture %d header\n", tex_idx);
                break;
            }

            //logv_error("    Texture: %ux%u, %u pixels\n", tex_hdr.width, tex_hdr.height, tex_hdr.pixel_data_size);

            // Ensure pixel buffer is large enough
            if (tex_hdr.pixel_data_size > pixel_buffer_size) {
                //logv_error("    Realloc pixel buffer: %u -> %u\n", pixel_buffer_size, tex_hdr.pixel_data_size);
                pixel_buffer = (uint8_t*)realloc(pixel_buffer, tex_hdr.pixel_data_size);
                pixel_buffer_size = tex_hdr.pixel_data_size;
            }

            // Read pixel data
            //log_error("    Reading pixel data\n");
            if (sceIoRead(fd, pixel_buffer, tex_hdr.pixel_data_size) != tex_hdr.pixel_data_size) {
                //logv_error("Failed reading texture %d pixels\n", tex_idx);
                break;
            }

            // Read palette if present
            uint32_t *pal_ptr = NULL;
            if (tex_hdr.has_palette) {
                //log_error("    Reading palette\n");
                if (sceIoRead(fd, palette_data, 256 * 4) != 256 * 4) {
                    logv_error("Failed reading texture %d palette\n", tex_idx);
                    break;
                }
                pal_ptr = palette_data;
            }

            // Create GPU texture
            //log_error("    Creating GPU texture\n");
            if (pal_ptr) {
                create_gpu_texture_from_cache(entries, tex_idx,
                                             tex_hdr.width, tex_hdr.height,
                                             pixel_buffer, pal_ptr);
            }
           // log_error("    Done with texture\n");
        }

        // Restore the count (it may have been overwritten by entry[0]'s width/height)
       *(uint32_t*)entries = saved_count;
       // logv_error("  Restored count to %u\n", saved_count);

       // log_error("  Chunk complete\n");
    }

    if (pixel_buffer) free(pixel_buffer);
    sceIoClose(fd);
    return true;
}

// Save world to cache (DEPRECATED - now done incrementally in worldAllocateSegments)
void texcache_save_world(const char *world_name, const void *cache_data,
                        uint32_t size, uint32_t num_chunks) {
    // This function is no longer used - caching is done incrementally
    // during texture decompression to avoid large memory allocations
    log_error("texcache_save_world called but incremental caching is now used\n");
}
