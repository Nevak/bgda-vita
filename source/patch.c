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
#include "worldAllocateSegments.h"

#include "utils/macros.h"

#include "patches/bgda_types.h"
#include "patches/frustum_culling.h"
#include "patches/texture_decomp.h"
#include "patches/texture_palette.h"
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

so_hook coreAddTask_hook;
int coreAddTask(void *fn, int prio, char *name) {
	logv_error("coreAddTask(%p, %i, %s)", fn, prio, name);
	// ignore if task is StatCache
	if (name && strcmp(name, "StatCache") == 0) {
		//logv_error("Ignoring StatCache task\n");
		return 0;
	}

	if (name && strcmp(name, "RenderDelayedShadows") == 0) {
	   	log_error("Ignoring renderDelayedShadows task\n");
	   	return 0;
	}

    return SO_CONTINUE(int, coreAddTask_hook, fn, prio, name);
}

so_hook renderTouchIcons_hook;
void renderTouchIcons(void *param_1) {}

so_hook usingTouchscreen_hook;
bool usingTouchscreen() {
	return false;
}

so_hook frontEndDoControllerScreenInput_hook;
void frontEndDoControllerScreenInput(int *param_1, int *param_2) {}

so_hook inputRender_hook;
void inputRender(void *thisptr) {}

so_hook virtualControlsRender_hook;
void virtualControlsRender(void *thisptr) {}

so_hook writeConfigDirect_hook;
void writeConfigDirect() {}

so_hook cdDirectoryLookup_hook;
int cdDirectoryLookup(const char *path, int *param_2, int *param_3) {
	float timeNow = sceKernelGetProcessTimeWide();
	int returnval = SO_CONTINUE(int, cdDirectoryLookup_hook, path, param_2, param_3);
	float timeAfter = sceKernelGetProcessTimeWide();

	logv_error("[%d] cdDirectoryLookup took %f ms\n", path, (timeAfter - timeNow) / 1000);
	
	return returnval;
}

so_hook D3DDevice_SetTextureStages_hook;
void D3DDevice_SetTextureStages(uint8_t *param_1, uint32_t param_2) {
	SO_CONTINUE(void *, D3DDevice_SetTextureStages_hook, param_1, param_2);
}

so_hook machFrameStart_hook;
void machFrameStart(int p) {
	sceKernelChangeThreadCpuAffinityMask(sceKernelGetThreadId(), SCE_KERNEL_CPU_MASK_USER_1);
	SO_CONTINUE(void *, machFrameStart_hook, p);
}

