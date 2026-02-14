#include "utils/ptch.h"
#include "utils/utils.h"
#include "utils/logger.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <malloc.h>

#include <psp2/io/stat.h>

#ifdef USE_SCELIBC_IO
#include <libc_bridge/libc_bridge.h>
#endif

// PTCH format v2 constants
#define PTCH_MAGIC      "PTCH"
#define PTCH_VERSION    2

// Paths
#define PATCHES_DIR_DATA    DATA_PATH "patches/"
#define PATCHES_DIR_APP     "app0:/patches/"
#define APPLIED_FILE        DATA_PATH "patches/.applied"

// Header structure (packed, little-endian)
// Total: 4 + 4 + 8 + 4 + 2 = 22 bytes (before target_path)
typedef struct __attribute__((packed)) {
    char     magic[4];       // "PTCH"
    uint32_t version;        // 2
    uint64_t output_size;    // Size of target file
    uint32_t record_count;   // Number of diff records
    uint16_t path_len;       // Length of target_path string
} ptch_header_t;

// Record header structure (packed, little-endian)
typedef struct __attribute__((packed)) {
    uint64_t offset;         // Offset in target file
    uint32_t length;         // Length of data
} ptch_record_t;

/**
 * Extract just the filename from a full path.
 */
static const char *get_filename(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

/**
 * Check if a patch has already been applied by looking up its SHA1 in .applied file.
 */
static bool ptch_is_applied(const char *patch_path) {
    if (!file_exists(APPLIED_FILE)) {
        return false;
    }

    char *patch_sha1 = file_sha1sum(patch_path);
    if (!patch_sha1) {
        return false;
    }

    const char *patch_name = get_filename(patch_path);

    uint8_t *buffer;
    size_t size;
    if (!file_load(APPLIED_FILE, &buffer, &size)) {
        free(patch_sha1);
        return false;
    }

    // Search for "patch_name:sha1" in the file
    char search_str[512];
    snprintf(search_str, sizeof(search_str), "%s:%s", patch_name, patch_sha1);

    bool found = (strstr((char *)buffer, search_str) != NULL);

    free(buffer);
    free(patch_sha1);
    return found;
}

/**
 * Mark a patch as applied by appending its entry to .applied file.
 */
static bool ptch_mark_applied(const char *patch_path) {
    char *patch_sha1 = file_sha1sum(patch_path);
    if (!patch_sha1) {
        logv_error("ptch: Failed to compute SHA1 for %s", patch_path);
        return false;
    }

    const char *patch_name = get_filename(patch_path);

    // Ensure patches directory exists
    file_mkpath(APPLIED_FILE, 0755);

    // Append to .applied file
#ifdef USE_SCELIBC_IO
    FILE *f = sceLibcBridge_fopen(APPLIED_FILE, "ab");
#else
    FILE *f = fopen(APPLIED_FILE, "ab");
#endif

    if (!f) {
        logv_error("ptch: Failed to open %s for writing", APPLIED_FILE);
        free(patch_sha1);
        return false;
    }

    char line[512];
    snprintf(line, sizeof(line), "%s:%s\n", patch_name, patch_sha1);

#ifdef USE_SCELIBC_IO
    sceLibcBridge_fwrite(line, strlen(line), 1, f);
    sceLibcBridge_fclose(f);
#else
    fwrite(line, strlen(line), 1, f);
    fclose(f);
#endif

    free(patch_sha1);
    return true;
}

bool ptch_apply(const char *patch_path) {
    uint8_t *patch_data = NULL;
    size_t patch_size = 0;
    uint8_t *target_data = NULL;
    size_t target_size = 0;
    char *target_path = NULL;
    bool success = false;

    // Load patch file
    if (!file_load(patch_path, &patch_data, &patch_size)) {
        logv_error("ptch: Failed to load patch file %s", patch_path);
        goto cleanup;
    }

    // Validate minimum size for header
    if (patch_size < sizeof(ptch_header_t)) {
        logv_error("ptch: Patch file %s too small for header", patch_path);
        goto cleanup;
    }

    // Parse header
    ptch_header_t *header = (ptch_header_t *)patch_data;

    // Validate magic
    if (memcmp(header->magic, PTCH_MAGIC, 4) != 0) {
        logv_error("ptch: Invalid magic in %s", patch_path);
        goto cleanup;
    }

    // Validate version
    if (header->version != PTCH_VERSION) {
        logv_error("ptch: Unsupported version %u in %s (expected %d)",
                   header->version, patch_path, PTCH_VERSION);
        goto cleanup;
    }

    // Validate we have enough data for target path
    if (patch_size < sizeof(ptch_header_t) + header->path_len) {
        logv_error("ptch: Patch file %s truncated (missing target path)", patch_path);
        goto cleanup;
    }

    // Extract target path (not null-terminated in file)
    target_path = malloc(header->path_len + 1);
    if (!target_path) {
        log_error("ptch: Failed to allocate memory for target path");
        goto cleanup;
    }
    memcpy(target_path, patch_data + sizeof(ptch_header_t), header->path_len);
    target_path[header->path_len] = '\0';

    // Build full target path
    char full_target_path[512];
    snprintf(full_target_path, sizeof(full_target_path), DATA_PATH "%s", target_path);

    // Check if target file exists
    if (!file_exists(full_target_path)) {
        logv_warn("ptch: Target file %s does not exist, skipping patch", full_target_path);
        goto cleanup;
    }

    // Load target file
    if (!file_load(full_target_path, &target_data, &target_size)) {
        logv_error("ptch: Failed to load target file %s", full_target_path);
        goto cleanup;
    }

    // Validate target size
    if (target_size != header->output_size) {
        logv_error("ptch: Target file size mismatch: expected %llu, got %zu",
                   (unsigned long long)header->output_size, target_size);
        goto cleanup;
    }

    // Apply each record
    uint8_t *record_ptr = patch_data + sizeof(ptch_header_t) + header->path_len;
    uint8_t *patch_end = patch_data + patch_size;

    for (uint32_t i = 0; i < header->record_count; i++) {
        // Check we have enough data for record header
        if (record_ptr + sizeof(ptch_record_t) > patch_end) {
            logv_error("ptch: Patch file %s truncated at record %u", patch_path, i);
            goto cleanup;
        }

        ptch_record_t *record = (ptch_record_t *)record_ptr;
        record_ptr += sizeof(ptch_record_t);

        // Check we have enough data for record data
        if (record_ptr + record->length > patch_end) {
            logv_error("ptch: Patch file %s truncated at record %u data", patch_path, i);
            goto cleanup;
        }

        // Validate offset + length within target bounds
        if (record->offset + record->length > target_size) {
            logv_error("ptch: Record %u out of bounds (offset=%llu, len=%u, target_size=%zu)",
                       i, (unsigned long long)record->offset, record->length, target_size);
            goto cleanup;
        }

        // Apply the patch: copy new bytes to target buffer
        memcpy(target_data + record->offset, record_ptr, record->length);
        record_ptr += record->length;
    }

    // Write modified target back
    if (!file_save(full_target_path, target_data, target_size)) {
        logv_error("ptch: Failed to write patched file %s", full_target_path);
        goto cleanup;
    }

    logv_info("ptch: Applied patch to %s (%u records)", target_path, header->record_count);
    success = true;

cleanup:
    if (patch_data) free(patch_data);
    if (target_data) free(target_data);
    if (target_path) free(target_path);
    return success;
}

/**
 * Apply all patches from a specific directory.
 * Returns number of patches applied.
 */
static int ptch_apply_from_dir(const char *patches_dir) {
    if (!is_dir(patches_dir)) {
        return 0;
    }

    DIR *dir = opendir(patches_dir);
    if (!dir) {
        logv_error("ptch: Failed to open patches directory %s", patches_dir);
        return 0;
    }

    int applied_count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        // Skip non-.ptch files
        if (!string_ends_with(entry->d_name, ".ptch")) {
            continue;
        }

        // Build full patch path
        char patch_path[512];
        snprintf(patch_path, sizeof(patch_path), "%s%s", patches_dir, entry->d_name);

        // Skip directories (shouldn't happen with .ptch extension, but be safe)
        if (is_dir(patch_path)) {
            continue;
        }

        // Check if already applied
        if (ptch_is_applied(patch_path)) {
            logv_debug("ptch: Patch %s already applied, skipping", entry->d_name);
            continue;
        }

        // Apply the patch
        if (ptch_apply(patch_path)) {
            // Mark as applied
            ptch_mark_applied(patch_path);
            applied_count++;
        }
        // If patch fails, don't mark as applied so it retries next run
    }

    closedir(dir);
    return applied_count;
}

int ptch_apply_all(void) {
    int total_applied = 0;

    total_applied += ptch_apply_from_dir(PATCHES_DIR_APP);

    return total_applied;
}
