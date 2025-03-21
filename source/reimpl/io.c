/*
 * reimpl/io.c
 *
 * Wrappers and implementations for some of the IO functions.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "reimpl/io.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <psp2/kernel/threadmgr.h>

//#ifdef USE_SCELIBC_IO
#include <libc_bridge/libc_bridge.h>
//#endif

#include "utils/logger.h"
#include "utils/utils.h"

#include <fios/fios.h>

// Includes the following inline utilities:
// int oflags_newlib_to_oflags_musl(int flags);
// dirent64_bionic * dirent_newlib_to_dirent_bionic(struct dirent* dirent_newlib);
// void stat_newlib_to_stat_bionic(struct stat * src, stat64_bionic * dst);
#include "_struct_converters.c"

extern uint8_t psarc_exists;

FILE *fopen_soloader(char *fname, char *mode) {
    if (strcmp(fname, "/proc/cpuinfo") == 0) {
        return fopen_soloader("app0:/cpuinfo", mode);
    } else if (strcmp(fname, "/proc/meminfo") == 0) {
        return fopen_soloader("app0:/meminfo", mode);
    } else if (strcmp(fname, "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq") == 0) {
        return fopen_soloader("app0:/cpuinfo_max_freq", mode);
    } else if (strcmp(fname, "/sys/devices/system/cpu/present") == 0) {
        return fopen_soloader("app0:/present", mode);
    } else if (strcmp(fname, "/sys/devices/system/cpu/possible") == 0) {
        return fopen_soloader("app0:/possible", mode);
    }

    // this returns stuff like  0x81700010
    FILE* ret = sceLibcBridge_fopen(fname, mode);

    logv_debug("[io] fopen(%s, %s): 0x%x", fname, mode, ret);

    return ret;

}

int retOpen = 0;
int open_soloader(char *_fname, int flags) {
    if (strcmp(_fname, "/proc/cpuinfo") == 0) {
        return open_soloader("app0:/cpuinfo", flags);
    } else if (strcmp(_fname, "/proc/meminfo") == 0) {
        return open_soloader("app0:/meminfo", flags);
    } else if (strcmp(_fname, "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq") == 0) {
        return open_soloader("app0:/cpuinfo_max_freq", flags);
    } else if (strcmp(_fname, "/sys/devices/system/cpu/present") == 0) {
        return open_soloader("app0:/present", flags);
    } else if (strcmp(_fname, "/sys/devices/system/cpu/possible") == 0) {
        return open_soloader("app0:/possible", flags);
    }

    SceFiosFH* handle = 0;
    char real_fname[256];
    if (psarc_exists && !strncmp(_fname, "ux0:data/bgda/assets//res/", 26)) {
        // real name is whatever is after the prefix "ux0:data/bgda/assets//res/", so strip that
        // for example ux0:data/bgda/assets/res/add_texture.lmp should be res/add_texture.lmp
        strcpy(real_fname, _fname + 21);

        //logv_error("res file: %s", real_fname);
        // this returns stuff like 0x1800a
        int res = sceFiosFHOpenSync(NULL, &handle, real_fname, NULL);
        if (res != 0)
        {
            logv_error("res not found inside the PSARC!!! %s\n", real_fname);
            return -1;
        }
        
        // this returns stuff like 0x7fff8000
        int result = sceFiosFilenoToFH(handle);
        //int result = handle;
        logv_error("res file: %s, handle: 0x%x, result: 0x%x", real_fname, handle, result);

        // int size = sceFiosFHGetSize(handle);
        // logv_error("size: %i", size);

        // int fseekRes = sceFiosFHSeek(handle, 0, 2);
        // logv_error("fseekRes: %i", fseekRes);
        return result;

        // if (res < 0) {
        //     logv_error("res not found inside the PSARC!!! %s\n", real_fname);
        // } else {
        //     if (f == NULL) {
        //         logv_error("res not found inside the PSARC!!! %s\n", real_fname);
        //     } else {
        //         logv_debug("res found inside the PSARC!!! %s\n", real_fname);
        //         return res;
        //     }
        // }
    }

    flags = oflags_newlib_to_oflags_musl(flags);
    int ret = open(_fname, flags);
    // if (!strncmp(_fname, "ux0:data/bgda/assets//res/", 26))
    // {
    //     logv_debug("[io] open(%s, %x): %i", _fname, flags, ret);
    //     retOpen = ret;
    // }
    //logv_debug("[io] open(%s, %x): %i", _fname, flags, ret);
    return ret;
}

int fstat_soloader(int fd, void *statbuf) {
    struct stat st;
    int res = fstat(fd, &st);
    if (res == 0)
        stat_newlib_to_stat_bionic(&st, statbuf);

    logv_debug("[io] fstat(fd#%i): %i", fd, res);
    return res;
}

int stat_soloader(char *_pathname, stat64_bionic *statbuf) {
    struct stat st;
    int res = stat(_pathname, &st);

    if (res == 0)
        stat_newlib_to_stat_bionic(&st, statbuf);

    //logv_debug("[io] stat(%s): %i", _pathname, res);
    return res;
}

int fclose_soloader(FILE * f) {
    int ret = sceLibcBridge_fclose(f);

    logv_debug("[io] fclose(0x%x): %i", f, ret);
    return ret;
}

int close_soloader(int fd) {
    uint32_t fiosH = sceFiosFHToFileno(fd);
	if (fiosH == 0xffffffff)
	{
        int ret = close(fd);
       // logv_debug("[io]non-fios close(fd#%i): %i", fd, ret);
        return ret;
    }
    else
    {
        logv_debug("[io] close(fd#0x%x), fiosH=0x%x", fd, fiosH);
        int ret = sceFiosFHCloseSync(NULL, fiosH);
        logv_debug("[io] return close(fd#0x%x): %i", fd, ret);
        return ret;
    }
}

DIR* opendir_soloader(char* _pathname) {
    DIR* ret = opendir(_pathname);
    logv_debug("[io] opendir(\"%s\"): 0x%x", _pathname, ret);
    return ret;
}

struct dirent64_bionic * readdir_soloader(DIR * dir) {
    static struct dirent64_bionic dirent_tmp;

    struct dirent* ret = readdir(dir);
    logv_debug("[io] readdir(%p): %p", dir, ret);

    if (ret) {
        dirent64_bionic* entry_tmp = dirent_newlib_to_dirent_bionic(ret);
        memcpy(&dirent_tmp, entry_tmp, sizeof(dirent64_bionic));
        free(entry_tmp);
        //logv_debug("  [io] readdir(%p): %s", dir, dirent_tmp.d_name);
        return &dirent_tmp;
    }

    return NULL;
}

int readdir_r_soloader(DIR *dirp, dirent64_bionic *entry, dirent64_bionic **result) {
    struct dirent dirent_tmp;
    struct dirent* pdirent_tmp;

    int ret = readdir_r(dirp, &dirent_tmp, &pdirent_tmp);

    if (ret == 0) {
        dirent64_bionic* entry_tmp = dirent_newlib_to_dirent_bionic(&dirent_tmp);
        memcpy(entry, entry_tmp, sizeof(dirent64_bionic));
        *result = (pdirent_tmp != NULL) ? entry : NULL;
        free(entry_tmp);
    }

    log_debug("[io] readdir_r()");
    return ret;
}

int closedir_soloader(DIR* dir) {
    int ret = closedir(dir);
    logv_debug("[io] closedir(0x%x): %i", dir, ret);
    return ret;
}

int fcntl_soloader(int fd, int cmd, ...) {
    logv_debug("[io] fcntl(fd#%i, cmd#%i)", fd, cmd);
    return 0;
}

int fsync_soloader(int fd) {
    int ret = fsync(fd);
    logv_debug("[io] fsync(%i): %i", fd, ret);
    return ret;
}

size_t fread_soloader(void *p, size_t size, size_t num, FILE *f) {
    logv_debug("[io] fread(%p, %i, %i, 0x%x)", p, size, num, f);
	return sceLibcBridge_fread(p, size, num, f);
}

int fstat_hook(int fd, void *statbuf) {
	struct stat st;
	int res = fstat(fd, &st);
	if (res == 0)
		*(uint64_t *)(statbuf + 0x30) = st.st_size;
	return res;
}

int fseek_soloader(FILE *f, int dist, int off) {
    logv_debug("[io] fseek(0x%x, %i, %i)", f, dist, off);
	return sceLibcBridge_fseek(f, dist, off);
}

long ftell_soloader(FILE *f) {
    logv_debug("[io] ftell(0x%x)", f);
	return sceLibcBridge_ftell(f);
}