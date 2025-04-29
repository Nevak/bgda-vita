/*
 * utils/utils.h
 *
 * Common helper utilities.
 *
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef SOLOADER_UTILS_H
#define SOLOADER_UTILS_H

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>

#define SCREEN_W 960  
#define SCREEN_H 544
#define MEMORY_VITAGL_THRESHOLD_MB 16

/**
 * Get Unix timestamp in milliseconds.
 *
 * @return Number of milliseconds that have elapsed since January 1, 1970.
 */
uint64_t current_timestamp_ms();

/**
 * Create a copy of a file.
 *
 * If the file specified by `destination` already exists, it will be
 * overwritten. If a parent directory or directories of the file specified by
 * `destination` do not exist, they will be created automatically.
 *
 * @warning The function will fail if the size of the source file specified by
 *          `path` exceeds the amount of free memory available.
 *
 * @param[in] path        Full path of the source file.
 * @param[in] destination Full path of the destination file.
 *
 * @return `true` on success, `false` otherwise.
 */
bool file_copy(const char * path, const char * destination);

/**
 * Check whether a file exists.
 *
 * @param path Full path of the file to look for.
 *
 * @return `true` if file exists, `false` otherwise.
 */
bool file_exists(const char * path);

/**
 * Load file contents into memory.
 *
 * @param[in]  path   Full path of the source file.
 * @param[out] buffer Output buffer address, allocated by the function. Must be
 *                    freed by the caller if the function returns `true`.
 * @param[out] size   Output buffer size.
 *
 * @return `true` on success, `false` otherwise.
 */
bool file_load(const char * path, uint8_t ** buffer, size_t * size);

/**
 * Create directories leading to file.
 *
 * @param[in] path Full path of the target file.
 * @param[in] mode Permissions to set for new directories (if any).
 *
 * @return `true` on success, `false` otherwise.
 */
bool file_mkpath(const char * path, mode_t mode);

/**
 * Save buffer contents into a file.
 *
 * @param[in] path   Full path of the target file.
 * @param[in] buffer Buffer containing data to save.
 * @param[in] size   Size of the buffer (in bytes).
 *
 * @return `true` on success, `false` otherwise.
 */
bool file_save(const char * path, const uint8_t * buffer, size_t size);

/**
 * Get the size of a file in bytes
 *
 * @param[in] path Full path of the target file.
 *
 * @return File size in bytes or (size_t)-1 in case of a failure.
 */
size_t file_size(const char * path);

/**
 * Get SHA1 hash of file contents.
 *
 * @param[in] path Full path of the source file.
 *
 * @return 40-char long null-terminated string containing SHA1 hash. Can be
 *         NULL in case of an error. Must be freed by the caller.
 */
char * file_sha1sum(const char * path);

/**
 * Check whether specified path is a directory.
 *
 * @param[in] path Target path.
 *
 * @return `true` if path is a directory, `false` otherwise.
 */
bool is_dir(const char * path);




int ret0(void);

__attribute__((unused)) int ret1(void);

int retminus1(void);


bool module_loaded(const char * name);

bool string_ends_with(const char * str, const char * suffix);

/* Prepends t into s. Assumes s has enough space allocated
** for the combined string.
*/
void str_prepend(char* s, const char* t);

__attribute__((unused)) inline int string_starts_with(const char *pre,
                                                      const char *str) {
    char cp;
    char cs;

    if (!*pre)
        return 1;

    while ((cp = *pre++) && (cs = *str++))
    {
        if (cp != cs)
            return 0;
    }

    if (!cs)
        return 0;

    return 1;
}

uint64_t current_timestamp_ms();

void str_remove(char *str, const char *sub);

void str_replace(char *target, const char *needle, const char *replacement);

/**
 * Get SHA1 hash of a string or byte array.
 *
 * @param[in] str  Source string or byte array.
 * @param[in] size Length of the source string or byte array. If `0` is
 *                 specified, `str` is treated as a null-terminated string.
 *
 * @return 40-char long null-terminated string containing SHA1 hash. Can be
 *         NULL in case of an error. Must be freed by the caller.
 */
char * str_sha1sum(const char * str, size_t size);


typedef struct known_shaders_struct {
    char engine_name[42];
    char real_name[42];
} known_shaders_struct;

#endif // SOLOADER_UTILS_H
