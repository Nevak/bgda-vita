/*
 * Patching some of the .so internal functions or bridging them to native for
 * better compatibility.
 *
 * Copyright (C) 2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "patch.h"

#include <kubridge.h>
#include <so_util/so_util.h>
#include <stdint.h>
#include <utils/trophies.h>

#include <stdio.h>
#include <vitasdk.h>
#include <libsysmodule.h>
#include <libperf.h>
#include <vitaGL.h>
#include "utils/logger.h"

#include "utils/macros.h"
#include "utils/existing_files.h"

#include "patches/bgda_types.h"
#include "patches/frustum_culling.h"
#include "patches/texture_decomp.h"
#include "patches/texture_palette.h"
#include "patches/worldAllocateSegments.h"
#include "patches/touch_ui.h"

#include "patches/usprintf.h"
#include "patches/write_render_command.h"
#include "patches/memory.h"

#ifdef PROFILER_ENABLED
#include <utils/prof.h>
#include "patches/profiler_hooks.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern so_module so_mod;
extern so_module so_mod_libxmv;

#ifdef __cplusplus
};
#endif

#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <arm_neon.h>

extern int log_profiler;

// Global accumulator for cdProcess time tracking
static float g_cdProcessTotalMs = 0.0f;
static float g_MC_LoadLevelEntitiesMs = 0.0f;
float g_ProcessAndUploadTextureMs = 0.0f;  // Non-static because it's extern'd in texture_palette.h
static float g_lumpLoadTotalMs = 0.0f;
static float g_lumpLoadGlobTotalMs = 0.0f;
static float g_worldAllocateSegmentsMs = 0.0f;
static float g_worldResetMs = 0.0f;
static float g_gameClearMs = 0.0f;
static float g_SND_StartStreamMs = 0.0f;
 float g_D3DDevice_CreateTexture2Ms = 0.0f;
static float g_D3DDevice_CreatePalette2Ms = 0.0f;
static float g_D3DTexture_LockRectMs = 0.0f;
static float g_D3DTexture_UnlockRectMs = 0.0f;
static float g_machHostOpenMs = 0.0f;
static float g_machHostReadMs = 0.0f;
static float g_machHostSeekMs = 0.0f;
static float g_machHostCloseMs = 0.0f;

int ret0() { return 0; }
int ret1() { return 1; }


extern void renderDelayedShadows(void);


so_hook coreAddTask_hook;
int coreAddTask(void *fn, int prio, char *name) {
	logv_debug("coreAddTask(%p, %i, %s)", fn, prio, name);
	// ignore if task is StatCache
	if (name && strcmp(name, "StatCache") == 0) {
		//logv_error("Ignoring StatCache task\n");
		return 0;
	}

	if (name && strcmp(name, "RenderDelayedShadows") == 0) {
		//log_error("Ignoring renderDelayedShadows task\n");
		// Disable shadows for now since they are a bit expensive. Will optimize later
	    return 0;
	}

    return SO_CONTINUE(int, coreAddTask_hook, fn, prio, name);
}


so_hook writeConfigDirect_hook;
void writeConfigDirect() {}

int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

so_hook cdDirectoryLookup_hook;
int cdDirectoryLookup(const char *path, int *param_2, int *size) {
	logv_debug("->cdDirectoryLookup (%s, 0x%X, 0x%X)", path, param_2, size);
	uint64_t timeNow = sceKernelGetProcessTimeWide();
    int returnVal=0;
	
    if (!strcmp(path, "cellar1.vat")) {
        if (param_2 != 0) {
            *param_2 = -1;
        }
    
        if (size != 0){
            *size = 0x1B3D00;
        }

        logv_error("cdDirectoryLookup: HARDCODED! %s", path);

        returnVal = 1;
    }
    else {
        char real_fname_ptr[0x40];
        
        snprintf(real_fname_ptr, 0x40, "/res/%s", path);
        const char *key = real_fname_ptr;
        char **existing_file = (char **)bsearch(&key, existing_files, existing_files_len, sizeof(char *), compare_strings);
        if (existing_file == NULL) {
            logv_warn("cdDirectoryLookup: file not found inside the existing_files list!!! %s", real_fname_ptr);
            returnVal = 0;
        }
        else {
            returnVal = SO_CONTINUE(int, cdDirectoryLookup_hook, path, param_2, size);
        }
    }

    int x_2;
    if (param_2 != 0) {
        x_2 = *param_2;
    }
    int x_3;
    if (size != 0) {
        x_3 = *size;
    }

    uint64_t timeAfter = sceKernelGetProcessTimeWide();
    float elapsedMs = (timeAfter - timeNow) / 1000.0f;
    logv_debug("->cdDirectoryLookup took %f ms, result=0x%X, param_2=0x%X, size=0x%X", elapsedMs, returnVal, x_2, x_3);
	return returnVal;
}

so_hook D3DDevice_SetTextureStages_hook;
void D3DDevice_SetTextureStages(uint8_t *param_1, uint32_t param_2) {
	SO_CONTINUE(void *, D3DDevice_SetTextureStages_hook, param_1, param_2);
}

static uint32_t prev_vcount = 0;

void init_vblank_counter(void) {
    prev_vcount = sceDisplayGetVcount();
}


int compute_lastFrameVBlankCount(void) {
    uint32_t now = sceDisplayGetVcount();
    uint32_t delta = now - prev_vcount;   // wraps naturally for uint32_t
    prev_vcount = now;

    // If one vblank per rendered frame is "normal", extras mean catch-up:
    if (delta == 0) return 0;
    return (int)(delta - 1);
}

so_hook machFrameStart_hook;
void machFrameStart(int p) {
	sceKernelChangeThreadCpuAffinityMask(sceKernelGetThreadId(), SCE_KERNEL_CPU_MASK_USER_1);


	SO_CONTINUE(void *, machFrameStart_hook, p);
}

extern int total_malloc_calls_in_frame;
extern int total_free_calls_in_frame;
extern uint64_t total_allocated_memory_in_frame;
extern uint64_t total_time_taken_by_allocs_in_frame_us;


so_hook mach_frameEnd_hook;
void machFrameEnd(int param_1) {
	SO_CONTINUE(void *, mach_frameEnd_hook, param_1);
	
	int lastFrameVBlankCount = compute_lastFrameVBlankCount();
	if (lastFrameVBlankCount <= 0) {
		lastFrameVBlankCount = 1;
	}

	// if (lastFrameVBlankCount > 1) {
	// 	logv_debug("machFrameStart: lastFrameVBlankCount=%d", lastFrameVBlankCount);
	// }

	// write to global variable at 0029e5c4 using LOC() macro
	// uint32_t *g_lastFrameVBlankCount = (uint32_t *)LOC(0x0029e5c4);
	// *g_lastFrameVBlankCount = (uint32_t)lastFrameVBlankCount;

	if (total_malloc_calls_in_frame == 0 && total_free_calls_in_frame == 0) {
		// reset counters
		total_malloc_calls_in_frame = 0;
		total_free_calls_in_frame = 0;
		total_allocated_memory_in_frame = 0;
		total_time_taken_by_allocs_in_frame_us = 0;
		return;
	}
	// logv_debug("machFrameEnd: malloc calls: %d, free calls: %d, total allocated memory: %llu bytes, total time in allocs: %llu us",
	// 	total_malloc_calls_in_frame,
	// 	total_free_calls_in_frame,
	// 	total_allocated_memory_in_frame,
	// 	total_time_taken_by_allocs_in_frame_us
	// );

	// reset counters
	total_malloc_calls_in_frame = 0;
	total_free_calls_in_frame = 0;
	total_allocated_memory_in_frame = 0;
	total_time_taken_by_allocs_in_frame_us = 0;
}

so_hook JBE_D3DDevice_Swap_hook;
void JBE_D3DDevice_Swap(void *param_1, int param_2) {
	SO_CONTINUE(void *, JBE_D3DDevice_Swap_hook, param_1, param_2);
}

so_hook DisplayPF_Swap_hook;
void DisplayPF_Swap(void *param_1) {
	SO_CONTINUE(void *, DisplayPF_Swap_hook, param_1);
}

//D3DDevice_AsyncRenderCB_hook
so_hook D3DDevice_AsyncRenderCB_hook;
uintptr_t D3DDevice_ReadCommand_addr;
uintptr_t g_Singleton_addr;
uintptr_t displayPF_AcquireContext_addr;
uintptr_t displayPF_ReleaseContext_addr;

struct astruct {
    uint32_t field0;           // offset 0x0
    /* … */
};

