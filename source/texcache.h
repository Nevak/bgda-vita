#ifndef TEXCACHE_H
#define TEXCACHE_H

#include <stdint.h>
#include <stdbool.h>

#define TEXCACHE_DIR "ux0:data/bgda/texcache/"
#define TEXCACHE_MAGIC 0x54455843  // 'TEXC'
#define TEXCACHE_VERSION 2  // Bumped: added chunk_idx to TexCacheChunk

// Cache file header
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t num_chunks;
    uint32_t total_size;
} TexCacheFileHeader;

// Chunk header in cache
typedef struct {
    uint32_t chunk_idx;     // Original chunk index (for sparse arrays)
    uint32_t num_textures;
    // Followed by num_textures × TexCacheTexture entries
} TexCacheChunk;

// Texture header in cache
typedef struct {
    uint16_t width;
    uint16_t height;
    uint32_t pixel_data_size;
    uint32_t has_palette;
    // Followed by pixel_data[pixel_data_size]
    // Followed by palette[256] (RGBA) if has_palette
} TexCacheTexture;

// Forward declaration
typedef struct _worldHeader _worldHeader;

// Simple API
bool texcache_load_world(const char *world_name, _worldHeader *worldHeader);
void texcache_save_world(const char *world_name, const void *cache_data, uint32_t size, uint32_t num_chunks);

#endif // TEXCACHE_H
