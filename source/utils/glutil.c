/*
 * utils/glutil.c
 *
 * OpenGL API initializer, related functions.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/glutil.h" 

#include "utils/utils.h"
#include "utils/dialog.h"
#include "utils/logger.h"
#include "utils/trophies.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <vitasdk.h>
 


 // Helpers for our handling of shaders
 GLboolean skip_next_compile = GL_FALSE;
 char next_shader_fname[256];
 void load_shader(GLuint shader, const char * string, size_t length);
 
 extern known_shaders_struct known_shaders[256];
 extern int known_shaders_count;
 
 void gl_preload() {
     if (!file_exists("ur0:/data/libshacccg.suprx")
         && !file_exists("ur0:/data/external/libshacccg.suprx")) {
         fatal_error("Error:  libshacccg.suprx is not installed. "
                     "Google \"ShaRKBR33D\" for quick installation.");
     }
 
 #ifdef USE_GLSL_SHADERS
     vglSetSemanticBindingMode(VGL_MODE_POSTPONED);
 #endif
 }
 
 void gl_init() {
    vglSetVertexPoolSize(16 * 1024 * 1024);
    vglSetParamBufferSize(8 * 1024 * 1024);
    vglUseTripleBuffering(GL_FALSE);
    //vglSetSemanticBindingMode(VGL_MODE_POSTPONED);
    //vglInitWithCustomThreshold(0, 960, 544, 18 * 1024 * 1024, 0, 20 * 1024 * 1024, 0, SCE_GXM_MULTISAMPLE_NONE);
    vglInitWithCustomThreshold(0, SCREEN_W, SCREEN_H, MEMORY_VITAGL_THRESHOLD_MB * 1024 * 1024, 0, 0, 0, SCE_GXM_MULTISAMPLE_2X);
    //eglSwapInterval(0, 2);
 }
 

 