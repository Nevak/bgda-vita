/*
 * dynlib.c
 *
 * Resolving dynamic imports of the .so.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

// Disable IDE complaints about _identifiers and global interfaces
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "cppcoreguidelines-interfaces-global-init"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

// Suppress `mktemp` deprecation warning
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#define _POSIX_TIMERS
#include "dynlib.h"

#include <psp2/kernel/clib.h>
#include <psp2/kernel/dmac.h>
#include <psp2/kernel/sysmem.h>
#include <vitaGL.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <netdb.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <zlib.h>
#include <dirent.h>
#include <locale.h>
#include <poll.h>
#include "dll_psp2.h"
#include <fios/fios.h>

//#include <SLES/OpenSLES.h>

#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/param.h>

#include <so_util/so_util.h>

#include "utils/glutil.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "utils/prof.h"

#ifdef USE_SCELIBC_IO
#include <libc_bridge/libc_bridge.h>
#endif

#include "reimpl/egl.h"
#include "reimpl/env.h"
#include "reimpl/errno.h"
#include "reimpl/io.h"
#include "reimpl/ioctl.h"
#include "reimpl/log.h"
#include "reimpl/mem.h"
#include "reimpl/pthr.h"
#include "reimpl/sys.h"
#include "patch.h"

#include <AFakeNative/ALooper.h>
#include <AFakeNative/AAssetManager.h>
#include <AFakeNative/AInput.h>
#include <AFakeNative/AConfiguration.h>
#include <AFakeNative/ANativeWindow.h>
#include <AFakeNative/polling/pseudo_pipe.h>
#include <AFakeNative/PseudoEpoll.h>
#include <AFakeNative/AFakeNative.h>
#include <AFakeNative/AStorageManager.h>

// include openal
#include <AL/al.h>
#include <AL/alc.h>

#ifdef PROFILER_ENABLED
#include <utils/prof.h>
#include <libperf.h>
#endif

extern void * _ZNSt9exceptionD2Ev;
extern void * _ZSt17__throw_bad_allocv;
extern void * _ZSt9terminatev;
extern void * _ZdaPv;
extern void * _ZdlPv;
extern void * _Znaj;
extern void * __cxa_allocate_exception;
extern void * __cxa_begin_catch;
extern void * __cxa_end_catch;
extern void * __cxa_free_exception;
extern void * __cxa_rethrow;
extern void * __cxa_throw;
extern void * __gxx_personality_v0;
extern void *_ZNSt8bad_castD1Ev;
extern void *_ZTISt8bad_cast;
extern void *_ZTISt9exception;
extern void *_ZTVN10__cxxabiv117__class_type_infoE;
extern void *_ZTVN10__cxxabiv120__si_class_type_infoE;
extern void *_ZTVN10__cxxabiv121__vmi_class_type_infoE;
extern void *_Znwj;
extern void *__aeabi_atexit;
extern void *__cxa_atexit;
extern void *__cxa_finalize;
extern void *__cxa_pure_virtual;
extern void *__cxa_guard_acquire;
extern void *__cxa_guard_release;
extern void *__gnu_unwind_frame;
// extern void *__stack_chk_fail; // Implemented below instead
extern void *__stack_chk_guard;

// Custom stack smashing detector handler
// The game's __stack_chk_fail calls through a GOT entry that can be corrupted
// by the very stack overflow we're trying to detect. This safer implementation
// logs the error and aborts cleanly.
__attribute__((noreturn))
void __stack_chk_fail(void) {
    sceClibPrintf("\n*** STACK SMASHING DETECTED ***\n");
    sceClibPrintf("Stack buffer overflow detected!\n");
    sceClibPrintf("Aborting to prevent further corruption...\n");
    // Use abort() which goes through proper cleanup
    abort();
    // Never reached, but needed for noreturn
    __builtin_trap();
}

extern void *__aeabi_d2lz;
extern void *__aeabi_dadd;
extern void *__aeabi_dcmpgt;
extern void *__aeabi_dcmplt;
extern void *__aeabi_ddiv;
extern void *__aeabi_dmul;
extern void *__aeabi_f2lz;
extern void *__aeabi_i2d;
extern void *__aeabi_idiv;
extern void *__aeabi_idivmod;
extern void *__aeabi_l2d;
extern void *__aeabi_l2f;
extern void *__aeabi_ldivmod;
extern void *__aeabi_uidiv;
extern void *__aeabi_uidivmod;
extern void *__aeabi_ui2d;
extern void *__aeabi_ul2d;
extern void *__aeabi_ul2f;
extern void *__aeabi_uldivmod;
extern void *__aeabi_unwind_cpp_pr0;
extern void *__aeabi_unwind_cpp_pr1;
extern void *__gnu_ldivmod_helper;

extern void *__aeabi_memclr;
extern void *__aeabi_memcpy;
extern void *__aeabi_memmove;
extern void *__aeabi_memset;
extern void *__aeabi_memset4;
extern void *__aeabi_memset8;

extern void *__srget;
extern void *__swbuf;

extern const char *BIONIC_ctype_;
extern const short *BIONIC_tolower_tab_;
extern const short *BIONIC_toupper_tab_;


extern so_module so_mod;


static FILE __sF_fake[3];

// DMA staging buffers (uncached CDRAM for vertex buffer uploads)
#define DMA_STAGING_SIZE (1024 * 1024)  // 1MB staging buffer
static void *dma_staging_buffer = NULL;

// Shadow rendering optimization
static int g_in_shadow_rendering = 0;
static int g_shadow_clear_count = 0;
#define SKIP_SHADOW_CLEARS 0  // Causes glitches - don't skip!
#define OPTIMIZE_CLEAR_FLAGS 0  // Reduce clear flags (color only, not depth+stencil)
#define SHADOW_TEXTURE_SCALE 2  // 1=full (512x128), 2=half (256x64), 4=quarter (128x32)

int __atomic_dec(volatile int *ptr) {
	return __sync_fetch_and_sub (ptr, 1);
}

int __atomic_inc(volatile int *ptr) {
	return __sync_fetch_and_add (ptr, 1);
}

int __system_property_get(const char* name, char* value) {
	logv_error("__system_property_get(name: \"%s\")", name);
	return 0;
}

int sigaction(int signal, const struct sigaction* bionic_new_action, struct sigaction* bionic_old_action) {
	logv_error("sigaction: %i", signal);
	return 0;
}

int AAsset_getBuffer() {
	log_error("unimpl: AAsset_getBuffer");
	return 0;
}
int AAsset_getLength() {
	log_error("unimpl: AAsset_getLength");
	return 0;
}

int AAssetDir_close() {
	log_error("unimpl: AAssetDir_close");
	return 0;
}
int AAssetDir_getNextFileName() {
	log_error("unimpl: AAssetDir_getNextFileName");
	return 0;
}

int AAssetManager_openDir() {
	log_error("unimpl: AAssetManager_openDir");
	return 0;
}

int AKeyEvent_getFlags() {
	//log_error("unimpl: AKeyEvent_getFlags");
	return 0;
}
int AKeyEvent_getMetaState() {
	//log_error("unimpl: AKeyEvent_getMetaState");
	return 0;
}
int AMotionEvent_getFlags() {
	//log_error("unimpl: AMotionEvent_getFlags");
	return 0;
}
int AMotionEvent_getHistoricalX() {
	log_error("unimpl: AMotionEvent_getHistoricalX");
	return 0;
}
int AMotionEvent_getHistoricalY() {
	log_error("unimpl: AMotionEvent_getHistoricalY");
	return 0;
}
int AMotionEvent_getHistorySize() {
	//log_error("unimpl: AMotionEvent_getHistorySize");
	return 0;
}
int AMotionEvent_getMetaState() {
	//log_error("unimpl: AMotionEvent_getMetaState");
	return 0;
}
int __gnu_Unwind_Find_exidx() {
	log_error("ret0d function __gnu_Unwind_Find_exidx called");
	return 0;
}

int __exidx_end() {
	log_error("ret0d function __exidx_end called");
	return 0;
}

int __exidx_start() {
	log_error("ret0d function __exidx_start called");
	return 0;
}

void exit_soloader(int status) {
	logv_info("exit(%i) called from %p", status, __builtin_return_address(0));
	exit(status);
}

void *dlopen_hook(const char *restrict filename, int flags) {
	// if libandroid.so, we handle it
	if (strstr(filename, "libandroid.so") != NULL) {
		logv_error("dlopen(%s, %i) called", filename, flags);
		// Just return whatever for this case.
		// The game will call dlsym(0xDEADBEEF, "AMotionEvent_getAxisValue") immediately after dlopen
		// and we will handle it in dlsym_fake.
		return (void *)0xDEADBEEF;
	}

	logv_error("Not Implemented dlopen(%s, %i) called", filename, flags);

	return 0;
}

void *dlsym_fake(void *restrict handle, const char *restrict symbol) {
	logv_info("dlsym(%p, %s) called", handle, symbol);

	if (strcmp("JBE_android_main_sub", symbol) == 0) {
		uintptr_t jbe_andoid_main_addr = (uintptr_t) so_symbol(&so_mod, "JBE_android_main_sub");
		if (jbe_andoid_main_addr == 0)
		{
			log_error("[dlsym]JBE_android_main_sub not found");
		}
		else
		{
			logv_debug("[dlsym]JBE_android_main_sub found at %p", jbe_andoid_main_addr);
			return (void *) jbe_andoid_main_addr;
		}
	}
	if (strcmp("AMotionEvent_getAxisValue", symbol) == 0) {
		return (void *)AMotionEvent_getAxisValue;
	}

	logv_error("dlsym(%p, %s) not implemented", handle, symbol);
	return NULL;
}

// glTexParameterfv_fake
void glTexParameterfv_fake(GLenum target, GLenum pname, const GLfloat *params) {
	logv_error("[UNIMPLEMENTED] glTexParameterfv(%i, %i, %p) called", target, pname, params);
	//glTexParameterfv(target, pname, params);
}

// glBlendColor_wrap
void glBlendColor_wrap(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
	logv_error("[UNIMPLEMENTED] glBlendColor(%f, %f, %f, %f) called", red, green, blue, alpha);
	//glBlendColor(red, green, blue, alpha);
	return;
}

// glCompressedTexSubImage2D_fake
int glCompressedTexSubImage2D_fake(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) {
	logv_error("[UNIMPLEMENTED] glCompressedTexSubImage2D(%i, %i, %i, %i, %i, %i, %i, %i, %p) called", target, level, xoffset, yoffset, width, height, format, imageSize, data);
	//glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
	return 0;
}

// Original D3D functions (will be set during patching)
void (*D3DDevice_Clear_orig)(uint32_t count, void *rects, uint32_t flags, uint32_t color, float depth, uint32_t stencil) = NULL;
void (*RenderDelayedShadows_orig)(void) = NULL;
extern so_hook clear_hook;
extern so_hook renderDelayedShadows_hook;
extern so_hook createTexture2_hook;
// D3DDevice_Clear wrapper - optimize shadow clear flags
void D3DDevice_Clear(uint32_t count, void *rects, uint32_t flags, uint32_t color, uint32_t depth, uint32_t stencil) {
	uint32_t optimized_flags = flags;

	#if OPTIMIZE_CLEAR_FLAGS
	if (g_in_shadow_rendering && flags == 0xf0) {
		// Original: 0xf0 = clear color+depth+stencil (expensive!)
		// Optimized: 0x01 = clear color only (shadows don't use depth/stencil)
		optimized_flags = 0x01;
		g_shadow_clear_count++;
		//sceRazorCpuPushMarkerWithHud("Clear_OPT", SCE_RAZOR_COLOR_GREEN, SCE_RAZOR_MARKER_DISABLE_HUD);
	} else {
	#endif
		//sceRazorCpuPushMarkerWithHud("Clear", SCE_RAZOR_COLOR_YELLOW, SCE_RAZOR_MARKER_DISABLE_HUD);
	#if OPTIMIZE_CLEAR_FLAGS
	}
	#endif

	SO_CONTINUE(void*, clear_hook, count, rects, optimized_flags, color, depth, stencil);
	//sceRazorCpuPopMarker();
}

// RenderDelayedShadows wrapper - track when we're in shadow rendering
void renderDelayedShadows(void) {
	g_in_shadow_rendering = 1;
	g_shadow_clear_count = 0;

	SO_CONTINUE(void *, renderDelayedShadows_hook);


	g_in_shadow_rendering = 0;
}

// D3DDevice_CreateTexture2 wrapper - reduce shadow texture resolution
void* D3DDevice_CreateTexture2(uint32_t width, uint32_t height, uint32_t levels, uint32_t usage,
                                uint32_t pool, uint32_t format, uint32_t type) {
	uint32_t optimized_width = width;
	uint32_t optimized_height = height;

	#if SHADOW_TEXTURE_SCALE > 1
	// Detect shadow texture creation (512x128, format 6)
	if (g_in_shadow_rendering && width == 0x200 && height == 0x80 && format == 6) {
		optimized_width = width / SHADOW_TEXTURE_SCALE;
		optimized_height = height / SHADOW_TEXTURE_SCALE;
		// logv_info("Shadow texture: Reduced from %dx%d to %dx%d (scale=%d)",
		// 	width, height, optimized_width, optimized_height, SHADOW_TEXTURE_SCALE);
	}
	#endif

	return SO_CONTINUE(void*, createTexture2_hook, optimized_width, optimized_height,
		levels, usage, pool, format, type);
}

void glGenTextures_profiled(GLsizei n, GLuint *textures) {
	logv_error("glGenTextures(%i, %p) called", n, textures);
	//Profiler_BeginSample("glGenTextures");
	glGenTextures(n, textures);
	//Profiler_EndSample();
}

void glVertexAttrib4fv_profiled(GLuint index, const GLfloat *v) {
	//Profiler_BeginSample("glVertexAttrib4fv");
	glVertexAttrib4fv(index, v);
	//Profiler_EndSample();
}

// glTexImage2D_fake
void glTexImage2D_fake(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
	// if (level > 0)
	// {
	// 	return;
	// }
    // GLint prog = 0;
    // glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    // int usePalette = 0;
	// if (width == 1024 && height == 1024) {
	// 	      int* caller = __builtin_return_address(0);

	// 	logv_error("1024 found, prog=%d from: %p", prog, caller);
	// }
	glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

void glTexSubImage2D_fake(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
	//logv_error("glTexSubImage2D: target=0x%x, level=%d, offset=(%d,%d), size=(%dx%d), format=0x%x, type=0x%x, pixels=%p",
	//	target, level, xoffset, yoffset, width, height, format, type, pixels);

	glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void glTexParameteri_fake(GLenum target, GLenum pname, GLint param) {
	// Override filtering parameters to always use GL_NEAREST
	// if (pname == GL_TEXTURE_MIN_FILTER || pname == GL_TEXTURE_MAG_FILTER) {
	// 	//logv_error("glTexParameteri: Overriding filter 0x%x from %d to GL_NEAREST", pname, param);
	// 	param = GL_NEAREST;
	// }

	glTexParameteri(target, pname, param);
}

void glDeleteTextures_fake(GLsizei n, const GLuint *textures) {
	// Native GXM palettes don't need manual state cleanup
	glDeleteTextures(n, textures);
}

void app_dummy(void)
{
	return;
}

ssize_t read_delegate(int fd, void *buf, size_t count) {
	if (count == 0) {
		logv_error("!!! read_delegate called with count=0! fd=0x%x", fd);
	}
	//SceFiosFH fiosH = sceFiosFHToFileno(fd);
	//if (fiosH == 0xffffffff)
	if (fd < 0x18000)
	{
		//logv_error("non-fios read(fd=0x%x, 0x%p, %zu) delegate called", fd, buf, count);
		return read(fd, buf, count);
	}
	else
	{
		//logv_error("read(fd=0x%x, 0x%p, %zu) delegate called .fiosH=0x%x", fd, buf, count, fiosH);
		//uint32_t read = 0;
		//int res = sceFiosFHReadSync(NULL, fiosH, buf, count);
		int res = sceFiosFHReadSync(NULL, fd, buf, count);
		//logv_error("read(fd=0x%x, 0x%p, %zu) delegate called .fiosH=0x%x, read=0x%x, res=%i", fd, buf, count, fiosH, read, res);
		return res;
	}
}

off_t lseek_delegate(int fd, off_t offset, int whence) {
	if (fd < 0x18000)
	{
		int res = lseek(fd, offset, whence);
		return res;
	}
	else
	{
		int res = sceFiosFHSeek(fd, offset, whence);
		return res;
	}
}

//glViewport_profiled
void glViewport_profiled(GLint x, GLint y, GLsizei width, GLsizei height) {
	//Profiler_BeginSample("glViewport");
	glViewport(x, y, width, height);
	//Profiler_EndSample();
}

// eglSwapBuffers_profiled
int eglSwapBuffers_profiled(EGLDisplay dpy, EGLSurface surface) {
	//Profiler_BeginSample("eglSwapBuffers");
	int res = eglSwapBuffers(dpy, surface);
	//Profiler_EndSample();

	return res;
}

// glEnable_profiled
void glEnable_profiled(GLenum cap) {
	//Profiler_BeginSample("glEnable");
	glEnable(cap);
	//Profiler_EndSample();
}

// glDisable_profiled
void glDisable_profiled(GLenum cap) {
	//Profiler_BeginSample("glDisable");
	glDisable(cap);
	//Profiler_EndSample();
}

// glDrawElements_profiled	
void glDrawElements_profiled(GLenum mode, GLsizei count, GLenum type, const void *indices) {
	//Profiler_BeginSample("glDrawElements");
	//sceRazorCpuPushMarkerWithHud("glDrawElements", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);

	glDrawElements(mode, count, type, indices);
	//sceRazorCpuPopMarker();
}

void glBindBuffer_profiled(GLenum target, GLuint buffer) {
	// Profiler_BeginSample("glBindBuffer");
	//sceRazorCpuPushMarkerWithHud("glBindBuffer", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
	glBindBuffer(target, buffer);
	//sceRazorCpuPopMarker();
}

void glBufferSubData_profiled(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
	//Profiler_BeginSample("glBufferSubData");
	//logv_error("glBufferSubData_profiled (0x%X, 0x%X, 0x%X, %p)", target, offset, size, data);
	//sceRazorCpuPushMarkerWithHud("glBufferSubData", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);

	// Optimize both frequent buffer updates: use glMapBufferRange instead of glBufferSubData
	// Avoids intermediate buffer copy and allocation overhead
	if (target == GL_ARRAY_BUFFER && offset == 0 && (size == 0x90000 || size == 0x5280)) {
		// Try glMapBufferRange first (faster - direct write)
		void *mapped = glMapBufferRange(GL_ARRAY_BUFFER, offset, size,
			GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

		if (mapped) {
			// Just use optimized memcpy - staging buffer approach is slower
			// due to uncached memory write overhead
			//sceRazorCpuPushMarkerWithHud("sceClibMemcpy", SCE_RAZOR_COLOR_GREEN, SCE_RAZOR_MARKER_DISABLE_HUD);
			sceClibMemcpy(mapped, data, size);
			//sceRazorCpuPopMarker();

			glUnmapBuffer(GL_ARRAY_BUFFER);

			//logv_error("  -> Used glMapBuffer for buffer (0x%X bytes)", size);
		} else {
			// Fallback to glBufferSubData if mapping fails
			log_error("  -> glMapBuffer failed, using glBufferSubData");
			glBufferSubData(target, offset, size, data);
		}
		//sceRazorCpuPopMarker();
		return;
	}

	glBufferSubData(target, offset, size, data);
	//sceRazorCpuPopMarker();
}

static GLfloat g_shadowDepthBiasFactor = 0.0f;  // Try: -1, 0, 1
static GLfloat g_shadowDepthBiasUnits = -4.0f;  // Try: -14, -8, -4, -2, 2, 4, 8, 14

void glPolygonOffset_logged(GLfloat factor, GLfloat units) {
	GLfloat originalFactor = factor;
	GLfloat originalUnits = units;

	if (units <= -3900.0f) {
		// Shadow rendering (originally -4000.0) - needs special handling
		factor = g_shadowDepthBiasFactor;
		units = g_shadowDepthBiasUnits;
	} if (units <= -255.0f) {
		factor = 0.0f;
		units = -5.0f;
	}

	glPolygonOffset(factor, units);
}

//glGetUniformLocation_fake
GLint glGetUniformLocation_fake(GLuint program, const GLchar *name) {
	GLint res = glGetUniformLocation(program, name);
	logv_error("glGetUniformLocation(%u, %s) called, returning %d", program, name, res);
	return res;
}

// glBindTexture_fake - simplified for native GXM palettes
void glBindTexture_fake(GLenum target, GLuint texture) {
	glBindTexture(target, texture);
	// No longer need shader uniform management - GXM handles palettes natively
}

void alSourceQueueBuffers_safe(ALuint source, ALsizei n, const ALuint *buffers) {
      // Check if we have enough memory before calling
      void *test = malloc(4);
      if (!test) {
          logv_error("alSourceQueueBuffers: Out of memory, skipping queue: %x, %x, %x", source, n, buffers);
          return;  // Silently fail instead of crashing
      }
      free(test);

      alSourceQueueBuffers(source, n, buffers);
}

// void glShaderSource_fake(GLuint shader, GLsizei count, const GLchar * const *string, const GLint *length) {
// 	uint32_t threadId = sceKernelGetThreadId();
// 	logv_error("----------[T%u] glShaderSource: shader=%u, count=%d", threadId, shader, count);

// 	// Write shader to file
// 	char filename[256];
// 	snprintf(filename, sizeof(filename), "ux0:data/dump_shaders/shader_%u.glsl", shader);

// 	FILE* file = fopen(filename, "w");
// 	if (file) {
// 		fprintf(file, "// Shader ID: %u, Thread: %u, Count: %d\n", shader, threadId, count);

// 		// Write each string in the array
// 		for (GLsizei i = 0; i < count; i++) {
// 			if (string[i]) {
// 				int len = length ? length[i] : strlen(string[i]);
// 				fprintf(file, "%.*s", len, string[i]);
// 			}
// 		}
// 		fclose(file);
// 		logv_error("[T%u] Shader %u written to %s", threadId, shader, filename);
// 	} else {
// 		logv_error("[T%u] Failed to write shader %u to file", threadId, shader);
// 	}

// 	glShaderSource(shader, count, string, length);
// }

void glAttachShader_fake(GLuint program, GLuint shader) {
	uint32_t threadId = sceKernelGetThreadId();
	logv_error("[T%u] glAttachShader: program=%u, shader=%u", threadId, program, shader);
	glAttachShader(program, shader);
}

so_default_dynlib default_dynlib[] = {
		// Common C/C++ internals
		{ "_ZNSt8bad_castD1Ev", (uintptr_t)&_ZNSt8bad_castD1Ev },
		{ "_ZNSt9exceptionD2Ev", (uintptr_t)&_ZNSt9exceptionD2Ev },
		{ "_ZSt17__throw_bad_allocv", (uintptr_t)&_ZSt17__throw_bad_allocv },
		{ "_ZSt9terminatev", (uintptr_t)&_ZSt9terminatev },
		{ "_ZTISt8bad_cast", (uintptr_t)&_ZTISt8bad_cast },
		{ "_ZTISt9exception", (uintptr_t)&_ZTISt9exception },
		{ "_ZTVN10__cxxabiv117__class_type_infoE", (uintptr_t)&_ZTVN10__cxxabiv117__class_type_infoE },
		{ "_ZTVN10__cxxabiv120__si_class_type_infoE", (uintptr_t)&_ZTVN10__cxxabiv120__si_class_type_infoE },
		{ "_ZTVN10__cxxabiv121__vmi_class_type_infoE", (uintptr_t)&_ZTVN10__cxxabiv121__vmi_class_type_infoE },
		{ "_ZdaPv", (uintptr_t)&_ZdaPv },
		{ "_ZdlPv", (uintptr_t)&_ZdlPv },
		{ "_Znaj", (uintptr_t)&_Znaj },
		{ "_Znwj", (uintptr_t)&_Znwj },
		{ "__aeabi_atexit", (uintptr_t)&__aeabi_atexit },
		{ "__aeabi_d2lz", (uintptr_t)&__aeabi_d2lz },
		{ "__aeabi_dadd", (uintptr_t)&__aeabi_dadd },
		{ "__aeabi_dcmpgt", (uintptr_t)&__aeabi_dcmpgt },
		{ "__aeabi_dcmplt", (uintptr_t)&__aeabi_dcmplt },
		{ "__aeabi_ddiv", (uintptr_t)&__aeabi_ddiv },
		{ "__aeabi_dmul", (uintptr_t)&__aeabi_dmul },
		{ "__aeabi_f2lz", (uintptr_t)&__aeabi_f2lz },
		{ "__aeabi_i2d", (uintptr_t)&__aeabi_i2d },
		{ "__aeabi_idiv", (uintptr_t)&__aeabi_idiv },
		{ "__aeabi_idivmod", (uintptr_t)&__aeabi_idivmod },
		{ "__aeabi_l2d", (uintptr_t)&__aeabi_l2d },
		{ "__aeabi_l2f", (uintptr_t)&__aeabi_l2f },
		{ "__aeabi_ldivmod", (uintptr_t)&__aeabi_ldivmod },
		{ "__aeabi_memclr", (uintptr_t)&__aeabi_memclr },
		{ "__aeabi_memclr4", (uintptr_t)&__aeabi_memclr },
		{ "__aeabi_memclr8", (uintptr_t)&__aeabi_memclr },
		{ "__aeabi_memcpy", (uintptr_t)&__aeabi_memcpy },
		{ "__aeabi_memcpy4", (uintptr_t)&__aeabi_memcpy },
		{ "__aeabi_memcpy8", (uintptr_t)&__aeabi_memcpy },
		{ "__aeabi_memmove", (uintptr_t)&__aeabi_memmove },
		{ "__aeabi_memmove4", (uintptr_t)&__aeabi_memmove },
		{ "__aeabi_memmove8", (uintptr_t)&__aeabi_memmove },
		{ "__aeabi_memset", (uintptr_t)&__aeabi_memset },
		{ "__aeabi_memset4",  (uintptr_t)&__aeabi_memset4 },
		{ "__aeabi_memset8", (uintptr_t)&__aeabi_memset8 },
		{ "__aeabi_ui2d", (uintptr_t)&__aeabi_ui2d },
		{ "__aeabi_uidiv", (uintptr_t)&__aeabi_uidiv },
		{ "__aeabi_uidivmod", (uintptr_t)&__aeabi_uidivmod },
		{ "__aeabi_ul2d", (uintptr_t)&__aeabi_ul2d },
		{ "__aeabi_ul2f", (uintptr_t)&__aeabi_ul2f },
		{ "__aeabi_uldivmod", (uintptr_t)&__aeabi_uldivmod },
		{ "__aeabi_unwind_cpp_pr0", (uintptr_t)&__aeabi_unwind_cpp_pr0 },
		{ "__aeabi_unwind_cpp_pr1", (uintptr_t)&__aeabi_unwind_cpp_pr1 },
		{ "__atomic_dec", (uintptr_t)&__atomic_dec },
		{ "__atomic_inc", (uintptr_t)&__atomic_inc },
		{ "__cxa_allocate_exception", (uintptr_t)&__cxa_allocate_exception },
		{ "__cxa_atexit", (uintptr_t)&__cxa_atexit },
		{ "__cxa_begin_catch", (uintptr_t)&__cxa_begin_catch },
		{ "__cxa_end_catch", (uintptr_t)&__cxa_end_catch },
		{ "__cxa_finalize", (uintptr_t)&__cxa_finalize },
		{ "__cxa_free_exception", (uintptr_t)&__cxa_free_exception },
		{ "__cxa_guard_acquire", (uintptr_t)&__cxa_guard_acquire },
		{ "__cxa_guard_release", (uintptr_t)&__cxa_guard_release },
		{ "__cxa_pure_virtual", (uintptr_t)&__cxa_pure_virtual },
		{ "__cxa_rethrow", (uintptr_t)&__cxa_rethrow },
		{ "__cxa_throw", (uintptr_t)&__cxa_throw },
		{ "__exidx_end", (uintptr_t)&__exidx_end },
		{ "__exidx_start", (uintptr_t)&__exidx_start },
		{ "__gnu_Unwind_Find_exidx", (uintptr_t)&__gnu_Unwind_Find_exidx },
		{ "__gnu_ldivmod_helper", (uintptr_t)&__gnu_ldivmod_helper },
		{ "__gnu_unwind_frame", (uintptr_t)&__gnu_unwind_frame },
		{ "__google_potentially_blocking_region_begin", (uintptr_t)&ret0 },
		{ "__google_potentially_blocking_region_end", (uintptr_t)&ret0 },
		{ "__gxx_personality_v0", (uintptr_t)&__gxx_personality_v0 },
		{ "__sF", (uintptr_t)&__sF_fake },
		{ "__srget", (uintptr_t)&__srget },
		{ "__stack_chk_fail", (uintptr_t)&__stack_chk_fail },
		{ "__stack_chk_guard", (uintptr_t)&__stack_chk_guard },
		{ "__swbuf", (uintptr_t)&__swbuf },
		{ "__system_property_get", (uintptr_t)&__system_property_get },

		// ANative
		{ "AAssetDir_close", (uintptr_t)&AAssetDir_close },
		{ "AAssetDir_getNextFileName", (uintptr_t)&AAssetDir_getNextFileName },
		{ "AAssetManager_open", (uintptr_t)&AAssetManager_open },
		{ "AAssetManager_openDir", (uintptr_t)&AAssetManager_openDir },
		{ "AAsset_close", (uintptr_t)&AAsset_close },
		{ "AAsset_getBuffer", (uintptr_t)&AAsset_getBuffer },
		{ "AAsset_getLength", (uintptr_t)&AAsset_getLength },
		{ "AAsset_openFileDescriptor", (uintptr_t)&AAsset_openFileDescriptor },
		{ "AAsset_read", (uintptr_t)&AAsset_read },
		{ "AAsset_seek", (uintptr_t)&AAsset_seek },
		{ "AConfiguration_delete", (uintptr_t)&AConfiguration_delete },
		{ "AConfiguration_fromAssetManager", (uintptr_t)&AConfiguration_fromAssetManager },
		{ "AConfiguration_getCountry", (uintptr_t)&AConfiguration_getCountry },
		{ "AConfiguration_getLanguage", (uintptr_t)&AConfiguration_getLanguage },
		{ "AConfiguration_new", (uintptr_t)&AConfiguration_new },
		{ "AInputEvent_getDeviceId", (uintptr_t)&AInputEvent_getDeviceId },
		{ "AInputEvent_getSource", (uintptr_t)&AInputEvent_getSource },
		{ "AInputEvent_getType", (uintptr_t)&AInputEvent_getType },
		{ "AInputQueue_attachLooper", (uintptr_t)&AInputQueue_attachLooper },
		{ "AInputQueue_detachLooper", (uintptr_t)&AInputQueue_detachLooper },
		{ "AInputQueue_finishEvent", (uintptr_t)&AInputQueue_finishEvent },
		{ "AInputQueue_getEvent", (uintptr_t)&AInputQueue_getEvent },
		{ "AInputQueue_preDispatchEvent", (uintptr_t)&AInputQueue_preDispatchEvent },
		{ "AKeyEvent_getAction", (uintptr_t)&AKeyEvent_getAction },
		{ "AKeyEvent_getFlags", (uintptr_t)&AKeyEvent_getFlags },
		{ "AKeyEvent_getKeyCode", (uintptr_t)&AKeyEvent_getKeyCode },
		{ "AKeyEvent_getMetaState", (uintptr_t)&AKeyEvent_getMetaState },
		{ "ALooper_addFd", (uintptr_t)&ALooper_addFd },
		{ "ALooper_pollAll", (uintptr_t)&ALooper_pollAll },
		{ "ALooper_prepare", (uintptr_t)&ALooper_prepare },
		{ "AMotionEvent_getAction", (uintptr_t)&AMotionEvent_getAction },
		{ "AMotionEvent_getAxisValue", (uintptr_t)&AMotionEvent_getAxisValue },
		{ "AMotionEvent_getFlags", (uintptr_t)&AMotionEvent_getFlags },
		{ "AMotionEvent_getHistoricalX", (uintptr_t)&AMotionEvent_getHistoricalX },
		{ "AMotionEvent_getHistoricalY", (uintptr_t)&AMotionEvent_getHistoricalY },
		{ "AMotionEvent_getHistorySize", (uintptr_t)&AMotionEvent_getHistorySize },
		{ "AMotionEvent_getMetaState", (uintptr_t)&AMotionEvent_getMetaState },
		{ "AMotionEvent_getPointerCount", (uintptr_t)&AMotionEvent_getPointerCount },
		{ "AMotionEvent_getPointerId", (uintptr_t)&AMotionEvent_getPointerId },
		{ "AMotionEvent_getX", (uintptr_t)&AMotionEvent_getX },
		{ "AMotionEvent_getY", (uintptr_t)&AMotionEvent_getY },
		{ "ANativeActivity_finish", (uintptr_t)&ANativeActivity_finish },
		{ "ANativeActivity_setWindowFlags", (uintptr_t)&ANativeActivity_setWindowFlags },
		{ "ANativeWindow_getHeight", (uintptr_t)&ANativeWindow_getHeight },
		{ "ANativeWindow_getWidth", (uintptr_t)&ANativeWindow_getWidth },
		{ "ANativeWindow_setBuffersGeometry", (uintptr_t)&ANativeWindow_setBuffersGeometry },
		{ "ASensorEventQueue_disableSensor", (uintptr_t)&ret0 },
		{ "ASensorEventQueue_enableSensor", (uintptr_t)&ret0 },
		{ "ASensorEventQueue_getEvents", (uintptr_t)&ret0 },
		{ "ASensorEventQueue_setEventRate", (uintptr_t)&ret0 },
		{ "ASensorManager_createEventQueue", (uintptr_t)&ret0 },
		{ "ASensorManager_getDefaultSensor", (uintptr_t)&ret0 },
		{ "ASensorManager_getInstance", (uintptr_t)&ret0 },
		
		{ "AStorageManager_new", (uintptr_t)&AStorageManager_new },
		{ "AStorageManager_getMountedObbPath", (uintptr_t)&AStorageManager_getMountedObbPath },
		{ "AStorageManager_delete", (uintptr_t)&AStorageManager_delete },

		// ctype
		{ "_ctype_", (uintptr_t)&BIONIC_ctype_ },
		{ "_tolower_tab_", (uintptr_t)&BIONIC_tolower_tab_ },
		{ "_toupper_tab_", (uintptr_t)&BIONIC_toupper_tab_ },
		{ "isalnum", (uintptr_t)&isalnum },
		{ "isalpha", (uintptr_t)&isalpha },
		{ "isblank", (uintptr_t)&isblank },
		{ "iscntrl", (uintptr_t)&iscntrl },
		{ "isgraph", (uintptr_t)&isgraph },
		{ "islower", (uintptr_t)&islower },
		{ "isprint", (uintptr_t)&isprint },
		{ "ispunct", (uintptr_t)&ispunct },
		{ "isspace", (uintptr_t)&isspace },
		{ "isupper", (uintptr_t)&isupper },
		{ "isxdigit", (uintptr_t)&isxdigit },
		{ "tolower", (uintptr_t)&tolower },
		{ "toupper", (uintptr_t)&toupper },


		// Android SDK standard logging
		{ "__android_log_print", (uintptr_t)&android_log_print },
		{ "__android_log_vprint", (uintptr_t)&android_log_vprint },
		{ "__android_log_write", (uintptr_t)&android_log_write },


		// Math
		{ "acos", (uintptr_t)&acos },
		{ "acosf", (uintptr_t)&acosf },
		{ "asin", (uintptr_t)&asin },
		{ "asinf", (uintptr_t)&asinf },
		{ "atan", (uintptr_t)&atan },
		{ "atan2", (uintptr_t)&atan2 },
		{ "atan2f", (uintptr_t)&atan2f },
		{ "atanf", (uintptr_t)&atanf },
		{ "ceil", (uintptr_t)&ceil },
		{ "ceilf", (uintptr_t)&ceilf },
		{ "cos", (uintptr_t)&cos },
		{ "cosf", (uintptr_t)&cosf },
		{ "exp", (uintptr_t)&exp },
		{ "exp2", (uintptr_t)&exp2 },
		{ "exp2f", (uintptr_t)&exp2f },
		{ "expf", (uintptr_t)&expf },
		{ "floor", (uintptr_t)&floor },
		{ "floorf", (uintptr_t)&floorf },
		{ "fmod", (uintptr_t)&fmod },
		{ "fmodf", (uintptr_t)&fmodf },
		{ "frexp", (uintptr_t)&frexp },
		{ "ldexp", (uintptr_t)&ldexp },
		{ "ldexpf", (uintptr_t)&ldexpf },
		{ "log", (uintptr_t)&log },
		{ "log10", (uintptr_t)&log10 },
		{ "log10f", (uintptr_t)&log10f },
		{ "logf", (uintptr_t)&logf },
		{ "lrint", (uintptr_t)&lrint },
		{ "lrintf", (uintptr_t)&lrintf },
		{ "lround", (uintptr_t)&lround },
		{ "lroundf", (uintptr_t)&lroundf },
		{ "modf", (uintptr_t)&modf },
		{ "modff", (uintptr_t)&modff },
		{ "pow", (uintptr_t)&pow },
		{ "powf", (uintptr_t)&powf },
		{ "rint", (uintptr_t)&rint },
		{ "rintf", (uintptr_t)&rintf },
		{ "round", (uintptr_t)&round },
		{ "roundf", (uintptr_t)&roundf },
		{ "scalbn", (uintptr_t)&scalbn },
		{ "scalbnf", (uintptr_t)&scalbnf },
		{ "sin", (uintptr_t)&sin },
		{ "sincos", (uintptr_t)&sincos },
		{ "sincosf", (uintptr_t)&sincosf },
		{ "sinf", (uintptr_t)&sinf },
		{ "sinh", (uintptr_t)&sinh },
		{ "sqrt", (uintptr_t)&sqrt },
		{ "sqrtf", (uintptr_t)&sqrtf },
		{ "tan", (uintptr_t)&tan },
		{ "tanf", (uintptr_t)&tanf },
		{ "tanh", (uintptr_t)&tanh },
		{ "trunc", (uintptr_t)&trunc },
		{ "truncf", (uintptr_t)&truncf },


		// Sockets
		{ "accept", (uintptr_t)&accept },
		{ "bind", (uintptr_t)&bind },
		{ "connect", (uintptr_t)&connect },
		{ "freeaddrinfo", (uintptr_t)&freeaddrinfo },
		{ "getaddrinfo", (uintptr_t)&getaddrinfo },
		{ "gethostbyaddr", (uintptr_t)&gethostbyaddr },
		{ "gethostbyname", (uintptr_t)&gethostbyname },
		//{ "gethostname", (uintptr_t)&gethostname },
		{ "getpeername", (uintptr_t)&getpeername },
		{ "getservbyname", (uintptr_t)&getservbyname },
		{ "getsockname", (uintptr_t)&getsockname },
		{ "getsockopt", (uintptr_t)&getsockopt },
		{ "inet_aton", (uintptr_t)&inet_aton },
		{ "inet_ntoa", (uintptr_t)&inet_ntoa },
		{ "inet_ntop", (uintptr_t)&inet_ntop },
		{ "listen", (uintptr_t)&listen },
		{ "poll", (uintptr_t)&poll },
		{ "recv", (uintptr_t)&recv },
		{ "recvfrom", (uintptr_t)&recvfrom },
		{ "recvmsg", (uintptr_t)&recvmsg },
		{ "select", (uintptr_t)&select },
		{ "send", (uintptr_t)&send },
		{ "sendmsg", (uintptr_t)&sendmsg },
		{ "sendto", (uintptr_t)&sendto },
		{ "setsockopt", (uintptr_t)&setsockopt },
		{ "shutdown", (uintptr_t)&shutdown },
		{ "socket", (uintptr_t)&socket },
		

		// Memory
		{ "calloc", (uintptr_t)&calloc },
		{ "free", (uintptr_t)&free },
		{ "malloc", (uintptr_t)&malloc },
		{ "memalign", (uintptr_t)&memalign },
		{ "memcmp", (uintptr_t)&memcmp },
		{ "memcpy", (uintptr_t)&memcpy },
		{ "memmem", (uintptr_t)&memmem },
		{ "memmove", (uintptr_t)&memmove },
		{ "memset", (uintptr_t)&memset },
		{ "mmap", (uintptr_t)&mmap },
		{ "munmap", (uintptr_t)&munmap },
		{ "realloc", (uintptr_t)&realloc },
		{ "valloc", (uintptr_t)&valloc },
		

		// IO
		{ "close", (uintptr_t)&close_soloader },
		{ "closedir", (uintptr_t)&closedir_soloader },
		{ "fclose", (uintptr_t)&fclose_soloader },
		{ "fcntl", (uintptr_t)&fcntl_soloader },
		{ "fopen", (uintptr_t)&fopen_soloader },
		//{ "fread", (uintptr_t)&fread_soloader },
		{ "fstat", (uintptr_t)&fstat_soloader },
		//{ "fseek", (uintptr_t)&fseek_soloader },
		//{ "ftell", (uintptr_t)&ftell_soloader },
		{ "fsync", (uintptr_t)&fsync_soloader },
		{ "ioctl", (uintptr_t)&ioctl_soloader },
		{ "open", (uintptr_t)&open_soloader },
		{ "opendir", (uintptr_t)&opendir_soloader },
		{ "readdir", (uintptr_t)&readdir_soloader },
		{ "readdir_r", (uintptr_t)&readdir_r_soloader },
		{ "stat", (uintptr_t)&stat_soloader },
		{ "rewinddir", (uintptr_t)&rewinddir },


		#ifdef USE_SCELIBC_IO
			{ "fdopen", (uintptr_t)&sceLibcBridge_fdopen },
			{ "feof", (uintptr_t)&sceLibcBridge_feof },
			{ "ferror", (uintptr_t)&sceLibcBridge_ferror },
			{ "fflush", (uintptr_t)&sceLibcBridge_fflush },
			{ "fgetc", (uintptr_t)&sceLibcBridge_fgetc },
			{ "fgetpos", (uintptr_t)&sceLibcBridge_fgetpos },
			{ "fgets", (uintptr_t)&sceLibcBridge_fgets },
			{ "fputc", (uintptr_t)&sceLibcBridge_fputc },
			{ "fputs", (uintptr_t)&sceLibcBridge_fputs },
			{ "fread", (uintptr_t)&fread_soloader },
			{ "freopen", (uintptr_t)&sceLibcBridge_freopen },
			{ "fseek", (uintptr_t)&fseek_soloader },
			{ "fsetpos", (uintptr_t)&sceLibcBridge_fsetpos },
			{ "ftell", (uintptr_t)&ftell_soloader },
			{ "fwrite", (uintptr_t)&sceLibcBridge_fwrite },
			{ "getc", (uintptr_t)&sceLibcBridge_getc },
			{ "getwc", (uintptr_t)&sceLibcBridge_getwc },
			{ "putc", (uintptr_t)&sceLibcBridge_putc },
			{ "putchar", (uintptr_t)&sceLibcBridge_putchar },
			{ "puts", (uintptr_t)&sceLibcBridge_puts },
			{ "putwc", (uintptr_t)&sceLibcBridge_putwc },
			{ "setvbuf", (uintptr_t)&sceLibcBridge_setvbuf },
			{ "ungetc", (uintptr_t)&sceLibcBridge_ungetc },
			{ "ungetwc", (uintptr_t)&sceLibcBridge_ungetwc },
		#else
			{ "fdopen", (uintptr_t)&fdopen },
			{ "feof", (uintptr_t)&feof },
			{ "ferror", (uintptr_t)&ferror },
			{ "fflush", (uintptr_t)&fflush },
			{ "fgetc", (uintptr_t)&fgetc },
			{ "fgetpos", (uintptr_t)&fgetpos },
			{ "fgets", (uintptr_t)&fgets },
			{ "fputc", (uintptr_t)&fputc },
			{ "fputs", (uintptr_t)&fputs },
			{ "fread", (uintptr_t)&fread },
			{ "freopen", (uintptr_t)&freopen },
			{ "fseek", (uintptr_t)&fseek },
			{ "fsetpos", (uintptr_t)&fsetpos },
			{ "ftell", (uintptr_t)&ftell },
			{ "fwrite", (uintptr_t)&fwrite },
			{ "getc", (uintptr_t)&getc },
			{ "getwc", (uintptr_t)&getwc },
			{ "putc", (uintptr_t)&putc },
			{ "putchar", (uintptr_t)&putchar },
			{ "puts", (uintptr_t)&puts },
			{ "putwc", (uintptr_t)&putwc },
			{ "setvbuf", (uintptr_t)&setvbuf },
			{ "ungetc", (uintptr_t)&ungetc },
			{ "ungetwc", (uintptr_t)&ungetwc },
		#endif

		{ "access", (uintptr_t)&access },
		{ "chdir", (uintptr_t)&chdir },
		{ "chmod", (uintptr_t)&chmod },
		{ "dup", (uintptr_t)&dup },
		{ "fileno", (uintptr_t)&fileno },
		{ "fseeko", (uintptr_t)&fseeko }, // TODO: wrap normal fseek for SceLibc version?
		{ "ftello", (uintptr_t)&ftello },
		{ "ftruncate", (uintptr_t)&ftruncate },
		{ "getcwd", (uintptr_t)&getcwd },
		{ "lseek", (uintptr_t)&lseek_delegate },
		//{ "lstat", (uintptr_t)&lstat },
		{ "mkdir", (uintptr_t)&mkdir },
		{ "pipe", (uintptr_t)&pseudo_pipe },
		{ "read", (uintptr_t)&pseudo_read },
		{ "realpath", (uintptr_t)&realpath },
		{ "remove", (uintptr_t)&remove },
		{ "rename", (uintptr_t)&rename },
		{ "rewind", (uintptr_t)&rewind },
		{ "rmdir", (uintptr_t)&rmdir },
		{ "truncate", (uintptr_t)&truncate },
		{ "unlink", (uintptr_t)&unlink },
		{ "write", (uintptr_t)&pseudo_write },


		// *printf, *scanf
		{ "snprintf", (uintptr_t)&snprintf },
		{ "sprintf", (uintptr_t)&sprintf },
		{ "vasprintf", (uintptr_t)&vasprintf },
		{ "vprintf", (uintptr_t)&vprintf },
		{ "vsnprintf", (uintptr_t)&vsnprintf },
		{ "vsprintf", (uintptr_t)&vsprintf },
		{ "vsscanf", (uintptr_t)&vsscanf },
		{ "vswprintf", (uintptr_t)&vswprintf },
		#ifdef USE_SCELIBC_IO
		{ "printf", (uintptr_t)&sceClibPrintf },
		{ "fprintf", (uintptr_t)&sceLibcBridge_fprintf },
		{ "fscanf", (uintptr_t)&sceLibcBridge_fscanf },
		{ "sscanf", (uintptr_t)&sceLibcBridge_sscanf },
		{ "vfprintf", (uintptr_t)&sceLibcBridge_vfprintf },
		#else
		{ "printf", (uintptr_t)&printf },
		{ "fprintf", (uintptr_t)&fprintf },
		{ "fscanf", (uintptr_t)&fscanf },
		{ "sscanf", (uintptr_t)&sscanf },
		{ "vfprintf", (uintptr_t)&vfprintf },
		#endif


		// OpenGL
		{ "glActiveTexture", (uintptr_t)&glActiveTexture },
		{ "glAlphaFuncx", (uintptr_t)&glAlphaFuncx },
		{ "glAttachShader", (uintptr_t)&glAttachShader },
		{ "glBindAttribLocation", (uintptr_t)&glBindAttribLocation },
		{ "glBindBuffer", (uintptr_t)&glBindBuffer_profiled },
		{ "glBindFramebuffer", (uintptr_t)&glBindFramebuffer },
		{ "glBindRenderbuffer", (uintptr_t)&glBindRenderbuffer },
		{ "glBindTexture", (uintptr_t)&glBindTexture },
		{ "glBlendEquation", (uintptr_t)&glBlendEquation },
		{ "glBlendEquationSeparate", (uintptr_t)&glBlendEquationSeparate },
		{ "glBlendFunc", (uintptr_t)&glBlendFunc },
		{ "glBlendFuncSeparate", (uintptr_t)&glBlendFuncSeparate },
		{ "glBufferData", (uintptr_t)&glBufferData },
		{ "glBufferSubData", (uintptr_t)&glBufferSubData },
		{ "glCheckFramebufferStatus", (uintptr_t)&glCheckFramebufferStatus },
		{ "glClear", (uintptr_t)&glClear },
		{ "glClearColor", (uintptr_t)&glClearColor },
		{ "glClearColorx", (uintptr_t)&glClearColorx },
		{ "glClearDepthf", (uintptr_t)&glClearDepthf },
		{ "glClearDepthx", (uintptr_t)&glClearDepthx },
		{ "glClearStencil", (uintptr_t)&glClearStencil },
		{ "glColor4x", (uintptr_t)&glColor4x },
		{ "glColorMask", (uintptr_t)&glColorMask },
		{ "glColorPointer", (uintptr_t)&glColorPointer },
		{ "glCompileShader", (uintptr_t)&glCompileShader },
		{ "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2D },
		{ "glCompressedTexSubImage2D", (uintptr_t)&glCompressedTexSubImage2D_fake },
		{ "glCopyTexImage2D", (uintptr_t)&glCopyTexImage2D },
		{ "glCopyTexSubImage2D", (uintptr_t)&glCopyTexSubImage2D },
		{ "glCreateProgram", (uintptr_t)&glCreateProgram },
		{ "glCreateShader", (uintptr_t)&glCreateShader },
		{ "glCullFace", (uintptr_t)&glCullFace },
		{ "glDeleteBuffers", (uintptr_t)&glDeleteBuffers },
		{ "glDeleteFramebuffers", (uintptr_t)&glDeleteFramebuffers },
		{ "glDeleteProgram", (uintptr_t)&glDeleteProgram },
		{ "glDeleteRenderbuffers", (uintptr_t)&glDeleteRenderbuffers },
		{ "glDeleteShader", (uintptr_t)&glDeleteShader },
		{ "glDeleteTextures", (uintptr_t)&glDeleteTextures },
		{ "glDepthFunc", (uintptr_t)&glDepthFunc },
		{ "glDepthMask", (uintptr_t)&glDepthMask },
		{ "glDepthRangef", (uintptr_t) &glDepthRangef },
		{ "glDetachShader", (uintptr_t)&ret0 },
		{ "glDisable", (uintptr_t)&glDisable },
		{ "glDisableClientState", (uintptr_t)&glDisableClientState },
		{ "glDisableVertexAttribArray", (uintptr_t)&glDisableVertexAttribArray },
		{ "glDrawArrays", (uintptr_t)&glDrawArrays },
		{ "glDrawElements", (uintptr_t)&glDrawElements_profiled },
		{ "glEnable", (uintptr_t)&glEnable },
		{ "glEnableClientState", (uintptr_t)&glEnableClientState },
		{ "glEnableVertexAttribArray", (uintptr_t)&glEnableVertexAttribArray },
		{ "glFlush", (uintptr_t)&glFlush },
		{ "glFramebufferRenderbuffer", (uintptr_t)&glFramebufferRenderbuffer },
		{ "glFramebufferTexture2D", (uintptr_t)&glFramebufferTexture2D },
		{ "glFrontFace", (uintptr_t)&glFrontFace },
		{ "glGenBuffers", (uintptr_t)&glGenBuffers },
		{ "glGenerateMipmap", (uintptr_t)&glGenerateMipmap },
		{ "glGenFramebuffers", (uintptr_t)&glGenFramebuffers },
		{ "glGenRenderbuffers", (uintptr_t)&glGenRenderbuffers },
		{ "glGenTextures", (uintptr_t)&glGenTextures },
		{ "glGetActiveAttrib", (uintptr_t)&glGetActiveAttrib },
		{ "glGetActiveUniform", (uintptr_t)&glGetActiveUniform },
		{ "glGetAttribLocation", (uintptr_t)&glGetAttribLocation },
		{ "glGetError", (uintptr_t)&glGetError },
		{ "glGetFloatv", (uintptr_t)&glGetFloatv },
		{ "glGetIntegerv", (uintptr_t)&glGetIntegerv },
		{ "glGetProgramInfoLog", (uintptr_t)&glGetProgramInfoLog },
		{ "glGetProgramiv", (uintptr_t)&glGetProgramiv },
		{ "glGetShaderInfoLog", (uintptr_t)&glGetShaderInfoLog },
		{ "glGetShaderiv", (uintptr_t)&glGetShaderiv },
		{ "glGetString", (uintptr_t)&glGetString },
		{ "glGetUniformLocation", (uintptr_t)&glGetUniformLocation },
		{ "glHint", (uintptr_t)&glHint },
		{ "glLightModelxv", (uintptr_t)&glLightModelxv },
		{ "glLightx", (uintptr_t)&ret0 },
		{ "glLightxv", (uintptr_t)&glLightxv },
		{ "glLineWidth", (uintptr_t)&glLineWidth },
		{ "glLinkProgram", (uintptr_t)&glLinkProgram },
		{ "glLoadMatrixf", (uintptr_t)&glLoadMatrixf },
		{ "glLoadMatrixx", (uintptr_t)&glLoadMatrixx },
		{ "glMaterialx", (uintptr_t)&ret0 },
		{ "glMaterialxv", (uintptr_t)&glMaterialxv },
		{ "glMatrixMode", (uintptr_t)&glMatrixMode },
		{ "glNormalPointer", (uintptr_t)&glNormalPointer },
		{ "glPixelStorei", (uintptr_t)&ret0 },
		{ "glPolygonOffset", (uintptr_t)&glPolygonOffset_logged },
		{ "glPopMatrix", (uintptr_t)&glPopMatrix },
		{ "glPushMatrix", (uintptr_t)&glPushMatrix },
		{ "glReadPixels", (uintptr_t)&glReadPixels },
		{ "glRenderbufferStorage", (uintptr_t)&glRenderbufferStorage },
		{ "glScissor", (uintptr_t)&glScissor },
		{ "glShadeModel", (uintptr_t)&glShadeModel },
		{ "glShaderSource", (uintptr_t)&glShaderSource },
		{ "glStencilFunc", (uintptr_t)&glStencilFunc },
		{ "glStencilFuncSeparate", (uintptr_t)&glStencilFuncSeparate },
		{ "glStencilMask", (uintptr_t)&glStencilMask },
		{ "glStencilOp", (uintptr_t)&glStencilOp },
		{ "glStencilOpSeparate", (uintptr_t)&glStencilOpSeparate },
		{ "glTexCoordPointer", (uintptr_t)&glTexCoordPointer },
		{ "glTexEnvx", (uintptr_t)&glTexEnvx },
		{ "glTexEnvxv", (uintptr_t)&glTexEnvxv },
		{ "glTexImage2D", (uintptr_t)&glTexImage2D },
		{ "glTexParameterf", (uintptr_t)&glTexParameterf },
		{ "glTexParameteri", (uintptr_t)&glTexParameteri },
		{ "glTexSubImage2D", (uintptr_t)&glTexSubImage2D },
		{ "glUniform1f", (uintptr_t)&glUniform1f },
		{ "glUniform1fv", (uintptr_t)&glUniform1fv },
		{ "glUniform1i", (uintptr_t)&glUniform1i },
		{ "glUniform1iv", (uintptr_t)&glUniform1iv },
		{ "glUniform2f", (uintptr_t)&glUniform2f },
		{ "glUniform2fv", (uintptr_t)&glUniform2fv },
		{ "glUniform2iv", (uintptr_t)&glUniform2iv },
		{ "glUniform3f", (uintptr_t)&glUniform3f },
		{ "glUniform3fv", (uintptr_t)&glUniform3fv },
		{ "glUniform3iv", (uintptr_t)&glUniform3iv },
		{ "glUniform4f", (uintptr_t)&glUniform4f },
		{ "glUniform4fv", (uintptr_t)&glUniform4fv },
		{ "glUniform4iv", (uintptr_t)&glUniform4iv },
		{ "glUniformMatrix2fv", (uintptr_t)&glUniformMatrix2fv },
		{ "glUniformMatrix3fv", (uintptr_t)&glUniformMatrix3fv },
		{ "glUniformMatrix4fv", (uintptr_t)&glUniformMatrix4fv },
		{ "glUseProgram", (uintptr_t)&glUseProgram },
		{ "glVertexAttrib4f", (uintptr_t)&glVertexAttrib4f },
		{ "glVertexAttribPointer", (uintptr_t)&glVertexAttribPointer },
		{ "glVertexPointer", (uintptr_t)&glVertexPointer },
		{ "glViewport", (uintptr_t)&glViewport },
		{ "glDrawArraysInstanced", (uintptr_t)&glDrawArraysInstanced },
		{ "glDrawElementsInstanced", (uintptr_t)&glDrawElementsInstanced },

		// By Raul
		{ "glGetShaderPrecisionFormat", (uintptr_t)&ret0 },
		// TODO: see (https://github.com/Rinnegatamante/mc3-vita/blob/fa877861195538b525cdd618f81b72f94ad2c319/source/dynlib.c#L150) 
		{ "glBlendColor", (uintptr_t)&glBlendColor_wrap },
		{ "glColor4ub", (uintptr_t)&glColor4ub },
		{ "glGetBufferParameteriv", (uintptr_t)&glGetBufferParameteriv },
		{ "glLoadIdentity", (uintptr_t)&glLoadIdentity },
		{ "glOrthof", (uintptr_t)&glOrthof },
		{ "glReleaseShaderCompiler", (uintptr_t)&glReleaseShaderCompiler },
		{ "glScalef", (uintptr_t)&glScalef },
		{ "glTexParameterfv", (uintptr_t)&glTexParameterfv_fake },
		{ "glTranslatef", (uintptr_t)&glTranslatef },
		{ "glVertexAttrib4fv", (uintptr_t)&glVertexAttrib4fv },
		//{"glBlendColor", (uintptr_t)&ret0},


		// EGL
		{ "eglBindAPI", (uintptr_t)&eglBindAPI },
		{ "eglChooseConfig", (uintptr_t)&eglChooseConfig },
		{ "eglCreateContext", (uintptr_t)&eglCreateContext },
		{ "eglCreateWindowSurface", (uintptr_t)&eglCreateWindowSurface },
		{ "eglDestroyContext", (uintptr_t)&eglDestroyContext },
		{ "eglDestroySurface", (uintptr_t)&eglDestroySurface },
		{ "eglGetConfigAttrib", (uintptr_t)&eglGetConfigAttrib },
		{ "eglGetDisplay", (uintptr_t)&eglGetDisplay },
		{ "eglGetError", (uintptr_t)&eglGetError },
		{ "eglGetProcAddress", (uintptr_t)&eglGetProcAddress },
		{ "eglInitialize", (uintptr_t)&eglInitialize },
		{ "eglMakeCurrent", (uintptr_t)&eglMakeCurrent },
		{ "eglQuerySurface", (uintptr_t)&eglQuerySurface },
		{ "eglSwapBuffers", (uintptr_t)&eglSwapBuffers },
		{ "eglTerminate", (uintptr_t)&eglTerminate },
		// By Raul
		{ "eglGetConfigs", (uintptr_t)&eglGetConfigs },
		{ "eglGetCurrentContext", (uintptr_t)&eglGetCurrentContext },
		{ "eglQueryString", (uintptr_t)&eglQueryString },
		{ "eglQueryContext", (uintptr_t)&eglQueryContext },
		{ "eglCreatePbufferSurface", (uintptr_t)&eglCreatePbufferSurface },


		// Pthread
		{ "pthread_attr_destroy", (uintptr_t)&pthread_attr_destroy_soloader },
		{ "pthread_attr_init", (uintptr_t) &pthread_attr_init_soloader },
		{ "pthread_attr_setdetachstate", (uintptr_t) &pthread_attr_setdetachstate_soloader },
		{ "pthread_attr_setstacksize", (uintptr_t) &pthread_attr_setstacksize_soloader },
		{ "pthread_cond_broadcast", (uintptr_t) &pthread_cond_broadcast_soloader },
		{ "pthread_cond_destroy", (uintptr_t) &pthread_cond_destroy_soloader },
		{ "pthread_cond_init", (uintptr_t) &pthread_cond_init_soloader },
		{ "pthread_cond_signal", (uintptr_t) &pthread_cond_signal_soloader },
		{ "pthread_cond_timedwait", (uintptr_t) &pthread_cond_timedwait_soloader },
		{ "pthread_cond_wait", (uintptr_t) &pthread_cond_wait_soloader },
		{ "pthread_create", (uintptr_t) &pthread_create_soloader },
		{ "pthread_detach", (uintptr_t) &pthread_detach_soloader },
		{ "pthread_equal", (uintptr_t) &pthread_equal_soloader },
		{ "pthread_exit", (uintptr_t)&pthread_exit },
		{ "pthread_getschedparam", (uintptr_t) &pthread_getschedparam_soloader },
		{ "pthread_getspecific", (uintptr_t)&pthread_getspecific },
		{ "pthread_join", (uintptr_t) &pthread_join_soloader },
		{ "pthread_key_create", (uintptr_t)&pthread_key_create },
		{ "pthread_key_delete", (uintptr_t)&pthread_key_delete },
		{ "pthread_kill", (uintptr_t)&pthread_kill_soloader },
		{ "pthread_mutex_destroy", (uintptr_t) &pthread_mutex_destroy_soloader },
		{ "pthread_mutex_init", (uintptr_t) &pthread_mutex_init_soloader },
		{ "pthread_mutex_lock", (uintptr_t) &pthread_mutex_lock_soloader },
		{ "pthread_mutex_trylock", (uintptr_t) &pthread_mutex_trylock_soloader },
		{ "pthread_mutex_unlock", (uintptr_t) &pthread_mutex_unlock_soloader },
		{ "pthread_mutexattr_destroy", (uintptr_t) &pthread_mutexattr_destroy_soloader },
		{ "pthread_mutexattr_init", (uintptr_t) &pthread_mutexattr_init_soloader },
		{ "pthread_mutexattr_settype", (uintptr_t) &pthread_mutexattr_settype_soloader },
		{ "pthread_once", (uintptr_t)&pthread_once_soloader },
		{ "pthread_self", (uintptr_t) &pthread_self_soloader },
		{ "pthread_setname_np", (uintptr_t) &pthread_setname_np_soloader },
		{ "pthread_setschedparam", (uintptr_t) &pthread_setschedparam_soloader },
		{ "pthread_setspecific", (uintptr_t)&pthread_setspecific },
		{ "pthread_sigmask", (uintptr_t)&ret0 },
		{ "pthread_attr_setstack", (uintptr_t) &pthread_attr_setstack_soloader },
		{ "pthread_getattr_np", (uintptr_t) &pthread_getattr_np_soloader },
		{ "pthread_attr_getstack", (uintptr_t) &pthread_attr_getstack_soloader },

		{ "sem_destroy", (uintptr_t) &sem_destroy_soloader },
		{ "sem_getvalue", (uintptr_t) &sem_getvalue_soloader },
		{ "sem_init", (uintptr_t) &sem_init_soloader },
		{ "sem_post", (uintptr_t) &sem_post_soloader },
		{ "sem_timedwait", (uintptr_t) &sem_timedwait_soloader },
		{ "sem_trywait", (uintptr_t) &sem_trywait_soloader },
		{ "sem_wait", (uintptr_t) &sem_wait_soloader },

		{ "sched_get_priority_max", (uintptr_t)&sched_get_priority_max },
		{ "sched_get_priority_min", (uintptr_t)&sched_get_priority_min },
		{ "sched_yield", (uintptr_t)&sched_yield },


		// wchar, wctype
		{ "btowc", (uintptr_t)&btowc },
		{ "iswalpha", (uintptr_t)&iswalpha },
		{ "iswcntrl", (uintptr_t)&iswcntrl },
		{ "iswctype", (uintptr_t)&iswctype },
		{ "iswdigit", (uintptr_t)&iswdigit },
		{ "iswdigit", (uintptr_t)&iswdigit },
		{ "iswlower", (uintptr_t)&iswlower },
		{ "iswprint", (uintptr_t)&iswprint },
		{ "iswpunct", (uintptr_t)&iswpunct },
		{ "iswspace", (uintptr_t)&iswspace },
		{ "iswupper", (uintptr_t)&iswupper },
		{ "iswxdigit", (uintptr_t)&iswxdigit },
		{ "towlower", (uintptr_t)&towlower },
		{ "towupper", (uintptr_t)&towupper },
		{ "wcrtomb", (uintptr_t)&wcrtomb },
		{ "wcscasecmp", (uintptr_t)&wcscasecmp },
		{ "wcscmp", (uintptr_t)&wcscmp },
		{ "wcscoll", (uintptr_t)&wcscoll },
		{ "wcsftime", (uintptr_t)&wcsftime },
		{ "wcslcat", (uintptr_t)&wcslcat },
		{ "wcslcpy", (uintptr_t)&wcslcpy },
		{ "wcslen", (uintptr_t)&wcslen },
		{ "wcsncasecmp", (uintptr_t)&wcsncasecmp },
		{ "wcsncpy", (uintptr_t)&wcsncpy },
		{ "wcsxfrm", (uintptr_t)&wcsxfrm },
		{ "wctob", (uintptr_t)&wctob },
		{ "wctype", (uintptr_t)&wctype },
		{ "wmemchr", (uintptr_t)&wmemchr },
		{ "wmemcmp", (uintptr_t)&wmemcmp },
		{ "wmemcpy", (uintptr_t)&wmemcpy },
		{ "wmemmove", (uintptr_t)&wmemmove },
		{ "wmemset", (uintptr_t)&wmemset },
		{ "mbrlen", (uintptr_t)&mbrlen },
		{ "mbrtowc", (uintptr_t)&mbrtowc },


		// libdl
		{ "dlclose", (uintptr_t)&ret0 },
		{ "dlerror", (uintptr_t)&ret0 },
		{ "dlopen", (uintptr_t)&dlopen_hook },
		{ "dlsym", (uintptr_t)&dlsym_fake },


		// Errno
		{ "__errno", (uintptr_t)&__errno_soloader },
		{ "strerror", (uintptr_t)&strerror_soloader },
		{ "strerror_r", (uintptr_t)&strerror_r_soloader },
		

		// Strings
		{ "memchr", (uintptr_t)&memchr },
		{ "memrchr", (uintptr_t)&memrchr },
		{ "strcasecmp", (uintptr_t)&strcasecmp },
		{ "strcat", (uintptr_t)&strcat },
		{ "strchr", (uintptr_t)&strchr },
		{ "strcmp", (uintptr_t)&strcmp },
		{ "strcoll", (uintptr_t)&strcoll },
		{ "strcpy", (uintptr_t)&strcpy },
		{ "strcspn", (uintptr_t)&strcspn },
		{ "strdup", (uintptr_t)&strdup },
		{ "strlcat", (uintptr_t)&strlcat },
		{ "strlcpy", (uintptr_t)&strlcpy },
		{ "strlen", (uintptr_t)&strlen },
		{ "strncasecmp", (uintptr_t)&strncasecmp },
		{ "strncat", (uintptr_t)&strncat },
		{ "strncmp", (uintptr_t)&strncmp },
		{ "strncpy", (uintptr_t)&strncpy },
		{ "strnlen", (uintptr_t)&strnlen },
		{ "strpbrk", (uintptr_t)&strpbrk },
		{ "strrchr", (uintptr_t)&strrchr },
		{ "strspn", (uintptr_t)&strspn },
		{ "strstr", (uintptr_t)&strstr },
		{ "strtok", (uintptr_t)&strtok },
		{ "strtok_r", (uintptr_t)&strtok_r },
		{ "strxfrm", (uintptr_t)&strxfrm },
		

		// Syscalls
		{ "syscall", (uintptr_t)&syscall },
		{ "sysconf", (uintptr_t)&ret0 },
		{ "system", (uintptr_t)&system },


		// Time
		{ "clock", (uintptr_t)&clock },
		//{ "clock_getres", (uintptr_t)&clock_getres },
		{ "clock_gettime", (uintptr_t)&clock_gettime_soloader },
		{ "difftime", (uintptr_t)&difftime },
		{ "gettimeofday", (uintptr_t)&gettimeofday },
		{ "gmtime", (uintptr_t)&gmtime },
		{ "gmtime_r", (uintptr_t)&gmtime_r },
		{ "localtime", (uintptr_t)&localtime },
		{ "localtime_r", (uintptr_t)&localtime_r },
		{ "mktime", (uintptr_t)&mktime },
		//{ "nanosleep", (uintptr_t)&nanosleep },
		{ "strftime", (uintptr_t)&strftime },
		{ "time", (uintptr_t)&time },
		{ "tzset", (uintptr_t)&tzset },
		{ "utimes", (uintptr_t)&utimes },


		// Temp
		{ "mkstemp", (uintptr_t)&mkstemp },
		{ "mktemp", (uintptr_t)&mktemp },
		{ "tmpfile", (uintptr_t)&tmpfile },
		{ "tmpnam", (uintptr_t)&tmpnam },


		// stdlib
		{ "abort", (uintptr_t)&abort },
		{ "atof", (uintptr_t)&atof },
		{ "atoi", (uintptr_t)&atoi },
		{ "atol", (uintptr_t)&atol },
		{ "atoll", (uintptr_t)&atoll },
		{ "exit", (uintptr_t)&exit_soloader },
		{ "lrand48", (uintptr_t)&lrand48 },
		{ "prctl", (uintptr_t)&ret0 },
		{ "sleep", (uintptr_t)&sleep },
		{ "srand48", (uintptr_t)&srand48 },
		{ "strtod", (uintptr_t)&strtod },
		{ "strtof", (uintptr_t)&strtof },
		{ "strtoimax", (uintptr_t)&strtoimax },
		{ "strtol", (uintptr_t)&strtol },
		{ "strtold", (uintptr_t)&strtold },
		{ "strtoll", (uintptr_t)&strtoll },
		{ "strtoul", (uintptr_t)&strtoul },
		{ "strtoull", (uintptr_t)&strtoull },
		{ "strtoumax", (uintptr_t)&strtoumax },
		{ "usleep", (uintptr_t)&usleep },
		{ "perror", (uintptr_t)&perror },

		#ifdef USE_SCELIBC_IO
			{ "qsort", (uintptr_t)&sceLibcBridge_qsort },
			{ "rand", (uintptr_t)&sceLibcBridge_rand },
			{ "srand", (uintptr_t)&sceLibcBridge_srand },
		#else
			{ "qsort", (uintptr_t)&qsort },
			{ "rand", (uintptr_t)&rand },
			{ "srand", (uintptr_t)&srand },
		#endif


		// Env
		{ "getenv", (uintptr_t)&getenv_soloader },
		{ "setenv", (uintptr_t)&setenv_soloader },
		{ "unsetenv", (uintptr_t)&unsetenv_soloader },


		// Jmp
		{ "setjmp", (uintptr_t)&setjmp }, // TODO: May have different struct size?
		{ "longjmp", (uintptr_t)&longjmp }, // TODO: May have different struct size?


		// Signals
		{ "bsd_signal", (uintptr_t)&signal },
		{ "raise", (uintptr_t)&raise },
		{ "sigaction", (uintptr_t)&sigaction },
		
		
		// Locale
		{ "setlocale", (uintptr_t)&setlocale },


		// zlib
		{ "crc32", (uintptr_t)&crc32 },
		{ "gzopen", (uintptr_t)&gzopen },
		{ "gzgets", (uintptr_t)&gzgets },
		{ "gzclose", (uintptr_t)&gzclose },
		{ "compressBound", (uintptr_t)&compressBound },
		{ "compress", (uintptr_t)&compress },
		{ "uncompress", (uintptr_t)&uncompress },
		{ "deflateInit_", (uintptr_t)&deflateInit_ },
		{ "deflate", (uintptr_t)&deflate },
		{ "deflateEnd", (uintptr_t)&deflateEnd },
		{ "inflateInit_", (uintptr_t)&inflateInit_ },
		{ "inflate", (uintptr_t)&inflate },
		{ "inflateEnd", (uintptr_t)&inflateEnd },
		{ "inflateInit2_", (uintptr_t)&inflateInit2_ },
		{ "inflateReset", (uintptr_t)&inflateReset },

		// Android cpu features
		//{ "android_getCpuFeatures", (uintptr_t)&android_getCpuFeatures },

		// Misc
		{ "app_dummy", (uintptr_t)&app_dummy },

		// Audio
		{ "alBufferData", (uintptr_t)&alBufferData },
		{ "alIsExtensionPresent", (uintptr_t)&alIsExtensionPresent },
		{ "alGetProcAddress", (uintptr_t)&alGetProcAddress },
		{ "alcGetProcAddress", (uintptr_t)&alcGetProcAddress },
		{ "alGenSources", (uintptr_t)&alGenSources },
		{ "alSourcei", (uintptr_t)&alSourcei },
		{ "alDeleteBuffers", (uintptr_t)&alDeleteBuffers },
		{ "alGenBuffers", (uintptr_t)&alGenBuffers },
		{ "alBufferData", (uintptr_t)&alBufferData },
		{ "alSource3f", (uintptr_t)&alSource3f },
		{ "alSourcef", (uintptr_t)&alSourcef },
		{ "alGetSourcei", (uintptr_t)&alGetSourcei },
		{ "alDeleteSources", (uintptr_t)&alDeleteSources },
		{ "alSourcefv", (uintptr_t)&alSourcefv },
		{ "alSourcePlay", (uintptr_t)&alSourcePlay },
		{ "alSourcePause", (uintptr_t)&alSourcePause },
		{ "alSourceStop", (uintptr_t)&alSourceStop },
		{ "alGetBufferi", (uintptr_t)&alGetBufferi },
		{ "alGetSourcef", (uintptr_t)&alGetSourcef },
		{ "alSourceUnqueueBuffers", (uintptr_t)&alSourceUnqueueBuffers },
		{ "alSourceQueueBuffers", (uintptr_t)&alSourceQueueBuffers_safe },
		{ "alListenerfv", (uintptr_t)&alListenerfv },
		{ "alListener3f", (uintptr_t)&alListener3f },
		{ "alDopplerFactor", (uintptr_t)&alDopplerFactor },
		{ "alcSuspendContext", (uintptr_t)&alcSuspendContext },
		{ "alcMakeContextCurrent", (uintptr_t)&alcMakeContextCurrent },
		{ "alcProcessContext", (uintptr_t)&alcProcessContext },
		{ "alcOpenDevice", (uintptr_t)&alcOpenDevice },
		{ "alcCreateContext", (uintptr_t)&alcCreateContext },
		{ "alcGetError", (uintptr_t)&alcGetError },
		{ "alcCloseDevice", (uintptr_t)&alcCloseDevice },



		{ "fabs", (uintptr_t)&fabs },
		{ "isatty", (uintptr_t)&isatty },
		{ "llrint", (uintptr_t)&llrint },
		{ "bsearch", (uintptr_t)&bsearch },
		{ "__assert2", (uintptr_t)&assert2},
};

void resolve_imports(so_module* mod) {
	__sF_fake[0] = *stdin;
	__sF_fake[1] = *stdout;
	__sF_fake[2] = *stderr;

	so_resolve(mod, default_dynlib, sizeof(default_dynlib), 0);
}