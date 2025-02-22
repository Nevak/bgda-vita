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
         fatal_error("Error: libshacccg.suprx is not installed. "
                     "Google \"ShaRKBR33D\" for quick installation.");
     }
 
 #ifdef USE_GLSL_SHADERS
     vglSetSemanticBindingMode(VGL_MODE_POSTPONED);
 #endif
 }
 
 void gl_init() {
     vglInitWithCustomThreshold(0, 960, 544, 18 * 1024 * 1024, 0, 20 * 1024 * 1024, 12 * 1024 * 1024, SCE_GXM_MULTISAMPLE_4X);
 }
 
 void gl_swap() {
     vglSwapBuffers(GL_FALSE);
 }
 
 void glShaderSource_soloader(GLuint shader, GLsizei count,
                              const GLchar **string, const GLint *_length) {
 #ifdef DEBUG_OPENGL
     sceClibPrintf("[gl_dbg] glShaderSource<%p>(shader: %i, count: %i, string: %p, length: %p)\n", __builtin_return_address(0), shader, count, string, _length);
 #endif
     if (!string) {
        logv_error("<%p> Shader source string is NULL, count: %i",
                    __builtin_return_address(0), count);
         skip_next_compile = GL_TRUE;
         return;
     } else if (!*string) {
        logv_error("<%p> Shader source *string is NULL, count: %i",
                    __builtin_return_address(0), count);
         skip_next_compile = GL_TRUE;
         return;
     }
 
     size_t total_length = 0;
 
     for (int i = 0; i < count; ++i) {
         if (!_length) {
             total_length += strlen(string[i]);
         } else {
             total_length += _length[i];
         }
     }
 
     char * str = malloc(total_length+1);
     size_t l = 0;
 
     for (int i = 0; i < count; ++i) {
         if (!_length) {
             memcpy(str + l, string[i], strlen(string[i]));
             l += strlen(string[i]);
         } else {
             memcpy(str + l, string[i], _length[i]);
             l += _length[i];
         }
     }
     str[total_length] = '\0';
 
     load_shader(shader, str, total_length);
 
     free(str);
 }
 
 void glCompileShader_soloader(GLuint shader) {
 #ifdef DEBUG_OPENGL
     sceClibPrintf("[gl_dbg] glCompileShader<%p>(shader: %i)\n", __builtin_return_address(0), shader);
 #endif
 
 #ifndef USE_GXP_SHADERS
     if (!skip_next_compile) {
         glCompileShader(shader);
 #ifdef DUMP_COMPILED_SHADERS
         void *bin = vglMalloc(32 * 1024);
         GLsizei len;
         vglGetShaderBinary(shader, 32 * 1024, &len, bin);
         logv_debug("[Thread:%d]next_shader_fname saving GLSL shader to %s", sceKernelGetThreadId(), next_shader_fname);
         file_save(next_shader_fname, bin, len);
         vglFree(bin);
 #endif
     }
     skip_next_compile = GL_FALSE;
 #endif
 }
 
 #if defined(USE_GLSL_SHADERS) && defined(DUMP_COMPILED_SHADERS)
 void load_shader(GLuint shader, const char * string, size_t length) {
     char* sha_name = str_sha1sum(string, length);
 
     char gxp_path[256];
     snprintf(gxp_path, sizeof(gxp_path), DATA_PATH"gxp/%s.gxp", sha_name);
 
     if (file_exists(gxp_path)) {
         uint8_t *buffer;
         size_t size;
 
         file_load(gxp_path, &buffer, &size);
 
         glShaderBinary(1, &shader, 0, buffer, (int32_t) size);
 
         free(buffer);
         skip_next_compile = GL_TRUE;
     } else {
         glShaderSource(shader, 1, &string, &length);
         strcpy(next_shader_fname, gxp_path);
     }
 
     free(sha_name);
 }
 #elif defined(USE_GLSL_SHADERS)
 void load_shader(GLuint shader, const char * string, size_t length) {
     glShaderSource(shader, 1, &string, &length);
 }
 #elif defined(USE_CG_SHADERS) && defined(DUMP_COMPILED_SHADERS)
 void load_shader(GLuint shader, const char * string, size_t length) {
     char* sha_name = str_sha1sum(string, length);
 
     char gxp_path[256];
     char cg_path[256];
     snprintf(gxp_path, sizeof(gxp_path), DATA_PATH"gxp/%s.gxp", sha_name);
     snprintf(cg_path, sizeof(cg_path), DATA_PATH"cg/%s.cg", sha_name);
 
    logv_debug("[Thread:%d]USE_CG_SHADERS loading shader %s, %s", sceKernelGetThreadId(), gxp_path, cg_path);

     if (file_exists(gxp_path)) {
         uint8_t *buffer;
         size_t size;
 
         file_load(gxp_path, &buffer, &size);
 
         glShaderBinary(1, &shader, 0, buffer, (int32_t) size);
 
         free(buffer);
         skip_next_compile = GL_TRUE;
     } else if (file_exists(cg_path)) {
         char *buffer;
         size_t size;
 
         file_load(cg_path, (uint8_t **) &buffer, &size);
         //logv_debug("[Thread:%d] calling glShaderSource with source %s", sceKernelGetThreadId(), buffer);
         glShaderSource(shader, 1, &buffer, &size);
         strcpy(next_shader_fname, gxp_path);
 
         free(buffer);
         skip_next_compile = GL_FALSE;
     } else {
         logv_warn("Encountered an untranslated shader %s, saving GLSL "
                "and using a dummy shader.", sha_name);
 
         char glsl_path[256];
         snprintf(glsl_path, sizeof(glsl_path), DATA_PATH"glsl/%s.glsl", sha_name);
         file_mkpath(glsl_path, 0777);
         logv_debug("[Thread:%d] untranslated saving GLSL shader to %s", sceKernelGetThreadId(), glsl_path);
         file_save(glsl_path, (const uint8_t *) string, length);
 
         if (strstr(string, "gl_FragColor")) {
             const char *dummy_shader = "float4 main() { return float4(1.0,1.0,1.0,1.0); }";
             int32_t dummy_shader_len = (int32_t) strlen(dummy_shader);
             glShaderSource(shader, 1, &dummy_shader, &dummy_shader_len);
         } else {
             const char *dummy_shader = "void main(float4 out gl_Position : POSITION ) { gl_Position = float4(1.0,1.0,1.0,1.0); }";
             int32_t dummy_shader_len = (int32_t) strlen(dummy_shader);
             glShaderSource(shader, 1, &dummy_shader, &dummy_shader_len);
         }
 
         skip_next_compile = GL_FALSE;
     }
 
     free(sha_name);
 }
 #elif defined(USE_CG_SHADERS) || defined(USE_GXP_SHADERS)
 void load_shader(GLuint shader, const char * string, size_t length) {
     char* sha_name;
     if (strncmp(string, "sha:", 4) == 0) {
         sha_name = malloc(41);
         strncpy(sha_name, string+4, 40);
         sha_name[40] = '\0';
         logv_warn("will load faked shader using sha1 \"%s\"", sha_name);
     } else {
         sha_name = str_sha1sum((uint8_t*)string, length);
         strncpy(known_shaders[known_shaders_count - 1].real_name, sha_name, 40);
         known_shaders[known_shaders_count - 1].real_name[40] = '\0';
         logv_warn("saved faked shader sha1 \"%s\" under id %i", sha_name, known_shaders_count - 1);
     }
 
     char path[256];
 #ifdef USE_CG_SHADERS
     snprintf(path, sizeof(path), DATA_PATH"cg/%s.cg", sha_name);
 #else
     snprintf(path, sizeof(path), "app0:gxp/%s.gxp", sha_name);
 #endif
 
     if (file_exists(path)) {
 #ifdef USE_CG_SHADERS
         char *buffer;
         size_t size;
 
         file_load(path, (uint8_t **) &buffer, &size);
 
         glShaderSource(shader, 1, &string, &size);
 
         free(buffer);
 #else
         uint8_t *buffer;
         size_t size;
 
         file_load(path, &buffer, &size);
 
         glShaderBinary(1, &shader, 0, buffer, (int32_t) size);
 
         free(buffer);
 #endif
     } else {
        logv_warn("Encountered an untranslated shader %s, saving GLSL "
                "and using a dummy shader.", sha_name);
 
         char glsl_path[256];
         snprintf(glsl_path, sizeof(glsl_path), DATA_PATH"glsl/%s.glsl", sha_name);
         file_mkpath(glsl_path, 0777);
         logv_debug("[Thread:%d]USE_CG_SHADERS untranslated saving GLSL shader to %s", sceKernelGetThreadId(), glsl_path);
         file_save(glsl_path, (const uint8_t *) string, length);
 
         if (strstr(string, "gl_FragColor")) {
             const char *dummy_shader = "float4 main() { return float4(1.0,1.0,1.0,1.0); }";
             int32_t dummy_shader_len = (int32_t) strlen(dummy_shader);
             glShaderSource(shader, 1, &dummy_shader, &dummy_shader_len);
         } else {
             const char *dummy_shader = "void main(float4 out gl_Position : POSITION ) { gl_Position = float4(1.0,1.0,1.0,1.0); }";
             int32_t dummy_shader_len = (int32_t) strlen(dummy_shader);
             glShaderSource(shader, 1, &dummy_shader, &dummy_shader_len);
         }
     }
 
     free(sha_name);
 }
 #else
 #error "Define one of (USE_GLSL_SHADERS, USE_CG_SHADERS, USE_GXP_SHADERS)"
 #endif










 // EGL STUFF. TOOD: Move to another file

 EGLBoolean eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor) {
     logv_debug("eglInitialize(0x%x)", (int)dpy);
 
     gl_init();
 
     if (major) *major = 2;
     if (minor) *minor = 2;
 
     return EGL_TRUE;
 }
 
 EGLBoolean eglQuerySurface(EGLDisplay dpy, EGLSurface eglSurface, EGLint attribute, EGLint *value)
 {
     EGLBoolean ret = EGL_TRUE;
     switch (attribute) {
         case EGL_CONFIG_ID:
             ret = 1;
             break;
         case EGL_WIDTH:
             *value = 960;
             break;
         case EGL_HEIGHT:
             *value = 544;
             break;
         case EGL_TEXTURE_FORMAT:
             *value = 2; // NoTexture = 0, RGB = 1, RGBA = 2
             break;
         case EGL_TEXTURE_TARGET:
             *value = 1;
             break;
         case EGL_SWAP_BEHAVIOR:
             ret = EGL_TRUE;
             *value = EGL_BUFFER_PRESERVED;
             break;
         case EGL_LARGEST_PBUFFER:
         case EGL_MIPMAP_TEXTURE:
             *value = EGL_FALSE;
             break;
         case EGL_MIPMAP_LEVEL:
             *value = 0;
             break;
         case EGL_MULTISAMPLE_RESOLVE:
             // ignored when creating the surface, return default
             *value = EGL_MULTISAMPLE_RESOLVE_DEFAULT;
             break;
         case EGL_HORIZONTAL_RESOLUTION:
         case EGL_VERTICAL_RESOLUTION:
             *value = 220 * EGL_DISPLAY_SCALING; // VITA DPI is 220
             break;
         case EGL_PIXEL_ASPECT_RATIO:
             // Please don't ask why * EGL_DISPLAY_SCALING, the document says it
             *value = 960 / 544 * EGL_DISPLAY_SCALING;
             break;
         case EGL_RENDER_BUFFER:
             *value = EGL_BACK_BUFFER;
             break;
         case EGL_VG_COLORSPACE:
             // ignored when creating the surface, return default
             *value = EGL_VG_COLORSPACE_sRGB;
             break;
         case EGL_VG_ALPHA_FORMAT:
             // ignored when creating the surface, return default
             *value = EGL_VG_ALPHA_FORMAT_NONPRE;
             break;
         case EGL_TIMESTAMPS_ANDROID:
             *value = EGL_FALSE;
             break;
         default:
             logv_error("eglQuerySurface %x  EGL_BAD_ATTRIBUTE", attribute);
             break;
     }
 
     return ret;
 }
 
 EGLBoolean eglGetConfigs( EGLDisplay display,
                               EGLConfig * configs,
                               EGLint config_size,
                               EGLint * num_config) {
     *num_config = 1;
     return EGL_TRUE;
 }
 
 EGLBoolean eglGetConfigAttrib(EGLDisplay display,
                               EGLConfig config,
                               EGLint attribute,
                               EGLint * value) {
     switch (attribute) {
         case EGL_ALPHA_SIZE: {
             *value = 8;
             break;
         }
         case EGL_ALPHA_MASK_SIZE: {
             *value = 8;
             break;
         }
         case EGL_BIND_TO_TEXTURE_RGB: {
             *value = EGL_TRUE;
             break;
         }
         case EGL_BIND_TO_TEXTURE_RGBA: {
             *value = EGL_TRUE;
             break;
         }
         case EGL_BLUE_SIZE: {
             *value = 8;
             break;
         }
         case EGL_BUFFER_SIZE: {
             *value = 32;
             break;
         }
         case EGL_COLOR_BUFFER_TYPE: {
             *value = EGL_RGB_BUFFER;
             break;
         }
         case EGL_CONFIG_CAVEAT: {
             *value = EGL_NONE;
             break;
         }
         case EGL_CONFIG_ID: {
             *value = 1;
             break;
         }
         case EGL_CONFORMANT: {
             *value = 0;
             break;
         }
         case EGL_DEPTH_SIZE: {
             *value = 0;
             break;
         }
         case EGL_GREEN_SIZE: {
             *value = 0;
             break;
         }
         case EGL_LEVEL: {
             *value = 0;
             break;
         }
         case EGL_LUMINANCE_SIZE: {
             *value = 0;
             break;
         }
         case EGL_MAX_PBUFFER_WIDTH: {
             *value = 0;
             break;
         }
         case EGL_MAX_PBUFFER_HEIGHT: {
             *value = 0;
             break;
         }
         case EGL_MAX_PBUFFER_PIXELS: {
             *value = 0;
             break;
         }
         case EGL_MAX_SWAP_INTERVAL: {
             *value = 0;
             break;
         }
         case EGL_MIN_SWAP_INTERVAL: {
             *value = 0;
             break;
         }
         case EGL_NATIVE_RENDERABLE: {
             *value = 0;
             break;
         }
         case EGL_NATIVE_VISUAL_ID: {
             *value = 0;
             break;
         }
         case EGL_NATIVE_VISUAL_TYPE: {
             *value = 0;
             break;
         }
         case EGL_RED_SIZE: {
             *value = 0;
             break;
         }
         case EGL_RENDERABLE_TYPE: {
             *value = 0;
             break;
         }
         case EGL_SAMPLE_BUFFERS: {
             *value = 0;
             break;
         }
         case EGL_SAMPLES: {
             *value = 0;
             break;
         }
         case EGL_STENCIL_SIZE: {
             *value = 0;
             break;
         }
         case EGL_SURFACE_TYPE: {
             *value = 0;
             break;
         }
         case EGL_TRANSPARENT_TYPE: {
             *value = 0;
             break;
         }
         case EGL_TRANSPARENT_RED_VALUE: {
             *value = 0;
             break;
         }
         case EGL_TRANSPARENT_GREEN_VALUE: {
             *value = 0;
             break;
         }
         case EGL_TRANSPARENT_BLUE_VALUE: {
             *value = 0;
             break;
         }
         default:
             return EGL_FALSE;
     }
     return EGL_TRUE;
 }
 
 EGLBoolean eglChooseConfig (EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
     if (!num_config) {
         return EGL_BAD_PARAMETER;
     }
 
     if (!configs) {
         *num_config = 1;
         return EGL_TRUE;
     }
 
     *configs = strdup("1");
     *num_config = 1;
 
     return EGL_TRUE;
 }
 
 EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list) {
     return strdup("ctx");
 }

 EGLContext eglGetCurrentContext (void) {
    return strdup("ctx");
}
 
 EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, void * win, const EGLint *attrib_list) {
     return strdup("surface");
 }
 
 EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) {
     return EGL_TRUE;
 }
 
 EGLBoolean eglDestroyContext (EGLDisplay dpy, EGLContext ctx) {
     if (ctx) free(ctx);
     return EGL_TRUE;
 }
 
 EGLBoolean eglDestroySurface (EGLDisplay dpy, EGLSurface surface) {
     if (surface) free(surface);
     return EGL_TRUE;
 }
 
 EGLBoolean eglTerminate(EGLDisplay dpy) {
     return EGL_TRUE;
 }