struct d3dDeviceFake {
	int dummy;          
	int dummy2;        
    struct astruct *cmd;      
    /* … other fields … */
};
_Static_assert(offsetof(struct d3dDeviceFake, cmd) == 0x8,"`cmd` is not at offset 0x8 – fix the struct or add packed!");

typedef struct {
	char  _pad0[0x92C];       /* 0x000 ‑‑ 0x92B : unknown / base members   */
    int   hasRenderWork;      /* 0x92C */
    char  _pad1[0x1C];        /* 0x930 ‑‑ 0x94B : more unknown members     */
    int* pSemaphore_Main;    // at offset 0x94C in your device struct
    // …
} D3DDevice;

// Typedefs for function pointers
typedef void (*AcquireContextFn)(void* display);
typedef void (*ReleaseContextFn)(void* display);
typedef void (*ReadCommandFn)(D3DDevice* dev);

ReadCommandFn fnReadCommand;
void ReadCommandCustom(struct d3dDeviceFake *thisPtr) {	
	// Call the original ReadCommand function
	fnReadCommand(thisPtr);
}

void D3DDevice_AsyncRenderCB(void *device_ptr) {
	//init_vblank_counter();

	uint32_t threadId = sceKernelGetThreadId();
	logv_debug("[0x%X] ===============D3DDevice_AsyncRenderCB================\n", threadId);
	sceKernelChangeThreadPriority(threadId, 100);
	sceKernelChangeThreadCpuAffinityMask(threadId, SCE_KERNEL_CPU_MASK_USER_0);
	pthread_setname_np_soloader(threadId, "D3DDevice_AsyncRenderCB");

    D3DDevice* device = (D3DDevice*)device_ptr;
	logv_debug("g_Singleton_addr is: %p", (void*)g_Singleton_addr);
    void* display = (void*)((char*)(g_Singleton_addr) + 0x10);
	
	logv_debug("DisplayPF pointer is: %p", display);

    // Cast function pointer types
    AcquireContextFn fnAcquire = (AcquireContextFn)displayPF_AcquireContext_addr;
    ReleaseContextFn fnRelease = (ReleaseContextFn)displayPF_ReleaseContext_addr;
    fnReadCommand = (ReadCommandFn)D3DDevice_ReadCommand_addr;

	log_debug("will call fnAcquire");
    // Acquire Render Context
    fnAcquire(display);
	log_debug("fnAcquire finished");

    // Wait on semaphore until success (retries on EINTR)
	unsigned char * rawPtr = (unsigned char *)device_ptr;
	logv_debug("device addr: %p", device);
	logv_debug("rawPtr: %p", rawPtr);
	logv_debug("device->pSemaphore_Main: %p", device->pSemaphore_Main);
	logv_debug("addr of device->pSemaphore_Main: %p", (void*)&device->pSemaphore_Main);
    while (sem_wait_soloader(device->pSemaphore_Main) != 0) {
        // optional: check errno if needed
    }

	log_debug("sem_wait finished");

	struct d3dDeviceFake *thisPtr = (struct d3dDeviceFake *)device;
    // Process commands while still work remains
    while (device->hasRenderWork) {
#ifdef PROFILER_ENABLED
		struct astruct *cmdData = thisPtr->cmd;
		uint8_t cmdType = cmdData->field0 & 0xFF;
		uint32_t cmdType32 = cmdData->field0 & 0xFF;
		const char *label = gCmdLabels[cmdType];
		sceRazorCpuPushMarkerWithHud(label, SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
		// if (res != 0) {
		// 	logv_error("sceRazorCpuPushMarkerWithHud failed: %d", res);
		// }
		// Just read the command without profiling
		fnReadCommand(thisPtr);

		// if (cmdType == 0x0C)
		// {
		// 	sceRazorCpuSync();
		// }

		sceRazorCpuPopMarker();
#else
		// Just read the command without profiling
		fnReadCommand(thisPtr);
#endif
    }

    // Release context when done
    fnRelease(display);
}

so_hook System_BeginFrame_hook;
void System_BeginFrame(void *param_1) {
	// This is never called anyway
	logv_debug("System_BeginFrame(%p)\n", param_1);
	SO_CONTINUE(void *, System_BeginFrame_hook, param_1);
	log_debug("System_BeginFrame finished\n");
}

so_hook D3DDevice_ReadCommand_hook;
void D3DDevice_ReadCommand(struct d3dDeviceFake *thisPtr) {
	if (log_profiler) {
		// struct astruct *cmdData = thisPtr->cmd;
		// uint8_t cmdType = cmdData->field0 & 0xFF;
		// uint32_t cmdType32 = cmdData->field0 & 0xFF;
		// // Profile with the command type
		// const char *label = gCmdLabels[cmdType];
		// // if (!label) {
		// // 	logv_error("D3DDevice_ReadCommand: cmdType is NULL (0x%02X)\n", cmdType);
		// // 	label = "Unknown Command";
		// // }
		// Profiler_BeginSample(label);

		SO_CONTINUE(void *, D3DDevice_ReadCommand_hook, thisPtr);
		// Profiler_EndSample();
	}
	else {
		SO_CONTINUE(void *, D3DDevice_ReadCommand_hook, thisPtr);
	}
}

so_hook TrackScheduler_ThreadProcCB_hook;
void TrackScheduler_ThreadProcCB(void *param_1) {
	int threadId = sceKernelGetThreadId();
	logv_error("TrackScheduler_ThreadProcCB(%p), threadId: %d)\n", param_1, threadId);
	int r = sceKernelChangeThreadCpuAffinityMask(threadId, SCE_KERNEL_CPU_MASK_USER_2);
	if (r < 0) {
		logv_error("TrackScheduler_ThreadProcCB: sceKernelChangeThreadCpuAffinityMask failed: %d\n", (r));
	}

	SO_CONTINUE(void *, TrackScheduler_ThreadProcCB_hook, param_1);
	log_error("TrackScheduler_ThreadProcCB finished\n");
}

so_hook D3DDevice_RegisterTextureCommand_hook;
void D3DDevice_RegisterTextureCommand(void *pThis, int *param_2, int *param_3, int *param_4, int* param_5) {
	logv_error("D3DDevice_RegisterTextureCommand(%p, %p, %p, %p)\n", pThis, param_2, param_3, param_4, param_5);
	SO_CONTINUE(void *, D3DDevice_RegisterTextureCommand_hook, pThis, param_2, param_3, param_4, param_5);
}

so_hook D3DDevice_Swap_hook;
void D3DDevice_Swap(int flags) {
	SO_CONTINUE(void *, D3DDevice_Swap_hook, flags);
}

so_hook renderDelayedShadows_hook;


so_hook runObjects_hook;
void runObjects(void) {
	SO_CONTINUE(void *, runObjects_hook);
}
so_hook drawObjects_hook;
void drawObjects(void) {
	SO_CONTINUE(void *, drawObjects_hook);
}

// _ZN3JBE9D3DDevice8GetFVFVSEPNS0_24FVFVertexShaderContainerERm
so_hook D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm_hook;
int D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm(void *thisptr, uintptr_t* container, uint32_t param_2) {
	int result = SO_CONTINUE(int, D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm_hook, thisptr, container, param_2);
	return result;
}

// _ZN14D3DBaseTexture11BufferToOGLEP21RegisteredTextureDataPKvi 
so_hook D3DBaseTexture_BufferToOGL_hook;
void D3DBaseTexture_BufferToOGL(void *pThis, void *pTexData, const void *pBuffer, int size) {
	SO_CONTINUE(void *, D3DBaseTexture_BufferToOGL_hook, pThis, pTexData, pBuffer, size);
}

void DoNothing()
{
}

// so_hook memAlloc_hook;
// void* memAlloc(int size, const char* name) {
// 	if (name && name[0] != '\0') {
// 		logv_error("memAlloc(%d, \"%s\")", size, name);
// 	}
// 	void* ret = SO_CONTINUE(void*, memAlloc_hook, size, name);
// 	if (name && name[0] != '\0') {
// 		logv_error("ret=%p", ret);
// 	}
// 	return ret;
// }

so_hook cdStartStream_hook;
void cdStartStream(char *filename, int param_2) {
	logv_error("cdStartStream(\"%s\", %d)", filename ? filename : "NULL", param_2);
	SO_CONTINUE(void*, cdStartStream_hook, filename, param_2);
	log_error("cdStartStream completed");
}

so_hook cdStreamLoad_hook;
void cdStreamLoad(char *filename, int param_2) {
	logv_error("[%d] cdStreamLoad(%d)", sceKernelGetThreadId(), param_2);
	SO_CONTINUE(void*, cdStreamLoad_hook, filename, param_2);
	log_error("cdStreamLoad completed");
}

so_hook gameLoadWorld_hook;
void gameLoadWorld(char *worldName) {

	logv_error("[0x%X] Entered gameLoadWorld: %s", sceKernelGetThreadId(), worldName);

	// CRITICAL: Wait for GPU to finish all pending operations before loading new level
	// This prevents sync object corruption and use-after-free crashes
	log_error("Waiting for GPU to finish before loading level...");
	glFinish();  // Wait for all OpenGL commands to complete
	log_error("GPU finished, proceeding with level load");

	// Reset accumulators
	g_cdProcessTotalMs = 0.0f;
	g_MC_LoadLevelEntitiesMs = 0.0f;
	g_ProcessAndUploadTextureMs = 0.0f;
	g_lumpLoadTotalMs = 0.0f;
	g_lumpLoadGlobTotalMs = 0.0f;
	g_worldAllocateSegmentsMs = 0.0f;
	g_worldResetMs = 0.0f;
	g_gameClearMs = 0.0f;
	g_SND_StartStreamMs = 0.0f;
	g_D3DDevice_CreateTexture2Ms = 0.0f;
	g_D3DDevice_CreatePalette2Ms = 0.0f;
	g_D3DTexture_LockRectMs = 0.0f;
	g_D3DTexture_UnlockRectMs = 0.0f;
	g_machHostOpenMs = 0.0f;
	g_machHostReadMs = 0.0f;
	g_machHostSeekMs = 0.0f;
	g_machHostCloseMs = 0.0f;

	uint64_t timeStart = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, gameLoadWorld_hook, worldName);

	logv_error("Ended gameLoadWorld: %s", worldName);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

	logv_error("gameLoadWorld('%s') took %.2f ms (%.2f seconds)\n", worldName, elapsedMs, elapsedMs / 1000.0f);
	logv_error("  -> worldAllocateSegments: %.2f ms (%.1f%%)\n", g_worldAllocateSegmentsMs, (g_worldAllocateSegmentsMs / elapsedMs) * 100.0f);
	logv_error("    -> machHostOpen: %.2f ms (%.1f%%)\n", g_machHostOpenMs, (g_machHostOpenMs / elapsedMs) * 100.0f);
	logv_error("    -> machHostRead: %.2f ms (%.1f%%)\n", g_machHostReadMs, (g_machHostReadMs / elapsedMs) * 100.0f);
	logv_error("    -> machHostSeek: %.2f ms (%.1f%%)\n", g_machHostSeekMs, (g_machHostSeekMs / elapsedMs) * 100.0f);
	logv_error("    -> machHostClose: %.2f ms (%.1f%%)\n", g_machHostCloseMs, (g_machHostCloseMs / elapsedMs) * 100.0f);
	logv_error("    -> D3DDevice_CreateTexture2: %.2f ms (%.1f%%)\n", g_D3DDevice_CreateTexture2Ms, (g_D3DDevice_CreateTexture2Ms / elapsedMs) * 100.0f);
	logv_error("    -> D3DDevice_CreatePalette2: %.2f ms (%.1f%%)\n", g_D3DDevice_CreatePalette2Ms, (g_D3DDevice_CreatePalette2Ms / elapsedMs) * 100.0f);
	logv_error("    -> D3DTexture_LockRect: %.2f ms (%.1f%%)\n", g_D3DTexture_LockRectMs, (g_D3DTexture_LockRectMs / elapsedMs) * 100.0f);
	logv_error("    -> D3DTexture_UnlockRect: %.2f ms (%.1f%%)\n", g_D3DTexture_UnlockRectMs, (g_D3DTexture_UnlockRectMs / elapsedMs) * 100.0f);
	logv_error("  -> worldReset: %.2f ms (%.1f%%)\n", g_worldResetMs, (g_worldResetMs / elapsedMs) * 100.0f);
	logv_error("  -> gameClear: %.2f ms (%.1f%%)\n", g_gameClearMs, (g_gameClearMs / elapsedMs) * 100.0f);
	logv_error("  -> SND_StartStream: %.2f ms (%.1f%%)\n", g_SND_StartStreamMs, (g_SND_StartStreamMs / elapsedMs) * 100.0f);
	logv_error("  -> lumpLoad: %.2f ms (%.1f%%)\n", g_lumpLoadTotalMs, (g_lumpLoadTotalMs / elapsedMs) * 100.0f);
	logv_error("  -> lumpLoadGlob: %.2f ms (%.1f%%)\n", g_lumpLoadGlobTotalMs, (g_lumpLoadGlobTotalMs / elapsedMs) * 100.0f);
	logv_error("  -> cdProcess: %.2f ms (%.1f%%)\n", g_cdProcessTotalMs, (g_cdProcessTotalMs / elapsedMs) * 100.0f);
	logv_error("  -> MC_LoadLevelEntities: %.2f ms (%.1f%%)\n", g_MC_LoadLevelEntitiesMs, (g_MC_LoadLevelEntitiesMs / elapsedMs) * 100.0f);
	logv_error("  -> ProcessAndUploadTexture: %.2f ms (%.1f%%)\n", g_ProcessAndUploadTextureMs, (g_ProcessAndUploadTextureMs / elapsedMs) * 100.0f);
}

