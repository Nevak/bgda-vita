/*
 * main.c
 *
 * ARMv7 Shared Libraries loader. Baldur's Gate Dark Alliance Edition
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021-2023 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/init.h"
#include "utils/glutil.h"
#include "utils/settings.h"
#include "utils/logger.h"

#include <psp2/kernel/threadmgr.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>

#include <AFakeNative/AFakeNative.h>
#include <AFakeNative/utils/controls.h>
#include <vitasdk.h>
#include <stdio.h>
#include <string.h>
#include <psp2/gxm.h>
#ifdef PROFILER_ENABLED
#include "utils/prof.h"
#include <profilerino.h>
#endif

#include "utils/vorbis_patch.h"

__attribute__((__no_instrument_function__, __no_profile_instrument_function__))
void *__wrap_calloc(uint32_t nmember, uint32_t size) { return vglCalloc(nmember, size); }

int total_malloc_calls_in_frame = 0;
int total_free_calls_in_frame = 0;
uint64_t total_allocated_memory_in_frame = 0;
uint64_t total_time_taken_by_allocs_in_frame_us = 0;

__attribute__((__no_instrument_function__, __no_profile_instrument_function__))
void __wrap_free(void *addr) { 
	total_free_calls_in_frame++;
	vglFree(addr); 
};

__attribute__((__no_instrument_function__, __no_profile_instrument_function__))
void *__wrap_malloc(uint32_t size) { 

	uint64_t time_start = sceKernelGetProcessTimeWide();
	//Profiler_BeginSample("malloc");
	void * r = vglMalloc(size); 
	uint64_t time_end = sceKernelGetProcessTimeWide();

	total_time_taken_by_allocs_in_frame_us += (time_end - time_start);
	total_allocated_memory_in_frame += size;
	total_malloc_calls_in_frame++;

	// check if successful
	if (r == NULL) {
		logv_error("malloc(%d) failed", size);
	}

	//Profiler_EndSample();
	return r;
};

__attribute__((__no_instrument_function__, __no_profile_instrument_function__))
void *__wrap_memalign(uint32_t alignment, uint32_t size) { return vglMemalign(alignment, size); };
__attribute__((__no_instrument_function__, __no_profile_instrument_function__))
void *__wrap_realloc(void *ptr, uint32_t size) 
{ 
	logv_debug("realloc(%p, %d)", ptr, size);
	return vglRealloc(ptr, size); 
};

__attribute__((__no_instrument_function__, __no_profile_instrument_function__))
void *__wrap_memcpy (void *dst, const void *src, size_t num) { return sceClibMemcpy(dst, src, num); };
__attribute__((__no_instrument_function__, __no_profile_instrument_function__))
void *__wrap_memset (void *ptr, int value, size_t num) { return sceClibMemset(ptr, value, num); };


int _newlib_heap_size_user = 330 * 1024 * 1024;

#ifdef USE_SCELIBC_IO
int sceLibcHeapSize = 1 * 1024 * 1024;
#endif 

so_module so_mod;
so_module so_mod_libcpufeatues;
so_module so_mod_jbejni;
so_module so_mod_libcpp;
so_module so_mod_libxmv;

SceCtrlData pad_previous;

#define DEFAULT_RAZOR_CAPTURE_PATH "ur0:data/librazorcapture_es4.suprx"

int log_allocs = 0;
int log_profiler = 0;
int profiling_idx = 0;

bool enable_cheats = 0;

float g_uvFactor = 1.0f;
float g_uvFactorY = 1.0f;
int input_thread_fn(SceSize args, void *argp) {
	//log_error("Polling input");

	while (1) {
		// poll input
		//log_error("Polling input");
		sceKernelDelayThread(1000);
		SceCtrlData pad;
		sceCtrlPeekBufferPositiveExt2(0, &pad, 1);
	
		if (pad.buttons & SCE_CTRL_L1 
			&& pad.buttons & SCE_CTRL_R1 
			&& pad.buttons & SCE_CTRL_LEFT 
			&& pad.buttons & SCE_CTRL_TRIANGLE) {
			enable_cheats = true;
		}
		else {
			enable_cheats = false;
		}



		// if (pad.buttons & SCE_CTRL_R1 && !(pad_previous.buttons & SCE_CTRL_R1)) {
		// 	log_profiler = !log_profiler;
		// 	if (log_profiler) {
		// 		sceClibPrintf("Starting profiling\n");
		// 		//gprof_start();
		// 	} else {
		// 		sceClibPrintf("Stopping profiling\n");
		// 		//char fname[256];
		// 		//sprintf(fname, "ux0:data/prof_%d.out", profiling_idx++);
		// 		//gprof_stop(fname, 1);
		// 	}
		// }
		// if (pad.buttons & SCE_CTRL_R1) {
		// 	g_uvFactor -= 0.001f;
		// 	logv_error("g_uvFactor: %f\n", g_uvFactor);
		// }

		// if (pad.buttons & SCE_CTRL_LEFT ) {
		// 	g_uvFactorY += 0.001f;
		// 	logv_error("g_uvFactorY: %f\n", g_uvFactor);
		// }
		// if (pad.buttons & SCE_CTRL_RIGHT) {
		// 	g_uvFactorY -= 0.001f;
		// 	logv_error("g_uvFactorY: %f\n", g_uvFactor);
		// }
		// if (pad.buttons & SCE_CTRL_CIRCLE) {
		// 	g_uvFactor = 1.0f;
		// 	g_uvFactorY = 1.0f;
		// }
		pad_previous = pad;
	}

	return 0;
}


int main() {
	//gprof_stop("ux0:/data/gmon.out", 0);
#ifdef PROFILER_ENABLED
	log_error("profilerino_init!");
	profilerino_init();
#endif

	SceAppUtilInitParam appUtilParam;
	SceAppUtilBootParam appUtilBootParam;
	memset(&appUtilParam, 0, sizeof(SceAppUtilInitParam));
	memset(&appUtilBootParam, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&appUtilParam, &appUtilBootParam);
	
	soloader_init_all();



    int (*JNI_OnLoad)(JavaVM* jvm) = (void*)so_symbol(&so_mod_jbejni, "JNI_OnLoad");

	int (*ANativeActivity_onCreate)(ANativeActivity *activity, void *savedState,
		size_t savedStateSize) = (void *) so_symbol(&so_mod_jbejni, "ANativeActivity_onCreate");

	ANativeActivity *activity = ANativeActivity_create();
	init_jni_fields(activity->env);

	log_info("Created NativeActivity object");

	ANativeActivity_onCreate(activity, NULL, 0);
	log_info("ANativeActivity_onCreate() passed");

	activity->callbacks->onStart(activity);
	log_info("onStart() passed");

	AInputQueue *aInputQueue = AInputQueue_create();
	activity->callbacks->onInputQueueCreated(activity, aInputQueue);
	log_info("onInputQueueCreated() passed");

	JNI_OnLoad(&jvm);

	ANativeWindow *aNativeWindow = ANativeWindow_create();
	activity->callbacks->onNativeWindowCreated(activity, aNativeWindow);
	log_info("onNativeWindowCreated() passed");

	activity->callbacks->onWindowFocusChanged(activity, 1);
	log_info("onWindowFocusChanged() passed");


	
	void (*inputDeviceAdded)(JNIEnv *env, jclass clazz, jint device_id, jint device_type) = (void *) so_symbol(&so_mod_jbejni, "Java_com_jbe_Activity_inputDeviceAdded");
	if (inputDeviceAdded == NULL)
	{
		log_error("inputDeviceAdded is NULL");
	}
	else
	{
		/*
		enum Device {
			NONE,           // 0
			SIXAXIS,        // 1
			XB360,          // 2
			XB360_GENERIC,  // 3
			WII,            // 4
			NYKO_PLAYPAD,       // 5
			NYKO_PLAYPAD_PRO,   // 6
			OUYA,               // 7
			MOGA_PRO_HID,       // 8
			BROADCOM_HID,       // 9
			RED_SAMURAI,        // 10
			SHIELD,             // 11
			MOJO,               // 12
			AMAZON,             // 13
			NEXUS_PLAYER,       // 14
			PS4,                // 15
			FORGE_SERVAL,       // 16
			UNKNOWN,            // 17
			COUNT               // 18
		}
		*/

		int num_detected = detectControllers();
		int res = sceCtrlIsMultiControllerSupported();
		logv_error("sceCtrlIsMultiControllerSupported = %d", res);
		logv_error("Detected %d controller(s)", num_detected);

		for (int i = 0; i < num_detected; i++) {
			inputDeviceAdded(&jni, (void *)0x42424242, i, 2);
			logv_error("Registered controller %d", i);
		}
	}

	// poll input in another thread
	SceUID input_thread = sceKernelCreateThread("input_thread", &input_thread_fn, 0x10000100, 0x10000, 0, 0, NULL);
	 sceKernelStartThread(input_thread, 0, NULL);


	log_info("Main thread shutting down");
	
	sceKernelExitDeleteThread(0);
}