/*
 * utils/init.c
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021-2022 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/init.h"

#include "utils/dialog.h"
#include "utils/glutil.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/settings.h"
#include "utils/ogg_patch.h"
#include "utils/vorbis_patch.h"
#include "utils/ffmpeg_patch.h"
#include "utils/ptch.h"

#include "dynlib.h"
#include "patch.h"

#include <string.h>

#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/kernel/clib.h>
#include <psp2/power.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>
#include <fios/fios.h>

// Base address for the Android .so to be loaded at
#define LOAD_ADDRESS 0x98000000

extern so_module so_mod;
extern so_module so_mod_libcpufeatues;
extern so_module so_mod_jbejni;
extern so_module so_mod_libcpp;
extern so_module so_mod_libxmv;


void soloader_init_all() {
    // Apply binary patches before loading any files
    int patches_applied = ptch_apply_all();
    if (patches_applied > 0) {
        logv_info("Applied %d binary patch(es)", patches_applied);
    } else if (patches_applied < 0) {
        log_error("Failed to apply binary patches");
    }

    // Set default overclock values
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

#ifdef USE_SCELIBC_IO
    fios_init();
    log_info("fios init passed.");
#endif

    if (!module_loaded("kubridge"))
        fatal_error("Error: kubridge.skprx is not installed.");
    log_info("kubridge check passed.");

    if (!file_exists(SO_PATH)) {
        fatal_error("Looks like you haven't installed the data files for this "
                    "port, or they are in an incorrect location. Please make "
                    "sure that you have %s file exactly at that path.", SO_PATH);
    }

    // 10 KB
    if (so_file_load(&so_mod_libcpufeatues, SO_PATH_LIBCPUFEATURES, LOAD_ADDRESS) < 0)
        fatal_error("Error: could not load %s.", SO_PATH_LIBCPUFEATURES);
    so_relocate(&so_mod_libcpufeatues);
    logv_info("Resolving imports for %s", SO_PATH_LIBCPUFEATURES);
    resolve_imports(&so_mod_libcpufeatues);
    so_flush_caches(&so_mod_libcpufeatues);
    so_initialize(&so_mod_libcpufeatues);
    logv_info("%s loaded successfully.", SO_PATH_LIBCPUFEATURES);

    // 38 KB
    // 0x20000 in decimal is 131072
    if (so_file_load(&so_mod_jbejni, SO_PATH_JBEJNI, LOAD_ADDRESS + 0x15000) < 0)
        fatal_error("Error: could not load %s.", SO_PATH_JBEJNI);
    so_relocate(&so_mod_jbejni);
    logv_info("Resolving imports for %s", SO_PATH_JBEJNI);
    resolve_imports(&so_mod_jbejni);
    so_flush_caches(&so_mod_jbejni);
    so_initialize(&so_mod_jbejni);
    logv_info("%s loaded successfully.", SO_PATH_JBEJNI);

    // 592 KB
    if (so_file_load(&so_mod_libcpp, SO_PATH_LIBCPP, LOAD_ADDRESS + 0x35000) < 0)
        fatal_error("Error: could not load %s.", SO_PATH_LIBCPP);
    so_relocate(&so_mod_libcpp);
    logv_info("Resolving imports for %s", SO_PATH_LIBCPP);
    resolve_imports(&so_mod_libcpp);
    so_flush_caches(&so_mod_libcpp);
    so_initialize(&so_mod_libcpp);
    logv_info("%s loaded successfully.", SO_PATH_LIBCPP);

    // 1385 KB
    if (so_file_load(&so_mod_libxmv, SO_PATH_LIBXMV, LOAD_ADDRESS + 0xE0000) < 0)
        fatal_error("Error: could not load %s.", SO_PATH_LIBXMV);
    so_relocate(&so_mod_libxmv);
    logv_info("Resolving imports for %s", SO_PATH_LIBXMV);
    resolve_imports(&so_mod_libxmv);
    so_flush_caches(&so_mod_libxmv);
    so_initialize(&so_mod_libxmv);
    logv_info("%s loaded successfully.", SO_PATH_LIBXMV);

    // 2055 KB
    if (so_file_load(&so_mod, SO_PATH, LOAD_ADDRESS + 0x400000) < 0)
        fatal_error("Error: could not load %s.", SO_PATH);

    settings_load();
    log_info("settings_load() passed.");

    so_relocate(&so_mod);
    log_info("so_relocate() passed.");

    resolve_imports(&so_mod);
    log_info("so_resolve() passed.");

    so_patch();
    //patch_ogg();
    patch_vorbis();
    //patch_texture_decom();
    //patch_ffmpeg();
    log_info("so_patch() passed.");

    so_flush_caches(&so_mod);
    log_info("so_flush_caches() passed.");

    so_initialize(&so_mod);
    log_info("so_initialize() passed.");



    gl_preload();
    log_info("gl_preload() passed.");

    jni_init();
    log_info("jni_init() passed.");
}