so_hook cdProcess_hook;
void cdProcess(int param_1) {
	uint64_t timeStart = sceKernelGetProcessTimeWide();

	SO_CONTINUE(void*, cdProcess_hook, param_1);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

	// Accumulate time
	g_cdProcessTotalMs += elapsedMs;
}

so_hook lumpLoad_hook;
int lumpLoad(char *lumpName) {
	uint64_t timeStart = sceKernelGetProcessTimeWide();

	int result = SO_CONTINUE(int, lumpLoad_hook, lumpName);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

	g_lumpLoadTotalMs += elapsedMs;

	if (elapsedMs > 100)
	{
		logv_error("lumpLoad('%s') took %.2f ms (%.2f seconds)\n", lumpName, elapsedMs, elapsedMs / 1000.0f);
	}

	return result;
}

so_hook lumpLoadGlob_hook;
void lumpLoadGlob(char *lumpName) {
	logv_error("lumpLoadGlob called (%s)", lumpName);
	uint64_t timeStart = sceKernelGetProcessTimeWide();

	SO_CONTINUE(void*, lumpLoadGlob_hook, lumpName);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

	g_lumpLoadGlobTotalMs += elapsedMs;

	logv_error("lumpLoadGlob('%s') took %.2f ms (%.2f seconds)\n", lumpName, elapsedMs, elapsedMs / 1000.0f);
}

