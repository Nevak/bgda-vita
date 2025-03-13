/*
 * utils/glutil.h
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

 #ifndef SOLOADER_GLUTIL_H
 #define SOLOADER_GLUTIL_H
 
 #include <vitaGL.h>
 
//#define USE_CG_SHADERS
#define USE_GLSL_SHADERS
//#define USE_GXP_SHADERS
//#define DUMP_COMPILED_SHADERS
//#define DEBUG_OPENGL

 #ifdef __cplusplus
 extern "C" {
 #endif
 
 void gl_init();
 
 void gl_preload();
 
 #ifdef __cplusplus
 };
 #endif
 
 #endif // SOLOADER_GLUTIL_H