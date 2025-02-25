/*
 * utils/utils.c
 *
 * Common helper utilities.
 *
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/utils.h"
#include "logger.h"

#include <psp2/io/stat.h>
#include <psp2/ctrl.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <falso_jni/FalsoJNI.h>
#include <sha1/sha1.h>

#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"


uint64_t current_timestamp_ms() {
    struct timeval te;
    gettimeofday(&te, NULL);
    return (te.tv_sec * 1000LL + te.tv_usec / 1000);
}

bool file_copy(const char * path, const char * destination) {
    if (!file_exists(path)) {
        logv_error("file_copy: Specified source path \"%s\" "
                "does not exist.", path);
        return false;
    }

    if (!file_mkpath(destination, 0755)) {
        logv_error("file_copy: Could not create parent directories for the "
                "specified destination path \"%s\".", destination);
        return false;
    }

    uint8_t * buffer;
    size_t    size;

    if (!file_load(path, &buffer, &size)) {
        logv_error("file_copy: Failed to read data from "
                "the source path \"%s\".", path);
        return false;
    }

    if (!file_save(destination, buffer, size)) {
        logv_error("file_copy: Failed to write data to the specified "
                "destination path \"%s\".", destination);
        return false;
    }

    free(buffer);
    return true;
}

bool file_exists(const char * path) {
    SceIoStat stat;
    return sceIoGetstat(path, &stat) >= 0;
}

bool file_load(const char * path, uint8_t ** buffer, size_t * size) {
    //logv_debug("file_load: Loading data from \"%s\".", path);
    if (!buffer || !size) {
        log_error("file_load: Invalid argument(s).");
        return false;
    }

    if (!file_exists(path)) {
        logv_error("file_load: Specified source path \"%s\" "
                "does not exist.", path);
        return false;
    }

#ifdef USE_SCELIBC_IO
    FILE * f = sceLibcBridge_fopen(path, "rb");
#else
    FILE * f = fopen(path, "rb");
#endif

    if (!f) {
        logv_error("file_load: Could not open the specified "
                "source path \"%s\".", path);
        return false;
    }

#ifdef USE_SCELIBC_IO
    sceLibcBridge_fseek(f, 0, SEEK_END);
    *size = sceLibcBridge_ftell(f);
    sceLibcBridge_fseek(f, 0, SEEK_SET);
#else
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
#endif

    if (*size <= 0) {
        logv_error("file_load: The specified source file \"%s\" is empty.", path);
    #ifdef USE_SCELIBC_IO
        sceLibcBridge_fclose(f);
    #else
        fclose(f);
    #endif
        return false;
    }

    *buffer = malloc(*size);

    if (!*buffer) {
        logv_error("file_load: Unable to allocate %d bytes of memory to load "
                "the specified source file \"%s\".", path);
    #ifdef USE_SCELIBC_IO
        sceLibcBridge_fclose(f);
    #else
        fclose(f);
    #endif
        return false;
    }

#ifdef USE_SCELIBC_IO
    sceLibcBridge_fread(*buffer, 1, *size, f);
    sceLibcBridge_fclose(f);
#else
    fread(*buffer, 1, *size, f);
    fclose(f);
#endif

    return true;
}

bool file_mkpath(const char * path, mode_t mode) {
    logv_debug("file_mkpath: Creating directories leading to \"%s\".", path);
    if (!path || !*path) {
        log_error("file_mkpath: Invalid argument.");
        return false;
    }

    char * file_path = strdup(path);
    for (char* p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (mkdir(file_path, mode) == -1) {
            if (errno != EEXIST) {
                logv_error("file_mkpath: Unable to create a directory \"%s\", "
                        "mkdir error code is %s.", file_path, strerror(errno));
                free(file_path);
                return false;
            }
        }
        *p = '/';
    }

    free(file_path);
    return true;
}

bool file_save(const char * path, const uint8_t * buffer, size_t size) {
    logv_debug("file_save: Saving %d bytes to \"%s\".", size, path);
#ifdef USE_SCELIBC_IO
    FILE * f = sceLibcBridge_fopen(path, "wb");
#else
    FILE * f = fopen(path, "wb");
#endif

    if (!f) {
        logv_error("[Thread:%d]file_save: Could not open the specified "
                "target path \"%s\".",sceKernelGetThreadId(), path);
        return false;
    }

#ifdef USE_SCELIBC_IO
    sceLibcBridge_fwrite(buffer, size, 1, f);
    sceLibcBridge_fclose(f);
#else
    fwrite(buffer, size, 1, f);
    fclose(f);
#endif

    return true;
}

size_t file_size(const char * path) {
    size_t ret = -1;

    if (!file_exists(path)) {
        logv_error("size_t: Specified source path \"%s\" "
                "does not exist.", path);
        return ret;
    }

#ifdef USE_SCELIBC_IO
    FILE * f = sceLibcBridge_fopen(path, "rb");
#else
    FILE * f = fopen(path, "rb");
#endif

    if (!f) {
        logv_error("size_t: Could not open the specified "
                "source path \"%s\".", path);
        return ret;
    }

#ifdef USE_SCELIBC_IO
    sceLibcBridge_fseek(f, 0, SEEK_END);
    ret = sceLibcBridge_ftell(f);
    sceLibcBridge_fseek(f, 0, SEEK_SET);
    sceLibcBridge_fclose(f);
#else
    fseek(f, 0, SEEK_END);
    ret = ftell(f);
    fseek(f, 0, SEEK_SET);
    fclose(f);
#endif

    return ret;
}

char * file_sha1sum(const char * path) {
    uint8_t * buffer;
    size_t    size;

    if (!file_load(path, &buffer, &size)) {
        logv_error("file_sha1sum: Failed to read data from "
                "the source path \"%s\".", path);
        return NULL;
    }

    char * ret = str_sha1sum((const char *) buffer, size);

    free(buffer);
    return ret;
}

bool is_dir(const char * path) {
    DIR* d = opendir(path);

    if (d != NULL) {
        closedir(d);
        return true;
    }

    return false;
}



int ret0(void) {
    return 0;
}

int ret1(void) {
    return 1;
}

int retminus1(void) {
    return -1;
}

SceUID _vshKernelSearchModuleByName(const char *, int *);

bool module_loaded(const char * name) {
    int search_unk[2];
    return _vshKernelSearchModuleByName(name, search_unk) >= 0;
}

bool string_ends_with(const char * str, const char * suffix) {
    int str_len = (int)strlen(str);
    int suffix_len = (int)strlen(suffix);

    return
            (str_len >= suffix_len) &&
            (0 == strcmp(str + (str_len-suffix_len), suffix));
}

void str_remove(char *str, const char *sub) {
    char *p, *q, *r;

    if (*sub && (q = r = strstr(str, sub)) != NULL) {
        size_t len = strlen(sub);
        while ((r = strstr(p = r + len, sub)) != NULL) {
            while (p < r)
                *q++ = *p++;
        }
        while ((*q++ = *p++) != '\0')
            continue;
    }
}

char * str_sha1sum(const char * str, size_t size) {
    if (size == 0) {
        size = strlen(str);
    }

    uint8_t sha1[20];
    SHA1_CTX ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, (uint8_t *)str, size);
    sha1_final(&ctx, (uint8_t *)sha1);

    char hash[42];
    memset(hash, 0, sizeof(hash));

    for (int i = 0; i < 20; i++) {
        char string[4];
        sprintf(string, "%02X", sha1[i]);
        strcat(hash, string);
    }

    hash[41] = '\0';
    return strdup(hash);
}

void str_replace(char *target, const char *needle, const char *replacement) {
    char buffer[1024] = { 0 };
    char *insert_point = &buffer[0];
    const char *tmp = target;
    size_t needle_len = strlen(needle);
    size_t repl_len = strlen(replacement);

    while (1) {
        const char *p = strstr(tmp, needle);

        if (p == NULL) {
            strcpy(insert_point, tmp);
            break;
        }

        memcpy(insert_point, tmp, p - tmp);
        insert_point += p - tmp;

        memcpy(insert_point, replacement, repl_len);
        insert_point += repl_len;

        tmp = p + needle_len;
    }

    strcpy(target, buffer);
}

void str_prepend(char* s, const char* t) {
    size_t len = strlen(t);
    memmove(s + len, s, strlen(s) + 1);
    memcpy(s, t, len);
}