so_hook MC_LoadLevelEntities_hook;
void MC_LoadLevelEntities(char *worldName) {
	uint64_t timeStart = sceKernelGetProcessTimeWide();

	SO_CONTINUE(void*, MC_LoadLevelEntities_hook, worldName);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

	g_MC_LoadLevelEntitiesMs = elapsedMs;
}

// so_hook lockLoadingMutex_hook;
// void lockLoadingMutex(bool param_1) {
// 	SO_CONTINUE(void*, lockLoadingMutex_hook, param_1);
// }

// so_hook releaseLoadingMutex_hook;
// void releaseLoadingMutex(void) {
// 	SO_CONTINUE(void*, releaseLoadingMutex_hook);
// }

so_hook worldAllocateSegments_hook;
// void worldAllocateSegments(void *worldHeader) {
// 	uint64_t timeStart = sceKernelGetProcessTimeWide();

// 	worldAllocateSegments_impl((_worldHeader*)worldHeader);

// 	uint64_t timeEnd = sceKernelGetProcessTimeWide();
// 	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

// 	g_worldAllocateSegmentsMs = elapsedMs;
// }

so_hook worldReset_hook;
void worldReset(void *worldHeader) {
	uint64_t timeStart = sceKernelGetProcessTimeWide();

	SO_CONTINUE(void*, worldReset_hook, worldHeader);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

	g_worldResetMs = elapsedMs;
}

