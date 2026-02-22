#ifndef SHADER_PATCH_H
#define SHADER_PATCH_H

#include "utils/utils.h"
#include "utils/logger.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define FLAT_SHADER_PATH DATA_PATH "assets/flat.xvu"
#define FLAT_SHADER_BACKUP FLAT_SHADER_PATH ".backup"

/**
 * Apply UV scaling patch to flat.xvu shader.
 *
 * This patches the flat.xvu vertex shader to support UV coordinate scaling,
 * which is needed for non-power-of-two video textures.
 *
 * The patch adds:
 * - #define uvScale C(1) to access shader constant register 1
 * - oT0.x *= uvScale.x; to scale the U texture coordinate
 *
 * A backup of the original shader is created on first run.
 */
static bool patch_flat_shader(void) {
    // Skip if already patched (backup exists)
    if (file_exists(FLAT_SHADER_BACKUP)) {
        log_debug("flat.xvu already patched (backup exists)");
        return true;
    }

    // Check if shader exists
    if (!file_exists(FLAT_SHADER_PATH)) {
        log_warn("flat.xvu not found, skipping patch");
        return false;
    }

    // Load original shader
    uint8_t *data;
    size_t size;
    if (!file_load(FLAT_SHADER_PATH, &data, &size)) {
        log_error("Failed to load flat.xvu");
        return false;
    }

    // Create backup before patching
    if (!file_copy(FLAT_SHADER_PATH, FLAT_SHADER_BACKUP)) {
        log_error("Failed to create flat.xvu backup");
        free(data);
        return false;
    }

    // Detect line ending style (CRLF or LF)
    const char *newline = strstr((char*)data, "\r\n") ? "\r\n" : "\n";
    size_t nl_len = strlen(newline);

    // Build anchors and insertions based on detected line ending
    char anchor1[64], anchor2[32];
    snprintf(anchor1, sizeof(anchor1), "#define D3D_CONST_PRECISION highp%s", newline);
    snprintf(anchor2, sizeof(anchor2), "oT0 = v1;%s", newline);

    char insert1[128], insert2[64];
    snprintf(insert1, sizeof(insert1), "%s#define uvScale C(1)%s%s#define D3D_START_CONST 0%s#define D3D_END_CONST 2%s",
             newline, newline, newline, newline, newline);
    snprintf(insert2, sizeof(insert2), "\toT0.x *= uvScale.x;%s", newline);

    size_t insert1_len = strlen(insert1);
    size_t insert2_len = strlen(insert2);
    size_t new_size = size + insert1_len + insert2_len;

    char *patched = malloc(new_size + 1);
    if (!patched) {
        log_error("Failed to allocate memory for patched shader");
        free(data);
        return false;
    }

    char *src = (char*)data;
    char *dst = patched;

    // Find and copy up to anchor1, then insert patch1
    char *pos1 = strstr(src, anchor1);
    if (pos1) {
        pos1 += strlen(anchor1);  // Move past anchor
        size_t len = pos1 - src;
        memcpy(dst, src, len);
        dst += len;
        memcpy(dst, insert1, insert1_len);
        dst += insert1_len;
        src = pos1;
    } else {
        log_warn("flat.xvu: anchor1 not found, shader may have unexpected format");
    }

    // Find and copy up to anchor2, then insert patch2
    char *pos2 = strstr(src, anchor2);
    if (pos2) {
        pos2 += strlen(anchor2);
        size_t len = pos2 - src;
        memcpy(dst, src, len);
        dst += len;
        memcpy(dst, insert2, insert2_len);
        dst += insert2_len;
        src = pos2;
    } else {
        log_warn("flat.xvu: anchor2 not found, shader may have unexpected format");
    }

    // Copy remainder of the file
    size_t remaining = size - (src - (char*)data);
    memcpy(dst, src, remaining);
    dst += remaining;
    *dst = '\0';

    // Save patched shader
    bool ok = file_save(FLAT_SHADER_PATH, (uint8_t*)patched, dst - patched);

    free(data);
    free(patched);

    if (ok) {
        log_info("flat.xvu patched successfully for UV scaling");
    } else {
        log_error("Failed to save patched flat.xvu");
    }
    return ok;
}

#endif