so_hook mach_frameEnd_hook;
void machFrameEnd(int param_1) {
	SO_CONTINUE(void *, mach_frameEnd_hook, param_1);
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
	uint32_t threadId = sceKernelGetThreadId();
	logv_error("[0x%X] ===============D3DDevice_AsyncRenderCB================\n", threadId);
	sceKernelChangeThreadPriority(threadId, 100);
	sceKernelChangeThreadCpuAffinityMask(threadId, SCE_KERNEL_CPU_MASK_USER_0);
	pthread_setname_np_soloader(threadId, "D3DDevice_AsyncRenderCB");

    D3DDevice* device = (D3DDevice*)device_ptr;
	logv_error("g_Singleton_addr is: %p", (void*)g_Singleton_addr);
    void* display = (void*)((char*)(g_Singleton_addr) + 0x10);
	
	logv_error("DisplayPF pointer is: %p", display);

    // Cast function pointer types
    AcquireContextFn fnAcquire = (AcquireContextFn)displayPF_AcquireContext_addr;
    ReleaseContextFn fnRelease = (ReleaseContextFn)displayPF_ReleaseContext_addr;
    fnReadCommand = (ReadCommandFn)D3DDevice_ReadCommand_addr;

	log_error("will call fnAcquire");
    // Acquire Render Context
    fnAcquire(display);
	log_error("fnAcquire finished");

    // Wait on semaphore until success (retries on EINTR)
	unsigned char * rawPtr = (unsigned char *)device_ptr;
	logv_error("device addr: %p", device);
	logv_error("rawPtr: %p", rawPtr);
	logv_error("device->pSemaphore_Main: %p", device->pSemaphore_Main);
	logv_error("addr of device->pSemaphore_Main: %p", (void*)&device->pSemaphore_Main);
    while (sem_wait_soloader(device->pSemaphore_Main) != 0) {
        // optional: check errno if needed
    }

	log_error("sem_wait finished");

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
	logv_error("System_BeginFrame(%p)\n", param_1);
	SO_CONTINUE(void *, System_BeginFrame_hook, param_1);
	log_error("System_BeginFrame finished\n");
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
void renderDelayedShadows(void) {
	SO_CONTINUE(void *, renderDelayedShadows_hook);
}
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

so_hook memAlloc_hook;
void* memAlloc(int size, const char* name) {
	if (name && name[0] != '\0') {
		logv_error("memAlloc(%d, \"%s\")", size, name);
	}
	void* ret = SO_CONTINUE(void*, memAlloc_hook, size, name);
	if (name && name[0] != '\0') {
		logv_error("ret=%p", ret);
	}
	return ret;
}

so_hook cdStartStream_hook;
void cdStartStream(char *filename, int param_2) {
	logv_error("cdStartStream(\"%s\", %d)", filename ? filename : "NULL", param_2);
	SO_CONTINUE(void*, cdStartStream_hook, filename, param_2);
	log_error("cdStartStream completed");
}

so_hook cdStreamLoad_hook;
void cdStreamLoad(char *filename, int param_2) {
	logv_error("cdStreamLoad(%d)", param_2);
	SO_CONTINUE(void*, cdStreamLoad_hook, filename, param_2);
	log_error("cdStreamLoad completed");
}

so_hook gameLoadWorld_hook;
void gameLoadWorld(char *worldName) {

	logv_error("[0x%X] Entered gameLoadWorld: %s", sceKernelGetThreadId(), worldName);
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

// so_hook D3DDevice_CreateTexture2_hook;
// void* D3DDevice_CreateTexture2(int width, int height, int levels, int usage, int format, int d3dFormat, int pool) {
// 	uint64_t timeStart = sceKernelGetProcessTimeWide();

// 	void* result = SO_CONTINUE(void*, D3DDevice_CreateTexture2_hook, width, height, levels, usage, format, d3dFormat, pool);

// 	uint64_t timeEnd = sceKernelGetProcessTimeWide();
// 	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

// 	g_D3DDevice_CreateTexture2Ms += elapsedMs;

// 	return result;
// }

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

// so_hook D3DTexture_LockRect_hook;
// int D3DTexture_LockRect(void* texture, int level, int* lockedRect, int* rect, int flags) {
// 	uint64_t timeStart = sceKernelGetProcessTimeWide();

// 	int result = SO_CONTINUE(int, D3DTexture_LockRect_hook, texture, level, lockedRect, rect, flags);

// 	uint64_t timeEnd = sceKernelGetProcessTimeWide();
// 	float elapsedMs = (timeEnd - timeStart) / 1000.0f;

// 	g_D3DTexture_LockRectMs += elapsedMs;

// 	return result;
// }

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

void so_patch(void) {
	//sceSysmoduleLoadModule(SCE_SYSMODULE_PERF);
	memAlloc_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z8memAllociPKc"), (uintptr_t)&memAlloc);
	cdStartStream_hook = hook_addr(LOC(0x000cd4b8), (uintptr_t)&cdStartStream);
	cdStreamLoad_hook = hook_addr(LOC(0x000cd75c), (uintptr_t)&cdStreamLoad);
  	gameLoadWorld_hook = hook_addr(LOC(0x0013f154), (uintptr_t)&gameLoadWorld);
	cdProcess_hook = hook_addr(LOC(0x000cd80c), (uintptr_t)&cdProcess);
	MC_LoadLevelEntities_hook = hook_addr(LOC(0x000e133c), (uintptr_t)&MC_LoadLevelEntities);
	lumpLoad_hook = hook_addr(LOC(0x000f810c), (uintptr_t)&lumpLoad);
	lumpLoadGlob_hook = hook_addr(LOC(0x000f82bc), (uintptr_t)&lumpLoadGlob);
	//lockLoadingMutex_hook = hook_addr(LOC(0x0018364c), (uintptr_t)&lockLoadingMutex);
	//releaseLoadingMutex_hook = hook_addr(LOC(0x0018367c), (uintptr_t)&releaseLoadingMutex);
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
	//worldReset_hook = hook_addr(LOC(0x0013a0bc), (uintptr_t)&worldReset);
	//gameClear_hook = hook_addr(LOC(0x0013ef90), (uintptr_t)&gameClear);
	//SND_StartStream_hook = hook_addr(LOC(0x0010a690), (uintptr_t)&SND_StartStream);
	//D3DDevice_CreateTexture2_hook = hook_addr(LOC(0x00215bb0), (uintptr_t)&D3DDevice_CreateTexture2);
	//D3DDevice_CreatePalette2_hook = hook_addr(LOC(0x0020a2b0), (uintptr_t)&D3DDevice_CreatePalette2);
	//D3DPalette_Lock2_hook = hook_addr(LOC(0x0020a314), (uintptr_t)&D3DPalette_Lock2);
	//D3DTexture_LockRect_hook = hook_addr(LOC(0x00215260), (uintptr_t)&D3DTexture_LockRect);
	// D3DTexture_UnlockRect_hook = hook_addr(LOC(0x00215380), (uintptr_t)&D3DTexture_UnlockRect);
	// machHostOpen_hook = hook_addr(LOC(0x00180520), (uintptr_t)&machHostOpen);
	// machHostRead_hook = hook_addr(LOC(0x001805cc), (uintptr_t)&machHostRead);
	// machHostSeek_hook = hook_addr(LOC(0x001806a4), (uintptr_t)&machHostSeek);
	// machHostClose_hook = hook_addr(LOC(0x00180674), (uintptr_t)&machHostClose);
	// _Z8usprintfPtPKtfffffff
	uintptr_t usprintf_addr = (uintptr_t)so_symbol(&so_mod, "_Z8usprintfPtPKtfffffff");
	if (usprintf_addr == 0) {
		log_error("usprintf not found\n");
	} else {
		logv_error("usprintf found at %p\n", usprintf_addr);
		usprintf_hook = hook_addr(usprintf_addr, (uintptr_t)&usprintf_patched);
	}

	// _Z22worldClipCubeToFrustumPA2_fi
	uintptr_t worldClipCubeToFrustum_addr = (uintptr_t)so_symbol(&so_mod, "_Z22worldClipCubeToFrustumPA2_fi");
	if (worldClipCubeToFrustum_addr == 0) {
		log_error("worldClipCubeToFrustum not found\n");
	} else {
		logv_error("worldClipCubeToFrustum found at %p\n", worldClipCubeToFrustum_addr);
		worldClipCubeToFrustum_hook = hook_addr(worldClipCubeToFrustum_addr, (uintptr_t)&worldClipCubeToFrustum);
	}

	// _Z26worldClipCubeToClipFrustumPA2_fi
	uintptr_t worldClipCubeToClipFrustum_addr = (uintptr_t)so_symbol(&so_mod, "_Z26worldClipCubeToClipFrustumPA2_fi");
	if (worldClipCubeToClipFrustum_addr == 0) {
		log_error("worldClipCubeToClipFrustum not found\n");
	}
	else {
		logv_error("worldClipCubeToClipFrustum found at %p\n", worldClipCubeToClipFrustum_addr);
		worldClipCubeToClipFrustum_hook = hook_addr(worldClipCubeToClipFrustum_addr, (uintptr_t)&worldClipCubeToClipFrustum);
	}

	//_Z26worldClipCubeToFrustumOncePA2_f
	uintptr_t worldClipCubeToFrustumOnce_addr = (uintptr_t)so_symbol(&so_mod, "_Z26worldClipCubeToFrustumOncePA2_f");
	if (worldClipCubeToFrustumOnce_addr == 0) {
		log_error("worldClipCubeToFrustumOnce not found\n");
	} else {
		logv_error("worldClipCubeToFrustumOnce found at %p\n", worldClipCubeToFrustumOnce_addr);
		worldClipCubeToFrustumOnce_hook = hook_addr(worldClipCubeToFrustumOnce_addr, (uintptr_t)&worldClipCubeToFrustumOnce);
	}
	
	g_frustumVertexIndices = (FrustumIdx*)(LOC(0x003f7e26));
	logv_error("g_frustumVertexIndices is at %p\n", g_frustumVertexIndices);
	g_worldFrustum = (float*)(LOC(0x003f7ccc));
	logv_error("g_worldFrustum is at %p\n", g_worldFrustum);
	

	D3DDevice_SetTexture_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "D3DDevice_SetTexture"), (uintptr_t)&D3DDevice_SetTexture);
	
	// Hook WriteCommand function using direct address
	//WriteCommand_hook = hook_addr(LOC(0x001dc604), (uintptr_t)&WriteCommand_Optimized);
	logv_error("WriteCommand hooked at address 0x001dc604 -> %p\n", &WriteCommand_Optimized);
	
	D3DDevice_SetVertexShaderConstantNotInline_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_SetVertexShaderConstantNotInline");
	D3DDevice_SetVertexShaderConstantFast_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_SetVertexShaderConstantFast");
	D3DDevice_ReadCommand_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice11ReadCommandEv");
	D3DDevice_SetVertexShaderConstantNotInline_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "D3DDevice_SetVertexShaderConstantNotInline"), (uintptr_t)&D3DDevice_SetVertexShaderConstantNotInline_patched);
	
	// _Z11coreAddTaskPFvvEiPKc coreAddTask
	coreAddTask_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z11coreAddTaskPFvvEiPKc"), (uintptr_t)&coreAddTask);
	//renderDelayedShadows_hook = hook_addr(LOC(0x0013d578), (uintptr_t)&renderDelayedShadows);
	//runObjects_hook = hook_addr(LOC(0x00114a1c), (uintptr_t)&runObjects);
	//drawObjects_hook = hook_addr(LOC(0x00115094), (uintptr_t)&drawObjects);
	XGGetPixelBufferMinAlpha_hook = hook_addr(LOC(0x00209d0c), (uintptr_t)&XGGetPixelBufferMinAlpha);
	XGGetPixelBufferMaxAlpha_hook = hook_addr(LOC(0x00209ff0), (uintptr_t)&XGGetPixelBufferMaxAlpha);
	ProcessAndUploadTexture_hook = hook_addr(LOC(0x0021225c), (uintptr_t)&ProcessAndUploadTexture);
	//DoTheFinalGPUUpload_hook = hook_addr(LOC(0x002160ec), (uintptr_t)&DoTheFinalGPUUpload);
	XGSetTextureHeader_hook = hook_addr(LOC(0x0020fca4), (uintptr_t)&XGSetTextureHeader);

	//D3DDevice_TextureStageState_SetToGL_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice17TextureStageState7SetToGLEmN13XGSamplerType4EnumE"), (uintptr_t)&D3DDevice_TextureStageState_SetToGL);
	D3DDevice_UnregisterTextureCommand_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice24UnregisterTextureCommandEP25RegisteredBaseTextureDataRi"), (uintptr_t)&D3DDevice_UnregisterTextureCommand);
	// Don't hook these - they may be called from wrong thread (not rendering thread)
	//D3DBaseTexture_UnbufferToOGL_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN14D3DBaseTexture13UnbufferToOGLEv"), (uintptr_t)&D3DBaseTexture_UnbufferToOGL);
	//D3DBaseTexture_Unregister_hook =  hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN14D3DBaseTexture10UnregisterEi"), (uintptr_t)&D3DBaseTexture_Unregister);


	hook_addr(so_symbol(&so_mod, "_Z9SND_Framev"), (uintptr_t)&DoNothing);


	//uint32_t loc = LOC(0x00132474);
	//logv_error("COPY TEXTURE at %p\n", loc);
	//texture_copy_hook = hook_addr(loc, (uintptr_t)&texture_copy);

	//_Z25lowestPowerof2NotLessThani
	lowestPowerof2NotLessThan_addr = (uintptr_t)so_symbol(&so_mod, "_Z25lowestPowerof2NotLessThani");
	if (lowestPowerof2NotLessThan_addr == 0) {
		log_error("lowestPowerof2NotLessThan not found\n");
	} else {
		logv_error("lowestPowerof2NotLessThan found at %p\n", lowestPowerof2NotLessThan_addr);
		lowestPowerof2NotLessThan_hook = hook_addr(lowestPowerof2NotLessThan_addr, (uintptr_t)&lowestPowerof2NotLessThan);
	}


	//_Z7memInitPvi
	uintptr_t memInit_addr = (uintptr_t)so_symbol(&so_mod, "_Z7memInitPvi");
	if (memInit_addr == 0) {
		log_error("memInit not found\n");
	} else {
		logv_error("memInit found at %p\n", memInit_addr);
		memInit_hook = hook_addr(memInit_addr, (uintptr_t)&memInit);
	}

	// _Z17writeConfigDirectv
	uintptr_t writeConfigDirect_addr = (uintptr_t)so_symbol(&so_mod, "_Z17writeConfigDirectv");
	if (writeConfigDirect_addr == 0) {
		log_error("writeConfigDirect not found\n");
	} else {
		logv_error("writeConfigDirect found at %p\n", writeConfigDirect_addr);
		writeConfigDirect_hook = hook_addr(writeConfigDirect_addr, (uintptr_t)&writeConfigDirect);
	}

	//void D3DTexture_LockRect(D3DBaseTexture *pThis,undefined4 Level,int *pLockedRect,int *pRect,int flags)
	D3DTexture_LockRect_addr = (uintptr_t)so_symbol(&so_mod, "D3DTexture_LockRect");
	if (D3DTexture_LockRect_addr == 0) {
		log_error("D3DTexture_LockRect not found\n");
	} else {
		logv_error("D3DTexture_LockRect found at %p\n", D3DTexture_LockRect_addr);
		D3DTexture_LockRect_hook = hook_addr(D3DTexture_LockRect_addr, (uintptr_t)&D3DTexture_LockRect);
	}

	//D3DDevice_CreateTexture2
	D3DDevice_CreateTexture2_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_CreateTexture2");
	if (D3DDevice_CreateTexture2_addr == 0) {
		log_error("D3DDevice_CreateTexture2 not found\n");
	} else {
		logv_error("D3DDevice_CreateTexture2 found at %p\n", D3DDevice_CreateTexture2_addr);
		D3DDevice_CreateTexture2_hook = hook_addr(D3DDevice_CreateTexture2_addr, (uintptr_t)&D3DDevice_CreateTexture2);
	}

	// _ZN15VirtualControls6RenderEv
	uintptr_t virtualControlsRender_addr = (uintptr_t)so_symbol(&so_mod, "_ZN15VirtualControls6RenderEv");
	if (virtualControlsRender_addr == 0) {
		log_error("virtualControlsRender not found\n");
	} else {
		logv_error("virtualControlsRender found at %p\n", virtualControlsRender_addr);
		virtualControlsRender_hook = hook_addr(virtualControlsRender_addr, (uintptr_t)&virtualControlsRender);
	}

	// _Z31frontEndDoControllerScreenInputRiS_
	uintptr_t frontEndDoControllerScreenInput_addr = (uintptr_t)so_symbol(&so_mod, "_Z31frontEndDoControllerScreenInputRiS_");
	if (frontEndDoControllerScreenInput_addr == 0) {
		log_error("frontEndDoControllerScreenInput not found\n");
	} else {
		logv_error("frontEndDoControllerScreenInput found at %p\n", frontEndDoControllerScreenInput_addr);
		frontEndDoControllerScreenInput_hook = hook_addr(frontEndDoControllerScreenInput_addr, (uintptr_t)&frontEndDoControllerScreenInput);
	}

	// _ZN14CommonControls16UsingTouchscreenEv
	uintptr_t usingTouchscreen_addr = (uintptr_t)so_symbol(&so_mod, "_ZN14CommonControls16UsingTouchscreenEv");
	if (usingTouchscreen_addr == 0) {
		log_error("usingTouchscreen not found\n");
	} else {
		logv_error("usingTouchscreen found at %p\n", usingTouchscreen_addr);
		usingTouchscreen_hook = hook_addr(usingTouchscreen_addr, (uintptr_t)&usingTouchscreen);
	}

	//_ZN14CommonControls16RenderTouchIconsEP4Menu
	uintptr_t renderTouchIcons_addr = (uintptr_t)so_symbol(&so_mod, "_ZN14CommonControls16RenderTouchIconsEP4Menu");
	if (renderTouchIcons_addr == 0) {
		log_error("renderTouchIcons not found\n");
	} else {
		logv_error("renderTouchIcons found at %p\n", renderTouchIcons_addr);
		renderTouchIcons_hook = hook_addr(renderTouchIcons_addr, (uintptr_t)&renderTouchIcons);
	}

	// _Z17cdDirectoryLookupPKcPiS1_
	uintptr_t cdDirectoryLookup_addr = (uintptr_t)so_symbol(&so_mod, "_Z17cdDirectoryLookupPKcPiS1_");
	if (cdDirectoryLookup_addr == 0) {
		log_error("cdDirectoryLookup not found\n");
	} else {
		logv_error("cdDirectoryLookup found at %p\n", cdDirectoryLookup_addr);
		cdDirectoryLookup_hook = hook_addr(cdDirectoryLookup_addr, (uintptr_t)&cdDirectoryLookup);
	}

	// _Z14machFrameStartv
	uintptr_t machFrameStart_addr = (uintptr_t)so_symbol(&so_mod, "_Z14machFrameStartv");
	if (machFrameStart_addr == 0) {
		log_error("machFrameStart not found\n");
	} else {
		logv_error("machFrameStart found at %p\n", machFrameStart_addr);
		machFrameStart_hook = hook_addr(machFrameStart_addr, (uintptr_t)&machFrameStart);
	}

	//_ZN3JBE9D3DDevice13AsyncRenderCBEPv
	uintptr_t D3DDevice_AsyncRenderCB_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice13AsyncRenderCBEPv");
	if (D3DDevice_AsyncRenderCB_addr == 0) {
		log_error("D3DDevice_AsyncRenderCB not found\n");
	} else {
		logv_error("D3DDevice_AsyncRenderCB found at %p\n", D3DDevice_AsyncRenderCB_addr);
		D3DDevice_AsyncRenderCB_hook = hook_addr(D3DDevice_AsyncRenderCB_addr, (uintptr_t)&D3DDevice_AsyncRenderCB);
	}

	//_ZN14TrackScheduler12ThreadProcCBEPv
	uintptr_t TrackScheduler_ThreadProcCB_addr = (uintptr_t)so_symbol(&so_mod, "_ZN14TrackScheduler12ThreadProcCBEPv");
	if (TrackScheduler_ThreadProcCB_addr == 0) {
		log_error("TrackScheduler_ThreadProcCB not found\n");
	} else {
		logv_error("TrackScheduler_ThreadProcCB found at %p\n", TrackScheduler_ThreadProcCB_addr);
		TrackScheduler_ThreadProcCB_hook = hook_addr(TrackScheduler_ThreadProcCB_addr, (uintptr_t)&TrackScheduler_ThreadProcCB);
	}

	// D3DDevice_Swap
	uintptr_t D3DDevice_Swap_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_Swap");
	if (D3DDevice_Swap_addr == 0) {
		log_error("D3DDevice_Swap not found\n");
	} else {
		logv_error("D3DDevice_Swap found at %p\n", D3DDevice_Swap_addr);
		D3DDevice_Swap_hook = hook_addr(D3DDevice_Swap_addr, (uintptr_t)&D3DDevice_Swap);
	}
	
	//_ZN3JBE9SingletonINS_7DisplayEE11s_pInstanceE
	g_Singleton_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9SingletonINS_7DisplayEE11s_pInstanceE");
	if (g_Singleton_addr == 0) {
		log_error("g_Singleton not found\n");
	} else {
		logv_error("g_Singleton found at %p\n", g_Singleton_addr);
	}

	// _ZN3JBE9DisplayPF14AcquireContextEv
	displayPF_AcquireContext_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9DisplayPF14AcquireContextEv");
	if (displayPF_AcquireContext_addr == 0) {
		log_error("displayPF_AcquireContext not found\n");
	} else {
		logv_error("displayPF_AcquireContext found at %p\n", displayPF_AcquireContext_addr);
	}

	// _ZN3JBE9DisplayPF14ReleaseContextEv
	displayPF_ReleaseContext_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9DisplayPF14ReleaseContextEv");
	if (displayPF_ReleaseContext_addr == 0) {
		log_error("displayPF_ReleaseContext not found\n");
	} else {
		logv_error("displayPF_ReleaseContext found at %p\n", displayPF_ReleaseContext_addr);
	}

	#ifdef PROFILER_ENABLED
	//install_prof_hooks();
	#endif

	// Patch big and useless texture names so we don't load them
	// Mostly touchscreen UI stuff
	uintptr_t addresses[] = {
		0x0009caf1,
		0x000a326a,
		0x000a1733,
		0x0009b060,
		0x000a0523,
		0x000a9bde,
		0x000a327c,
		0x0009e89e,
		0x0009dda3,
		0x000a4da4,
		0x000a7e89,
		0x0009cb02,
		0x0009dd92,
		0x000ac2b3,
		0x000ab86c,
		0x0009e8b5,
		0x000a328c,
		0x000a2a2c,
		0x000a1745,
		0x0009b044,
		0x0009b9ae,
		0x0009f9bb,
		//0x000a61ab, // "arrowA.tex",
		0x000ab861,  // "arrowB.tex"
		0x000a6a40,  // cancelButtonA.tex
		0x000ad677,   // cancelButtonB.tex
		//0x000a3fa2,	// legal2.tex	
		//0x00a3a91	//frontendmenulong.tex	"frontendmenulong.tex"	ds
	};

	char* stringToPatch = "arrowA.tex";
	for(int i = 0; i < sizeof(addresses) / sizeof(uintptr_t); i++) {
		uintptr_t addressToPatch = so_mod.text_base + addresses[i] - 0x00010000;
		kuKernelCpuUnrestrictedMemcpy((void *)addressToPatch, stringToPatch, strlen(stringToPatch)+1);
	}

	// Patch the minimum pitch check in D3DDevice_CreateTexture2 to NOP
	// This removes the "if (pitch < 0x41) { pitch = 0x40; }" constraint
	uintptr_t pitchCheckAddress = so_mod.text_base + 0x00215c4c - 0x00010000;
	uint32_t nopInstruction = 0xe1a00000; // NOP instruction for ARM (mov r0, r0)
	logv_error("Patching pitch check at address %p with NOP", (void*)pitchCheckAddress);
	//kuKernelCpuUnrestrictedMemcpy((void *)pitchCheckAddress, &nopInstruction, sizeof(nopInstruction));
}