so_hook gameClear_hook;
void gameClear(int param_1) {
	uint64_t timeStart = sceKernelGetProcessTimeWide();

	SO_CONTINUE(void*, gameClear_hook, param_1);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

	g_gameClearMs = elapsedMs;
}

so_hook SND_StartStream_hook;
void SND_StartStream(int param_1, char *path, int param_3, int param_4, int param_5) {
	uint64_t timeStart = sceKernelGetProcessTimeWide();

	SO_CONTINUE(void*, SND_StartStream_hook, param_1, path, param_3, param_4, param_5);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

	g_SND_StartStreamMs = elapsedMs;
}


so_hook D3DDevice_CreatePalette2_hook;
void* D3DDevice_CreatePalette2(int param_1) {
	uint64_t timeStart = sceKernelGetProcessTimeWide();

	void* result = SO_CONTINUE(void*, D3DDevice_CreatePalette2_hook, param_1);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

	g_D3DDevice_CreatePalette2Ms += elapsedMs;

	return result;
}

so_hook D3DPalette_Lock2_hook;
int D3DPalette_Lock2(void* palette, int param_1) {
	return SO_CONTINUE(int, D3DPalette_Lock2_hook, palette, param_1);
}

so_hook D3DTexture_UnlockRect_hook;
int D3DTexture_UnlockRect(void* texture, int level) {
	uint64_t timeStart = sceKernelGetProcessTimeWide();

	int result = SO_CONTINUE(int, D3DTexture_UnlockRect_hook, texture, level);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

	g_D3DTexture_UnlockRectMs += elapsedMs;

	return result;
}

so_hook machHostOpen_hook;
int machHostOpen(char* path, char* mode) {
	uint64_t timeStart = sceKernelGetProcessTimeWide();

	int result = SO_CONTINUE(int, machHostOpen_hook, path, mode);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

	g_machHostOpenMs += elapsedMs;

	return result;
}

so_hook machHostRead_hook;
int machHostRead(int handle, void* buffer, int size) {
	uint64_t timeStart = sceKernelGetProcessTimeWide();

	int result = SO_CONTINUE(int, machHostRead_hook, handle, buffer, size);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

	g_machHostReadMs += elapsedMs;

	return result;
}

so_hook machHostSeek_hook;
int machHostSeek(int handle, int offset, int whence) {
	uint64_t timeStart = sceKernelGetProcessTimeWide();

	int result = SO_CONTINUE(int, machHostSeek_hook, handle, offset, whence);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

	g_machHostSeekMs += elapsedMs;

	return result;
}

so_hook machHostClose_hook;
int machHostClose(int handle) {
	uint64_t timeStart = sceKernelGetProcessTimeWide();

	int result = SO_CONTINUE(int, machHostClose_hook, handle);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

	g_machHostCloseMs += elapsedMs;

	return result;
}

uintptr_t lockLoadingMutex_addr;
uintptr_t releaseLoadingMutex_addr;

// Direct function addresses for worldAllocateSegments
uintptr_t lumpLoad_addr;
uintptr_t machHostOpen_addr;
uintptr_t machHostRead_addr;
uintptr_t machHostSeek_addr;
uintptr_t machHostClose_addr;
uintptr_t lowestPowerof2NotLessThan_addr;
uintptr_t D3DDevice_CreatePalette2_addr;
uintptr_t D3DPalette_Lock2_addr;
uintptr_t D3DDevice_CreateTexture2_addr;
uintptr_t D3DTexture_LockRect_addr;
uintptr_t D3DTexture_UnlockRect_addr;

so_hook ogg_stream_hook;
void ogg_stream_patched(void* thisptr, char* filename, int* param_2, int* param_3, int param_4) {
	 log_error("OggStream constructor called:");
	 logv_error("  thisptr: %p", thisptr);
	 logv_error("  filename: %s", filename ? filename : "NULL");
	 logv_error("  param_2: %p (value: %d)", param_2, param_2 ? *param_2 : 0);
	 logv_error("  param_3: %p (value: %d)", param_3, param_3 ? *param_3 : 0);
	 logv_error("  param_4: %d", param_4);

	SO_CONTINUE(void*, ogg_stream_hook, thisptr, filename, param_2, param_3, param_4);
}

/**
 * Patch shadow rendering coordinates for half-resolution (256x64) shadow textures.
 *
 * The RenderDelayedShadows function has two loops that render shadows to a texture.
 * Originally designed for 512x128 textures with Y positions at 64, 192, 320, 448.
 * This patches them for 256x64 textures with Y positions at 32, 96, 160, 224.
 *
 * Changes:
 * - Initial Y position: 64 → 32 pixels
 * - Y increment: 128 → 64 pixels
 * - Loop end: 576 → 288 pixels
 * - Width constant: 400.0 → 200.0
 * - Height constant: 64.0 → 32.0
 */
void patch_shadow_resolution() {
	log_error("Patching shadow resolution coordinates for 256x64 textures...");

	// ARM instruction encodings for half-resolution shadow textures
    
    uint32_t mov_r4_0x240 = 0xE3A04D09;      // mov r4, #0x240 

	uint32_t mov_r4_0x20 = 0xe3a04020;      // mov r4, #0x20 (initial X = 32)
	uint32_t add_r4_r4_0x40 = 0xe2844040;   // add r4, r4, #0x40 (X increment = 64)
	uint32_t cmp_r4_0x120 = 0xe3540f48;     // cmp r4, #0x120 (loop end = 288)
	uint32_t movt_r0_0x4348 = 0xe3400348;   // movt r0, #0x4348 (width = 200.0f)
	uint32_t movt_r0_0x4200 = 0xe3400200;   // movt r0, #0x4200 (height = 32.0f)

	// Phase 2 projection: Change division from 3.0 to 1.5 for half-resolution
	uint32_t vmov_s28_1p5 = 0xeeb78a00;     // vmov.f32 s28, #1.5 (was 3.0)

	// Loop 1: First shadow rendering loop
	log_error("  Patching Loop 1 (Phase 1 rendering)...");
	kuKernelCpuUnrestrictedMemcpy((void*)LOC(0x0013d9c4), &mov_r4_0x20, 4);       // Initial X
	kuKernelCpuUnrestrictedMemcpy((void*)LOC(0x0013da0c), &add_r4_r4_0x40, 4);    // X increment
	kuKernelCpuUnrestrictedMemcpy((void*)LOC(0x0013da14), &cmp_r4_0x120, 4);      // Loop end
	kuKernelCpuUnrestrictedMemcpy((void*)LOC(0x0013dac0), &movt_r0_0x4348, 4);    // Width = 200.0
	kuKernelCpuUnrestrictedMemcpy((void*)LOC(0x0013dad0), &movt_r0_0x4200, 4);    // Height = 32.0

	// Loop 2: Second shadow rendering loop (alternate vertex stream)
	log_error("  Patching Loop 2 (Phase 1 rendering)...");
	kuKernelCpuUnrestrictedMemcpy((void*)LOC(0x0013dbb0), &mov_r4_0x20, 4);       // Initial X
	kuKernelCpuUnrestrictedMemcpy((void*)LOC(0x0013dbb8), &add_r4_r4_0x40, 4);    // X increment
	kuKernelCpuUnrestrictedMemcpy((void*)LOC(0x0013dbc0), &cmp_r4_0x120, 4);      // Loop end
	kuKernelCpuUnrestrictedMemcpy((void*)LOC(0x0013dc6c), &movt_r0_0x4348, 4);    // Width = 200.0
	kuKernelCpuUnrestrictedMemcpy((void*)LOC(0x0013dc7c), &movt_r0_0x4200, 4);    // Height = 32.0

	// Phase 2: Shadow projection division constant
	//log_error("  Patching Phase 2 (projection divisor 3.0 -> 1.5)...");
	//kuKernelCpuUnrestrictedMemcpy((void*)LOC(0x0013dda8), &vmov_s28_1p5, 4);      // Change 3.0 to 1.5

	// Phase 1: Position offset scaling constants (constant pool)
	// The offset calculations use: base + offset * (1/128) * (1/3)
	// For half-resolution textures, the 1/3 factor needs to become 2/3
	log_error("  Patching Phase 1 position offset constants (1/3 -> 2/3)...");
	uint32_t two_thirds = 0x3f2aaaab;  // 2/3 (was 1/3 = 0x3eaaaaab)
    uint32_t zero_point_two = 0x3e4ccccd;
    uint32_t offsetScale = 0x3dcccccd;
	//kuKernelCpuUnrestrictedMemcpy((void*)LOC(0x0013da00), &zero_point_two, 4);  // Constant pool value 1
    //kuKernelCpuUnrestrictedMemcpy((void*)LOC(0x0013da04), &offsetScale, 4);  
	//kuKernelCpuUnrestrictedMemcpy((void*)LOC(0x0013da04), &two_thirds, 4);  // Constant pool value 2

	log_error("Shadow resolution patches applied successfully (complete)");
}

extern bool enable_cheats;
so_hook isCheatTriggered_hook;
bool isCheatTriggered()
{
    //logv_error("isCheatTriggered: 0x%x", enable_cheats);
    return !enable_cheats;
}

so_hook clear_hook;
so_hook createTexture2_hook;



void so_patch(void) {

    patch_memory();
	//mach_frameEnd_hook = hook_addr(LOC(0x00181d04), (uintptr_t)&machFrameEnd);

	lockLoadingMutex_addr = LOC(0x0018364c);
	releaseLoadingMutex_addr = LOC(0x0018367c);

	// Store direct addresses for worldAllocateSegments
	lumpLoad_addr = LOC(0x000f810c);
	machHostOpen_addr = LOC(0x00180520);
	machHostRead_addr = LOC(0x001805cc);
	machHostSeek_addr = LOC(0x001806a4);
	machHostClose_addr = LOC(0x00180674);
	D3DDevice_CreatePalette2_addr = LOC(0x0020a2b0);
	D3DPalette_Lock2_addr = LOC(0x0020a314);
	D3DTexture_UnlockRect_addr = LOC(0x00215380);

	worldAllocateSegments_hook = hook_addr(LOC(0x00131aec), (uintptr_t)&worldAllocateSegments);

    // This fixes some weird texts in spanish AND prevents buffer overflow in runDialog
	uintptr_t usprintf_addr = (uintptr_t)so_symbol(&so_mod, "_Z8usprintfPtPKtfffffff");
	if (usprintf_addr == 0) {
		log_error("usprintf not found\n");
	} else {
		logv_debug("usprintf found at %p\n", usprintf_addr);
		usprintf_hook = hook_addr(usprintf_addr, (uintptr_t)&usprintf_patched);
		//log_debug("usprintf hooked with bounds-checked version\n");
	}

	D3DDevice_SetTexture_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "D3DDevice_SetTexture"), (uintptr_t)&D3DDevice_SetTexture);
	D3DDevice_SetVertexShaderConstantNotInline_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_SetVertexShaderConstantNotInline");
	D3DDevice_SetVertexShaderConstantFast_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_SetVertexShaderConstantFast");
	D3DDevice_ReadCommand_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice11ReadCommandEv");
	D3DDevice_SetVertexShaderConstantNotInline_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "D3DDevice_SetVertexShaderConstantNotInline"), (uintptr_t)&D3DDevice_SetVertexShaderConstantNotInline_patched);

	coreAddTask_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z11coreAddTaskPFvvEiPKc"), (uintptr_t)&coreAddTask);
    isCheatTriggered_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN14CommonControls16IsCheatTriggeredEv"), (uintptr_t)&isCheatTriggered);
	renderDelayedShadows_hook = hook_addr(LOC(0x0013d578), (uintptr_t)&renderDelayedShadows);
	ProcessAndUploadTexture_hook = hook_addr(LOC(0x0021225c), (uintptr_t)&ProcessAndUploadTexture);
	XGSetTextureHeader_hook = hook_addr(LOC(0x0020fca4), (uintptr_t)&XGSetTextureHeader);

	// _Z17writeConfigDirectv
	uintptr_t writeConfigDirect_addr = (uintptr_t)so_symbol(&so_mod, "_Z17writeConfigDirectv");
	if (writeConfigDirect_addr == 0) {
		log_error("writeConfigDirect not found\n");
	} else {
		logv_debug("writeConfigDirect found at %p\n", writeConfigDirect_addr);
		writeConfigDirect_hook = hook_addr(writeConfigDirect_addr, (uintptr_t)&writeConfigDirect);
	}

	//void D3DTexture_LockRect(D3DBaseTexture *pThis,undefined4 Level,int *pLockedRect,int *pRect,int flags)
	D3DTexture_LockRect_addr = (uintptr_t)so_symbol(&so_mod, "D3DTexture_LockRect");
	if (D3DTexture_LockRect_addr == 0) {
		log_error("D3DTexture_LockRect not found\n");
	} else {
		logv_debug("D3DTexture_LockRect found at %p\n", D3DTexture_LockRect_addr);
		D3DTexture_LockRect_hook = hook_addr(D3DTexture_LockRect_addr, (uintptr_t)&D3DTexture_LockRect);
	}

	//D3DDevice_CreateTexture2
	D3DDevice_CreateTexture2_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_CreateTexture2");

	//_ZN3JBE9D3DDevice13AsyncRenderCBEPv
	uintptr_t D3DDevice_AsyncRenderCB_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice13AsyncRenderCBEPv");
	if (D3DDevice_AsyncRenderCB_addr == 0) {
		log_error("D3DDevice_AsyncRenderCB not found\n");
	} else {
		logv_debug("D3DDevice_AsyncRenderCB found at %p\n", D3DDevice_AsyncRenderCB_addr);
		D3DDevice_AsyncRenderCB_hook = hook_addr(D3DDevice_AsyncRenderCB_addr, (uintptr_t)&D3DDevice_AsyncRenderCB);
	}	

	uintptr_t cdDirectoryLookup_addr = (uintptr_t)so_symbol(&so_mod, "_Z17cdDirectoryLookupPKcPiS1_");
	if (cdDirectoryLookup_addr == 0) {
		log_error("cdDirectoryLookup not found\n");
	} else {
		logv_debug("cdDirectoryLookup found at %p\n", cdDirectoryLookup_addr);
		cdDirectoryLookup_hook = hook_addr(cdDirectoryLookup_addr, (uintptr_t)&cdDirectoryLookup);
	}


	//_ZN3JBE9SingletonINS_7DisplayEE11s_pInstanceE
	g_Singleton_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9SingletonINS_7DisplayEE11s_pInstanceE");
	displayPF_AcquireContext_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9DisplayPF14AcquireContextEv");
	displayPF_ReleaseContext_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9DisplayPF14ReleaseContextEv");

	#ifdef PROFILER_ENABLED
	//install_prof_hooks();
	#endif

    patch_touch_ui();

	// Shadow optimization hooks - skip redundant Clear calls in shadow rendering
	//extern void (*D3DDevice_Clear_orig)(uint32_t, void*, uint32_t, uint32_t, float, uint32_t);
	//extern void (*RenderDelayedShadows_orig)(void);
	extern void D3DDevice_Clear(uint32_t, void*, uint32_t, uint32_t, float, uint32_t);
	//extern void RenderDelayedShadows_hook(void);

	clear_hook = hook_addr(LOC(0x001e19c0), (uintptr_t)&D3DDevice_Clear);

	// Hook D3DDevice_CreateTexture2 to reduce shadow texture resolution
	extern void* D3DDevice_CreateTexture2(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
	createTexture2_hook = hook_addr(LOC(0x00215bb0), (uintptr_t)&D3DDevice_CreateTexture2);

	// Apply shadow resolution coordinate patches for 256x64 textures
	//patch_shadow_resolution();
}
