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
#include <utils/prof.h>
#include <stdio.h>
#include <vitasdk.h>
#include <libsysmodule.h>
#include <libperf.h>
#include <vitaGL.h>

#ifdef __cplusplus
extern "C" {
#endif
extern so_module so_mod;
extern so_module so_mod_libxmv;

#ifdef __cplusplus
};
#endif

#include "utils/logger.h"
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <arm_neon.h>

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



extern int log_allocs;
extern int log_profiler;
extern int profiling_idx;

so_hook renderTouchIcons_hook;
void renderTouchIcons(void *param_1) {

}

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

int curWidth = 0;
so_hook lowestPowerof2NotLessThan_hook;
int lowestPowerof2NotLessThan(int dimension) {
	uint32_t caller = (uint32_t)__builtin_return_address(0);
	if (caller != 0x98521dec && caller != 0x98521dcc) {
	 	return SO_CONTINUE(int, lowestPowerof2NotLessThan_hook, dimension);
	}

    int alignment = 64;
    int aligned = (dimension + (alignment - 1)) & ~(alignment - 1);
    if (aligned < alignment) 
		aligned = alignment;
    return aligned;
}

#define LOC(x) (int *)(so_mod.text_base + x - 0x00010000)
#define CONCAT22(high16, low16) ( \
    ( ((uint32_t)(high16) & 0xFFFF) << 16 ) | \
      ((uint32_t)(low16)  & 0xFFFF)          \
)
#define CONCAT44(high32, low32) ( \
    ( ((uint64_t)(high32) & 0xFFFFFFFFULL) << 32 ) | \
      (  (uint64_t)(low32)  & 0xFFFFFFFFULL )        \
)

so_hook D3DDevice_SetTexture_hook;
extern float g_uvFactor;
extern float g_uvFactorY;
uintptr_t D3DDevice_SetVertexShaderConstantNotInline_addr;
uintptr_t D3DDevice_SetVertexShaderConstantFast_addr;
typedef void (*D3DDevice_SetVertexShaderConstantNotInlineFn)(int reg, uint32_t pConstantData, uint32_t ConstantCount);
typedef void (*D3DDevice_SetVertexShaderConstantFastFn)(int reg, uint32_t pConstantData, uint32_t ConstantCount);

//void D3DDevice_SetVertexShaderConstantNotInline(int register,undefined4 pConstantData,ulong ConstantCount)
//uint32_t g_pConstantData = 0;
so_hook D3DDevice_SetVertexShaderConstantNotInline_hook;
void D3DDevice_SetVertexShaderConstantNotInline_patched(int reg, uint32_t pConstantData, uint32_t ConstantCount) {
	//Profiler_BeginSample("D3DDevice_SetVertexShaderConstantNotInline");
	D3DDevice_SetVertexShaderConstantFastFn D3DDevice_SetVertexShaderConstantFast = (D3DDevice_SetVertexShaderConstantFastFn)D3DDevice_SetVertexShaderConstantFast_addr;
	D3DDevice_SetVertexShaderConstantFast(reg, pConstantData, ConstantCount);
	//Profiler_EndSample();
}

void D3DDevice_SetTexture(uint32_t param_1, int param_2) {
	// Get the caller address
	uintptr_t caller = (uintptr_t)__builtin_return_address(0);
	if (caller == 0x98528c84)
	{
		// param_2 as uint32_t*
		uint32_t *param_2_ptr = (uint32_t *)param_2;
		uint32_t *tex_ptr = (uint32_t*)*(param_2_ptr + 1);

		uint32_t format = 0;
		int isCompressed;
		int isSwizzled;
		uint32_t width;
		uint32_t height;

		uint32_t dimensionsAndFlags = *(param_2_ptr + 4);

		width = (dimensionsAndFlags & 0xfff) + 1;
		height = ((dimensionsAndFlags << 8) >> 0x14) + 1;

		// log the width and height
		//logv_error("D3DDevice_SetTexture: w: %i, h: %i\n", width, height);


		int potWidth = 1;
		while (potWidth < width) {
			potWidth <<= 1;
		}
		if (potWidth < 64)
			potWidth = 64;
		int potHeight = 1;

		while (potHeight < height) {
			potHeight <<= 1;
		}
		if (potHeight < 64)
			potHeight = 64;

			
		// calculate the scale factor for width and height to pass it to the shader so it can scale the UV coordinates
		float scaleX = ((float)potWidth / (float)width);
		float scaleY = ((float)potHeight / (float)height);
		// the shader will need to do:
		// gl_FragColor = texture2D(tex, vec2(texCoord.x * scaleX, texCoord.y * scaleY));
		// set the scale factor in the shader
		//logv_debug("D3DDevice_SetTexture: width: %i, height: %i\n", width, height);

		if (width == 249 && height == 314) {
			float scale[4] = {g_uvFactor * scaleX, g_uvFactorY * scaleY, 0.0f, 0.0f};

			SO_CONTINUE(float, D3DDevice_SetVertexShaderConstantNotInline_hook, 24, (uint32_t)scale, 1);
		}
		else {
			float scale[4] = {scaleX, scaleY, 0.0f, 0.0f};
			SO_CONTINUE(float, D3DDevice_SetVertexShaderConstantNotInline_hook, 24, (uint32_t)scale, 1);
		}
	}
	SO_CONTINUE(void *, D3DDevice_SetTexture_hook, param_1, param_2);
}

so_hook cdDirectoryLookup_hook;
int cdDirectoryLookup(const char *path, int *param_2, int *param_3) {
	float timeNow = sceKernelGetProcessTimeWide();
	int returnval = SO_CONTINUE(int, cdDirectoryLookup_hook, path, param_2, param_3);
	float timeAfter = sceKernelGetProcessTimeWide();

	logv_error("[%d] cdDirectoryLookup took %f ms\n", path, (timeAfter - timeNow) / 1000);
	
	return returnval;
}


int g_width = 0;
int g_height = 0;
int g_pitch = 0;
so_hook D3DDevice_CreateTexture2_hook;
// D3DBaseTexture *D3DDevice_CreateTexture2(int width,int height,undefined4 depth,int levels,uint usage,undefined4 format,undefined4 resourceType)
void *D3DDevice_CreateTexture2(int width, int height, uint32_t depth, int levels, uint32_t usage, uint32_t format, uint32_t resourceType) {
	//logv_error("D3DDevice_CreateTexture2(%i, %i, %u, %i, %u, %u, %u)\n", width, height, depth, levels, usage, format, resourceType);
	void *res = SO_CONTINUE(void *, D3DDevice_CreateTexture2_hook, width, height, depth, levels, usage, format, resourceType);

	g_width = width;
	g_height = height;

	return res;
}

so_hook D3DTexture_LockRect_hook;
// void D3DTexture_UnlockRect(D3DBaseTexture *pThis,undefined4 Level,int *pLockedRect,int *pRect,int flags)
void D3DTexture_LockRect(void *pThis, uint32_t Level, int *pLockedRect, int *pRect, int flags) {
	//logv_error("D3DTexture_LockRect(%p, %u, %p, %p, %u)\n", pThis, Level, pLockedRect, pRect, flags);
	SO_CONTINUE(void *, D3DTexture_LockRect_hook, pThis, Level, pLockedRect, pRect, flags);
	// print pLockedRect[0] and pLockedRect[1]
	//logv_error("D3DTexture_LockRect: pLockedRect[0]: %d, pLockedRect[1]: %p\n", pLockedRect[0], pLockedRect[1]);
	g_pitch = pLockedRect[0];
}

void __aeabi_memclr_patched(void *dst, int n) {
	__aeabi_memclr(dst, n);
}
void __aeabi_memcpy_patched(void *dst, const void *src, int n) {
	//sceRazorCpuPushMarkerWithHud("__aeabi_memcpy_patched", SCE_RAZOR_COLOR_YELLOW, SCE_RAZOR_MARKER_DISABLE_HUD);
//	memcpy(dst, src, n);
	//sceRazorCpuPopMarker();

	//Profiler_BeginSample("memcpy");
	int* caller = __builtin_return_address(0);
	if (caller == LOC(0x00132478))
	{
		int actualHeight = g_height;
		int actualWidth = g_width;
		int pitch = g_pitch;

		int rows = actualHeight;     
		int rowBytes = actualWidth;
		unsigned char *dest = (unsigned char*)dst;
		unsigned char *destPtr = (unsigned char*)dest;
		for (int y = 0; y < rows; ++y) {
			memcpy(destPtr, src, rowBytes);
			destPtr += pitch;
			src += rowBytes;
		}

	//	Profiler_EndSample();
		return;
	}

	//Profiler_EndSample();
	memcpy(dst, src, n);

}


PROF_HOOK_VOID(gameDrawWorld, "_Z13gameDrawWorldv", (void))
PROF_HOOK_VOID(worldDoDelayDrawTask, "_Z20worldDoDelayDrawTaskv", (void))
PROF_HOOK_VOID(EndFrameFence, "_ZN3JBE9DisplayPF13EndFrameFenceEv", (int* param_1), (param_1));
PROF_HOOK_VOID(SystemUpdate, "_ZN3JBE6System6UpdateEv", (void));
PROF_HOOK_VOID(SwapToFront, "_ZN3JBE9D3DDevice11SwapToFrontEi", (int* param_1), (param_1));
PROF_HOOK_VOID(DisplaySwap, "_ZN3JBE9DisplayPF4SwapEv", (void *param_1), (param_1));
PROF_HOOK_VOID(D3DDevice_CommitState, "_ZN3JBE9D3DDevice11CommitStateEv", (void *param_1), (param_1));
PROF_HOOK_VOID(Blit, "_ZN3JBE9DisplayPF4BlitEiiiiRKNS_13ShaderProgramEi", (int* param_1, int param_2, int param_3, int param_4, void *param_5, int param_6), (param_1, param_2, param_3, param_4, param_5, param_6));
PROF_HOOK_VOID(D3DDevice_UpdateComboStates, "_ZN3JBE9D3DDevice17UpdateComboStatesEv", (int *param_1), (param_1));
PROF_HOOK_VOID(D3DDevice_SetVertexShaderInputDirect, "_ZN3JBE9D3DDevice26SetVertexShaderInputDirectE", (int *thisptr, int param_2, int param_3, int param_4), (thisptr, param_2, param_3, param_4));
PROF_HOOK_VOID(D3DBaseTexture_BufferToOGL, "_ZN14D3DBaseTexture11BufferToOGLEP21RegisteredTextureDataPKvi", (void *thisptr, void* param_1, void const* param_2, int param_3), thisptr, param_1, param_2, param_3);
// _ZN6squish15DecompressImageEPhiiPKvi
PROF_HOOK_VOID(Squish_DecompressImage, "_ZN6squish15DecompressImageEPhiiPKvi", (unsigned char *pDst, int width, int height, const void *pSrc, int flags), pDst, width, height, pSrc, flags);
// _Z20decompressBlockAlphaPhS_iiii
PROF_HOOK_VOID(DecompressBlockAlpha, "_Z20decompressBlockAlphaPhS_iiii", (unsigned char *pDst, unsigned char *pSrc, int width, int height, int flags), pDst, pSrc, width, height, flags);
// void XGUnswizzleRect_NOTXDK(int param_1,uint param_2,uint param_3,uint param_4,int param_5,int param_6,int param_7)
PROF_HOOK_VOID(XGUnswizzleRect_NOTXDK, "XGUnswizzleRect_NOTXDK", (int param_1, uint32_t param_2, uint32_t param_3, uint32_t param_4, int param_5, int param_6, int param_7), param_1, param_2, param_3, param_4, param_5, param_6, param_7);
//_ZN3JBE9D3DDevice17TextureStageState7SetToGLEmN13XGSamplerType4EnumE
PROF_HOOK_VOID(JBE_D3DDevice_TextureStageState_ManyParams, "_ZN3JBE9D3DDevice17TextureStageState7SetToGLEmN13XGSamplerType4EnumE", (uint8_t *param_1, uint32_t param_2, uint32_t param_3), param_1, param_2, param_3);
// _ZN17TextureStageState7SetToGLEmP25RegisteredBaseTextureDataN13XGSamplerType4EnumE
PROF_HOOK_VOID(TextureStageState_SetToGL, "_ZN17TextureStageState7SetToGLEmP25RegisteredBaseTextureDataN13XGSamplerType4EnumE", (uint8_t *param_1, uint32_t param_2, uint32_t param_3), param_1, param_2, param_3);
PROF_HOOK_VOID(JBE_ThreadSleep, "_ZN3JBE6Thread5SleepEj", (unsigned int param_1), param_1);
so_hook D3DDevice_SetTextureStages_hook;
void D3DDevice_SetTextureStages(uint8_t *param_1, uint32_t param_2) {
	//logv_error("D3DDevice_SetTextureStages(%p, %u)\n", param_1, param_2);
	//Profiler_BeginSample("D3DDevice_SetTextureStages");
	SO_CONTINUE(void *, D3DDevice_SetTextureStages_hook, param_1, param_2);
	//Profiler_EndSample();
	//log_error("D3DDevice_SetTextureStages finished\n");
}


// so_hook D3DBaseTexture_BufferToOGLE_hk;
// void D3DBaseTexture_BufferToOGLE_wrap(void *thisptr, void *param_1, void const *param_2, int param_3) {
//    SO_CONTINUE(void *, D3DBaseTexture_BufferToOGLE_hk, thisptr, param_1, param_2, param_3);
// }

void install_prof_hooks(void) {
	PROF_ATTACH(gameDrawWorld, "_Z13gameDrawWorldv");
	PROF_ATTACH(worldDoDelayDrawTask, "_Z20worldDoDelayDrawTaskv");
	PROF_ATTACH(EndFrameFence, "_ZN3JBE9DisplayPF13EndFrameFenceEv");
	PROF_ATTACH(SystemUpdate, "_ZN3JBE6System6UpdateEv");
	PROF_ATTACH(SwapToFront, "_ZN3JBE9D3DDevice11SwapToFrontEi");
	PROF_ATTACH(DisplaySwap, "_ZN3JBE9DisplayPF4SwapEv");
	PROF_ATTACH(Blit, "_ZN3JBE9DisplayPF4BlitEiiiiRKNS_13ShaderProgramEi");
	PROF_ATTACH(D3DDevice_CommitState, "_ZN3JBE9D3DDevice11CommitStateEv");
	PROF_ATTACH(D3DBaseTexture_BufferToOGL, "_ZN14D3DBaseTexture11BufferToOGLEP21RegisteredTextureDataPKvi");
	PROF_ATTACH(Squish_DecompressImage, "_ZN6squish15DecompressImageEPhiiPKvi");
	PROF_ATTACH(DecompressBlockAlpha, "_Z20decompressBlockAlphaPhS_iiii");
	PROF_ATTACH(XGUnswizzleRect_NOTXDK, "XGUnswizzleRect_NOTXDK");
	PROF_ATTACH(TextureStageState_SetToGL, "_ZN3JBE9D3DDevice17TextureStageState7SetToGLEmN13XGSamplerType4EnumE");
	PROF_ATTACH(JBE_ThreadSleep, "_ZN3JBE6Thread5SleepEj");
}

so_hook machFrameStart_hook;

uint32_t frameStartCallCount = 0;
uint32_t frameStartCalledAtTime;
uint32_t frameStartToEndTime = 0;
		
void machFrameStart(int p) {
	sceKernelChangeThreadCpuAffinityMask(sceKernelGetThreadId(), SCE_KERNEL_CPU_MASK_USER_1);

	frameStartCalledAtTime = sceKernelGetProcessTimeLow();
	
	frameStartCallCount++;
	//logv_error("machFrameStart(%i)\n", p);
	SO_CONTINUE(void *, machFrameStart_hook, p);
}

so_hook mach_frameEnd_hook;
void machFrameEnd(int param_1) {
	uint32_t now = sceKernelGetProcessTimeLow();
	uint32_t delta = now - frameStartCalledAtTime;
	frameStartToEndTime += delta;
	SO_CONTINUE(void *, mach_frameEnd_hook, param_1);
}

uint64_t lastFrameTime = 0;
uint64_t totalFrameTime = 0;
so_hook JBE_D3DDevice_Swap_hook;
void JBE_D3DDevice_Swap(void *param_1, int param_2) {
	//Profiler_BeginSample("JBE::D3DDevice::Swap (render thread)");
	SO_CONTINUE(void *, JBE_D3DDevice_Swap_hook, param_1, param_2);
	//Profiler_EndSample();
}

so_hook DisplayPF_Swap_hook;
void DisplayPF_Swap(void *param_1) {
	SO_CONTINUE(void *, DisplayPF_Swap_hook, param_1);

	static int frameCount = 0;
	frameCount++;
	if (frameCount == 1) {
		lastFrameTime = sceKernelGetProcessTimeWide();
	}
	bool shouldLog = (frameCount % 100 == 0);
	if (log_profiler && shouldLog) {

		uint64_t timeNow = sceKernelGetProcessTimeWide();
		uint64_t deltaSinceLastDump = timeNow - lastFrameTime;
		lastFrameTime = timeNow;

		SceUID threadId = sceKernelGetThreadId();
		// int flag_skip_input = param_2 & 0x20;
		// // int flag_skip_wait = param_2 & 0x40;
		// // int flag_skip_vsync = param_2 & 0x10;
		logv_error("[t:%d]JBE::DisplayPF::Swap deltaSinceLastDump: %f ms. Thread ID: 0x%X\n", 
		 	frameCount, (float)deltaSinceLastDump / 1000.0f, threadId);

		logv_error("frameStartCallCount: %u", frameStartCallCount);
		logv_error("frameStartToEndTime: %u ms", frameStartToEndTime / 1000);
		
		frameStartCallCount = 0;
		frameStartToEndTime = 0;

		Profiler_PrintAll();
		Profiler_ResetAll();
	}
}

//D3DDevice_AsyncRenderCB_hook
so_hook D3DDevice_AsyncRenderCB_hook;
uintptr_t D3DDevice_ReadCommand_addr;
uintptr_t g_Singleton_addr;
uintptr_t displayPF_AcquireContext_addr;
uintptr_t displayPF_ReleaseContext_addr;

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

ReadCommandFn fnReadCommand;
void ReadCommandCustom(struct d3dDeviceFake *thisPtr) {	

	// Call the original ReadCommand function
	fnReadCommand(thisPtr);
}

void D3DDevice_AsyncRenderCB(void *device_ptr) {
	uint32_t threadId = sceKernelGetThreadId();
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
		if (log_profiler) {
			struct astruct *cmdData = thisPtr->cmd;
			uint8_t cmdType = cmdData->field0 & 0xFF;
			uint32_t cmdType32 = cmdData->field0 & 0xFF;
			// Profile with the command type
			const char *label = gCmdLabels[cmdType];
	
			//Profiler_BeginSample(label);
			fnReadCommand(device);
			//Profiler_EndSample();
		}
		else {
#if PROFILER_ENABLED
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

_Static_assert(offsetof(struct d3dDeviceFake, cmd) == 0x8,
               "`cmd` is not at offset 0x8 – fix the struct or add packed!");


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

so_hook System_HasNEON_hook;
bool System_HasNEON(void) {
	//logv_error("System_HasNEON()\n");
	// Check if the CPU has NEON support
	bool result = SO_CONTINUE(bool, System_HasNEON_hook);
	logv_error("System_HasNEON: %s\n", result ? "true" : "false");
	return result;
}

typedef struct {
    void* field0_0x0;
    uint32_t size;
} ParamBlock;

typedef struct {
    void* cmdBufferWritePtr;
    int frameNumberOfCmdWritter;
    void* cmdBufferStart;
    int frameNumberOfCmdReader;
    void* tmpCmdWrite;
	uint8_t unknown[14]; // padding or unknown data
	uint8_t frameBitToToggle;
	uint8_t unknown2;
    void* field16_0x24;
    void* cmdBufferEnd;
    int commandSize;
} D3DDevice_2;

_Static_assert(offsetof(D3DDevice_2, tmpCmdWrite) == 0x10,
			   "`tmpCmdWrite` is not at offset 0x10 – fix the struct or add packed!");
// assert that field16_0x24 is at offset 0x24
_Static_assert(offsetof(D3DDevice_2, field16_0x24) == 0x24,
			   "`field16_0x24` is not at offset 0x24 – fix the struct or add packed!");

void WriteCommand_Optimized(D3DDevice_2* this, 
                                     const int* param_1, 
                                     const ParamBlock* paramsBlock, 
                                     const uint32_t* param_3) {
	// Cache frequently accessed struct members to reduce pointer dereferencing
	uint32_t *cmdBufWritePtr = (uint32_t *)this->cmdBufferWritePtr;
	uint32_t *cmdBufferStart = (uint32_t *)this->cmdBufferStart;
	uint32_t *cmdBufferEnd = (uint32_t *)this->cmdBufferEnd;
	int cmdWritterFrameNum = this->frameNumberOfCmdWritter;
	int cmdReaderFrameNum = this->frameNumberOfCmdReader;
	
	// Pre-calculate total size once
	const uint32_t paramSize = paramsBlock->size;
	const int totalSizeWords = ((paramSize + 3U) >> 2) + 4;
	
	// Fast path for common case: totalSizeWords = 8 (most frequent)
	if (__builtin_expect(totalSizeWords == 8, 1)) {
		// Fast buffer check for size 8
		if (__builtin_expect(cmdBufferEnd >= cmdBufWritePtr + 8, 1)) {
			// Common case: sufficient buffer space available
			this->commandSize = 8;
			
			// Direct sequential writes for optimal cache performance
			cmdBufWritePtr[0] = 0x819; // (8 << 8) | 0x19
			cmdBufWritePtr[1] = *param_1;
			
			const uint32_t paramWords = (paramSize + 3U) >> 2;
			cmdBufWritePtr[2] = paramWords;
			
			// Optimized copy for common param sizes: 64, 128, 192 bytes
			const uint32_t* src = (const uint32_t*)paramsBlock->field0_0x0;
			uint32_t* dst = &cmdBufWritePtr[3];
			
			// Fast unrolled copies for common sizes
			if (__builtin_expect(paramSize == 64, 1)) {
				// 64 bytes = 16 words - unrolled copy
				dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
				dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7];
				dst[8] = src[8]; dst[9] = src[9]; dst[10] = src[10]; dst[11] = src[11];
				dst[12] = src[12]; dst[13] = src[13]; dst[14] = src[14]; dst[15] = src[15];
			} else if (__builtin_expect(paramSize == 128, 1)) {
				// 128 bytes = 32 words - unrolled copy in blocks
				for (uint32_t i = 0; i < 32; i += 8) {
					dst[i] = src[i]; dst[i+1] = src[i+1]; dst[i+2] = src[i+2]; dst[i+3] = src[i+3];
					dst[i+4] = src[i+4]; dst[i+5] = src[i+5]; dst[i+6] = src[i+6]; dst[i+7] = src[i+7];
				}
			} else if (__builtin_expect(paramSize == 192, 1)) {
				// 192 bytes = 48 words - unrolled copy in blocks
				for (uint32_t i = 0; i < 48; i += 8) {
					dst[i] = src[i]; dst[i+1] = src[i+1]; dst[i+2] = src[i+2]; dst[i+3] = src[i+3];
					dst[i+4] = src[i+4]; dst[i+5] = src[i+5]; dst[i+6] = src[i+6]; dst[i+7] = src[i+7];
				}
			} else {
				// Fallback for other sizes
				__aeabi_memcpy(dst, paramsBlock->field0_0x0, paramSize);
			}
			
			cmdBufWritePtr[3 + paramWords] = *param_3;
			
			// Update pointers with single calculation
			this->tmpCmdWrite = (void*)&cmdBufWritePtr[4 + paramWords];
			this->cmdBufferWritePtr = (void*)(cmdBufWritePtr + 8);
			return;
		}
	}
	
	// Slow path for buffer wrapping or non-standard sizes
	if (__builtin_expect(cmdBufferEnd < cmdBufWritePtr + totalSizeWords, 0))
	{
		// Wait for buffer space, using cached frame numbers
		while ((cmdWritterFrameNum != cmdReaderFrameNum) && (cmdBufWritePtr == cmdBufferStart)) 
		{
			usleep(1000);
			cmdWritterFrameNum = this->frameNumberOfCmdWritter;
			cmdReaderFrameNum = this->frameNumberOfCmdReader;
			cmdBufWritePtr = (uint32_t *)this->cmdBufferWritePtr;
		}

		*cmdBufWritePtr = 10;
		cmdBufWritePtr = (uint32_t *)this->field16_0x24;
		cmdWritterFrameNum++;
		
		// Update struct members once
		this->cmdBufferWritePtr = cmdBufWritePtr;
		this->frameNumberOfCmdWritter = cmdWritterFrameNum;
	}
	
	// Set command size and temp write pointer
	this->commandSize = totalSizeWords;
	uint32_t *tmpCmdWrite = cmdBufWritePtr;
	
	// Optimized frame sync check
	if (__builtin_expect(cmdWritterFrameNum != cmdReaderFrameNum, 0)) 
	{
		uint32_t *cmdEndPtr = cmdBufWritePtr + totalSizeWords;
		do
		{
			if ((cmdEndPtr <= cmdBufferStart) || (cmdBufferStart < cmdBufWritePtr)) break;
			usleep(1000);
			tmpCmdWrite = (uint32_t *)this->tmpCmdWrite;
		} while (this->frameNumberOfCmdWritter != this->frameNumberOfCmdReader);
		cmdBufWritePtr = tmpCmdWrite;
	}
	
	// Write command header efficiently
	tmpCmdWrite = cmdBufWritePtr + 1;
	*cmdBufWritePtr = (totalSizeWords << 8) | 0x19;
	
	// Write param_1
	*tmpCmdWrite = *param_1;
	tmpCmdWrite++;
	
	// Calculate words needed for params, cache the calculation
	const uint32_t paramWords = (paramSize + 3U) >> 2;
	*tmpCmdWrite = paramWords;
	tmpCmdWrite++;
	
	// Direct memory copy using cached pointers - optimized for common sizes
	const uint32_t* src = (const uint32_t*)paramsBlock->field0_0x0;
	uint32_t* dst = tmpCmdWrite;
	
	if (__builtin_expect(paramSize == 64, 1)) {
		// 64 bytes = 16 words - unrolled copy
		dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
		dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7];
		dst[8] = src[8]; dst[9] = src[9]; dst[10] = src[10]; dst[11] = src[11];
		dst[12] = src[12]; dst[13] = src[13]; dst[14] = src[14]; dst[15] = src[15];
	} else if (__builtin_expect(paramSize == 128, 1)) {
		// 128 bytes = 32 words - unrolled copy in blocks
		for (uint32_t i = 0; i < 32; i += 8) {
			dst[i] = src[i]; dst[i+1] = src[i+1]; dst[i+2] = src[i+2]; dst[i+3] = src[i+3];
			dst[i+4] = src[i+4]; dst[i+5] = src[i+5]; dst[i+6] = src[i+6]; dst[i+7] = src[i+7];
		}
	} else if (__builtin_expect(paramSize == 192, 1)) {
		// 192 bytes = 48 words - unrolled copy in blocks
		for (uint32_t i = 0; i < 48; i += 8) {
			dst[i] = src[i]; dst[i+1] = src[i+1]; dst[i+2] = src[i+2]; dst[i+3] = src[i+3];
			dst[i+4] = src[i+4]; dst[i+5] = src[i+5]; dst[i+6] = src[i+6]; dst[i+7] = src[i+7];
		}
	} else {
		// Fallback for other sizes
		__aeabi_memcpy(dst, paramsBlock->field0_0x0, paramSize);
	}
	tmpCmdWrite += paramWords;
	
	// Write final parameter
	*tmpCmdWrite = *param_3;
	tmpCmdWrite++;
	
	// Update struct members with final values
	this->tmpCmdWrite = (void*)tmpCmdWrite;
	this->cmdBufferWritePtr = (void *)((uint8_t*)this->cmdBufferWritePtr + totalSizeWords * 4);
}

so_hook WriteCommand_hook;

so_hook D3DDevice_Swap_hook;


uint64_t lastFrameTimeMainThread = 0;
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
so_hook MEMAllocFromExpHeapEx_hook;
void *MEMAllocFromExpHeapEx(void *heap, int size, int flags) {
	//logv_error("MEMAllocFromExpHeapEx(heap: %p, size: %d, flags: %d)\n", heap, size, flags);
	void *res = SO_CONTINUE(void *, MEMAllocFromExpHeapEx_hook, heap, size, flags);
	if (res == NULL) {
		logv_error("MEMAllocFromExpHeapEx returned NULL for heap %p, size %d, flags %d\n", heap, size, flags);
	}
	//logv_error("MEMAllocFromExpHeapEx returned: %p\n", res);
	return res;
}
so_hook memInit_hook;
void memInit(void *param_1, int param_2) {
	logv_error("memInit(%p, %d)\n", param_1, param_2);
	// free the original memory
	free(param_1);

	// allocate new memory. original is 0x4000000 which is like 64MB
	int newSize = 0x2000000;
	void *newMem = malloc(newSize);

	SO_CONTINUE(void *, memInit_hook, newMem, newSize);
	//log_error("memInit finished\n");
}
so_hook memAlloc_hook;
void *memAlloc(int size, const char *name) {
	//logv_error("memAlloc(size: %d, name: %s)\n", size, name);
	void *res = SO_CONTINUE(void *, memAlloc_hook, size, name);
	if (res == NULL) {
		logv_error("memAlloc returned NULL for size %d, name %s\n", size, name);
	}
	//logv_error("memAlloc returned: %p\n", res);
	return res;
}

// make sure it's packed and not padded

#define NAN(x) ((x) != (x)) // check if x is NaN
struct __attribute__((packed)) FrustumIdx {
	uint8_t xsel;
    uint8_t ysel;
    uint8_t zsel;
};
typedef struct FrustumIdx FrustumIdx;

struct FrustumIdx *g_frustumVertexIndices;
float *g_worldFrustum;

_Static_assert(sizeof(struct FrustumIdx) == 3, "FrustumIdx must be packed to 3 bytes");

so_hook worldClipCubeToFrustum_hook;
uint32_t worldClipCubeToFrustum(float *cubeVertices, int clippedPlanes)
{
    //Profiler_BeginSample("worldClipCubeToFrustum");

    // Rename cryptic variables to meaningful names
    int planeNumber;             // Better than planeIndex starting at -6
    FrustumIdx *planeIndices;
    float planeDistance;         
    float planeNormalY;            
    float planeNormalZ;
    
    /* Frustum culling function: tests if a 3D cube intersects with viewing frustum.
       Returns bitfield of planes that clip the cube (0 = fully inside frustum) */
    
    /* Loop through 6 frustum planes (left, right, top, bottom, near, far) */
    float *currentPlane = (float *)g_worldFrustum;
    planeIndices = g_frustumVertexIndices;
    
    for (planeNumber = 0; planeNumber < 6; planeNumber++) {
        uint32_t planeBitMask = 1 << planeNumber;
        
        /* Skip planes already marked as clipped */
        if ((clippedPlanes & planeBitMask) == 0) {
            // Access frustum plane data: each plane has 4 floats (nx, ny, nz, d)
            float planeNormalX = currentPlane[0];
            planeNormalY = currentPlane[1];
            planeNormalZ = currentPlane[2];
            planeDistance = currentPlane[3];
            
			uint8_t zsel = planeIndices[-1].zsel;
			uint8_t xsel = planeIndices->xsel;
			uint8_t ysel = planeIndices->ysel;

            /* Test cube's "far" vertex against plane: if behind plane, mark this plane as clipping */
            if (planeDistance + planeNormalX *
                cubeVertices[zsel] +
                planeNormalY * cubeVertices[xsel + 2] +
                planeNormalZ * cubeVertices[ysel + 4] < 0.0) {
                
                //Profiler_EndSample();
                return 0;
            }
            
            float farVertexDistance = planeDistance + planeNormalX *
                          cubeVertices[zsel ^ 1] +
                          planeNormalY * cubeVertices[(xsel ^ 1U) + 2] +
                          planeNormalZ * cubeVertices[(ysel ^ 1U) + 4];
            
            if (farVertexDistance < 0.0 == NAN(farVertexDistance)) {
                clippedPlanes = clippedPlanes | planeBitMask;
            }
        }
        
        /* Move to next frustum plane */
        currentPlane += 4;  // Each plane has 4 floats
        planeIndices++;
    }
    
    //Profiler_EndSample();
    return clippedPlanes;
}


so_hook worldClipCubeToClipFrustum_hook;
uint32_t worldClipCubeToClipFrustum(float *cubeVertices, int clippedPlanes)
{	
	//Profiler_BeginSample("worldClipCubeToClipFrustum");
	uint32_t x = SO_CONTINUE(uint32_t, worldClipCubeToClipFrustum_hook, cubeVertices, clippedPlanes);
	//Profiler_EndSample();
	//log_error("worldClipCubeToClipFrustum finished\n");
	return x;
}

so_hook worldClipCubeToFrustumOnce_hook;
uint32_t worldClipCubeToFrustumOnce(float *cubeVertices, int clippedPlanes)
{
	//Profiler_BeginSample("worldClipCubeToFrustumOnce");
	uint32_t x = SO_CONTINUE(uint32_t, worldClipCubeToFrustumOnce_hook, cubeVertices, clippedPlanes);
	//Profiler_EndSample();
	return x;
}


// _ZN3JBE9D3DDevice8GetFVFVSEPNS0_24FVFVertexShaderContainerERm
so_hook D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm_hook;
int D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm(void *thisptr, uintptr_t* container, uint32_t param_2) {
	//logv_error("D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm(%p, %p, %u)\n", thisptr, container, param_2);
	//Profiler_BeginSample("D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm");
	int result = SO_CONTINUE(int, D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm_hook, thisptr, container, param_2);
	//Profiler_EndSample();
	//logv_error("D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm finished with result %d\n", result);
	return result;
}

// _ZN14D3DBaseTexture11BufferToOGLEP21RegisteredTextureDataPKvi 
so_hook D3DBaseTexture_BufferToOGL_hook;
void D3DBaseTexture_BufferToOGL(void *pThis, void *pTexData, const void *pBuffer, int size) {
	//logv_error("D3DBaseTexture_BufferToOGL(%p, %p, %p, %d)\n", pThis, pTexData, pBuffer, size);
	//Profiler_BeginSample("D3DBaseTexture_BufferToOGL");
	SO_CONTINUE(void *, D3DBaseTexture_BufferToOGL_hook, pThis, pTexData, pBuffer, size);
	//Profiler_EndSample();
	//log_error("D3DBaseTexture_BufferToOGL finished\n");
}

so_hook usprintf_hook;

// Custom sprintf-like function that outputs to wide character (ushort) buffer
void usprintf_patched(uint16_t *output_buffer, const uint16_t *format_string, 
              float arg1, float arg2, float arg3, float arg4, float arg5, float arg6, float arg7)
{
    const uint16_t *format_ptr;
    uint16_t *output_ptr;
    uint16_t current_char;
    char temp_buffer[256];
    char *temp_ptr;
    char c;
    float args[7] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7};
    int arg_index = 0;
    
    format_ptr = format_string;
    output_ptr = output_buffer;
    
    while (1) {
        current_char = *format_ptr;
        
        // Copy regular characters until we hit '%' or null terminator
        while (current_char != 0x25 && current_char != 0) { // 0x25 = '%'
            *output_ptr = current_char;
            output_ptr++;
            format_ptr++;
            current_char = *format_ptr;
        }
        
        if (current_char == 0) {
            *output_ptr = 0; // Null terminate
            return;
        }
        
        // Found '%', move to next character
        format_ptr++;
        current_char = *format_ptr;
        
        // Handle '%%' case (literal %)
        if (current_char == 0x25) {
            *output_ptr = 0x25;
            output_ptr++;
            format_ptr++;
            continue;
        }
        
        // Handle format specifiers
        switch (current_char) {
            case 'd': // Integer
                snprintf(temp_buffer, sizeof(temp_buffer), "%d", (int)args[arg_index]);
                temp_ptr = temp_buffer;
                while ((c = *temp_ptr) != '\0') {
                    *output_ptr = (uint16_t)c;
                    output_ptr++;
                    temp_ptr++;
                }
                arg_index++;
                break;
                
            case 'c': // Character
                *output_ptr = (uint16_t)(int)args[arg_index];
                output_ptr++;
                arg_index++;
                break;
                
            case 's': // String (assuming string pointer stored as float - needs verification)
                {
					char *str_ptr = (char*)((uintptr_t)args[arg_index]);

					// HACK to fix the stupid bug with plural/singular strings
					if (str_ptr == 0x9848F200) {
						str_ptr = LOC(0x0009f1e2);
					}
					else if (str_ptr == 0x9849AF00) {
						str_ptr = LOC(0x0009f1e3);
					}
                    
                    if (str_ptr != NULL) {
                        strcpy(temp_buffer, str_ptr);
                        temp_ptr = temp_buffer;
                        while ((c = *temp_ptr) != '\0') {
                            *output_ptr = (uint16_t)c;
                            output_ptr++;
                            temp_ptr++;
                        }
                    }
                    arg_index++;
                }
                break;
				
            case 'f': // Float
                snprintf(temp_buffer, sizeof(temp_buffer), "%f", args[arg_index]);
                temp_ptr = temp_buffer;
                while ((c = *temp_ptr) != '\0') {
                    *output_ptr = (uint16_t)c;
                    output_ptr++;
                    temp_ptr++;
                }
                arg_index++;
                break;
                
            default:
                // Unknown format specifier, just copy it
                *output_ptr = current_char;
                output_ptr++;
                break;
        }
        
        format_ptr++; // Move past the format specifier
    }
}

so_hook XGGetPixelBufferMinAlpha_hook;
uint XGGetPixelBufferMinAlpha(uint8_t (*param_1) [16], uint32_t param_2,int param_3,int param_4) {
	//return 0;
	return SO_CONTINUE(uint, XGGetPixelBufferMinAlpha_hook, param_1, param_2, param_3, param_4);
}

so_hook XGGetPixelBufferMaxAlpha_hook;
uint XGGetPixelBufferMaxAlpha(uint8_t (*param_1) [16], uint32_t param_2,int param_3,int param_4) {
	//return 255;
	return SO_CONTINUE(uint, XGGetPixelBufferMaxAlpha_hook, param_1, param_2, param_3, param_4);
}

typedef struct registeredPalette {
	uint32_t paletteAddr;
	uint32_t glTexId;
} registeredPalette;

uint32_t curTexIndex = 0;
registeredPalette registeredPalettes[1024];

void registerPalette(uint32_t paletteAddr, uint32_t glTexId) {
	if (curTexIndex < 1024) {
		registeredPalettes[curTexIndex].paletteAddr = paletteAddr;
		registeredPalettes[curTexIndex].glTexId = glTexId;
		curTexIndex++;
		logv_error("registerPalette: registered palette %p with glTexId %u\n", (void*)paletteAddr, glTexId);
	}
	else {
		log_error("registerPalette: exceeded max registered palettes\n");
	}
}

int isPaletteRegistered(uint32_t paletteAddr) {
	for (uint32_t i = 0; i < curTexIndex; i++) {
		if (registeredPalettes[i].paletteAddr == paletteAddr) {
			logv_error("isPaletteRegistered: palette %p is already registered with glTexId %u\n", (void*)paletteAddr, registeredPalettes[i].glTexId);
			return registeredPalettes[i].glTexId;
		}
	}
	return 0;
}

unsigned char Palette[256][4] = {
    {0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x14},
    {0x00,0x00,0x00,0x0A},
    {0x02,0x02,0x02,0xF6},
    {0x02,0x02,0x02,0x42},
    {0x00,0x00,0x02,0x22},
    {0x00,0x02,0x02,0x32},
    {0x00,0x00,0x02,0x3A},
    {0x02,0x02,0x04,0xD6},
    {0x02,0x04,0x04,0x2C},
    {0x02,0x02,0x04,0xEC},
    {0x02,0x02,0x04,0x66},
    {0x06,0x08,0x06,0xFE},
    {0x04,0x04,0x06,0xFE},
    {0x03,0x06,0x09,0x88},
    {0x06,0x08,0x08,0xA0},
    {0x04,0x04,0x09,0x5A},
    {0x03,0x06,0x09,0xBE},
    {0x08,0x08,0x08,0xFE},
    {0x03,0x06,0x09,0x4C},
    {0x08,0x0A,0x08,0xFE},
    {0x0A,0x0F,0x0A,0xFE},
    {0x0A,0x0C,0x0A,0xFE},
    {0x08,0x08,0x0A,0xFE},
    {0x0A,0x0A,0x0A,0xFE},
    {0x03,0x08,0x0D,0xF4},
    {0x05,0x08,0x0D,0x7A},
    {0x0C,0x0E,0x0C,0xFE},
    {0x05,0x08,0x0D,0xCA},
    {0x0C,0x11,0x0C,0xFE},
    {0x05,0x08,0x0D,0xFE},
    {0x0C,0x0C,0x0C,0xFE},
    {0x0E,0x10,0x0E,0xFE},
    {0x07,0x0A,0x0F,0xE6},
    {0x0E,0x13,0x0E,0xFE},
    {0x0C,0x0C,0x0E,0xFE},
    {0x09,0x0C,0x0F,0x92},
    {0x0F,0x17,0x0F,0xFE},
    {0x10,0x12,0x10,0xFE},
    {0x07,0x0C,0x11,0xFE},
    {0x0B,0x0E,0x11,0xA8},
    {0x0E,0x10,0x10,0xFE},
    {0x09,0x0C,0x11,0xD8},
    {0x10,0x15,0x10,0xFE},
    {0x09,0x0E,0x13,0xFE},
    {0x11,0x19,0x11,0xFE},
    {0x0B,0x10,0x13,0x48},
    {0x0B,0x0E,0x13,0xB6},
    {0x0F,0x15,0x12,0xFE},
    {0x12,0x17,0x12,0xFE},
    {0x0D,0x10,0x13,0xC0},
    {0x14,0x19,0x14,0xFE},
    {0x13,0x1B,0x13,0xFE},
    {0x0F,0x14,0x17,0x3C},
    {0x15,0x20,0x15,0xFE},
    {0x0D,0x12,0x17,0xC6},
    {0x06,0x0E,0x18,0xF8},
    {0x15,0x1D,0x15,0xFE},
    {0x15,0x22,0x15,0xFE},
    {0x13,0x1B,0x16,0xFE},
    {0x0A,0x12,0x17,0xDC},
    {0x11,0x14,0x17,0xFE},
    {0x11,0x16,0x16,0x3C},
    {0x13,0x19,0x16,0xFE},
    {0x0B,0x10,0x18,0xFE},
    {0x0F,0x16,0x19,0xDE},
    {0x13,0x18,0x18,0x3C},
    {0x15,0x1D,0x18,0xFE},
    {0x17,0x24,0x17,0xFE},
    {0x19,0x26,0x19,0xFE},
    {0x0C,0x16,0x1C,0xA0},
    {0x15,0x22,0x1A,0xFE},
    {0x0C,0x16,0x1C,0xFA},
    {0x0F,0x14,0x1C,0xF4},
    {0x15,0x1A,0x1A,0x3C},
    {0x0A,0x12,0x1C,0xFE},
    {0x17,0x21,0x1A,0xFE},
    {0x1B,0x2A,0x1B,0xFE},
    {0x19,0x23,0x1C,0xFE},
    {0x13,0x1A,0x1D,0x52},
    {0x17,0x24,0x1C,0xFE},
    {0x1B,0x28,0x1B,0xFE},
    {0x0A,0x14,0x1E,0xFE},
    {0x0B,0x18,0x20,0xE4},
    {0x0B,0x18,0x20,0x88},
    {0x10,0x18,0x20,0xF6},
    {0x16,0x26,0x1E,0xFE},
    {0x12,0x1A,0x1F,0xB4},
    {0x1D,0x2C,0x1D,0xFE},
    {0x15,0x1A,0x1F,0xFE},
    {0x1B,0x28,0x1D,0xFE},
    {0x1B,0x2A,0x1D,0xFE},
    {0x0E,0x16,0x20,0xFE},
    {0x19,0x23,0x1E,0xFE},
    {0x18,0x28,0x1E,0xFE},
    {0x1F,0x31,0x1F,0xFE},
    {0x0D,0x1A,0x22,0x88},
    {0x1C,0x2F,0x1F,0xFE},
    {0x0B,0x18,0x23,0x88},
    {0x0B,0x18,0x25,0xFE},
    {0x1D,0x2A,0x22,0xFE},
    {0x1F,0x2C,0x21,0xFE},
    {0x21,0x30,0x21,0xFE},
    {0x1C,0x2C,0x22,0xFE},
    {0x0D,0x1D,0x24,0x88},
    {0x1F,0x2E,0x21,0xFE},
    {0x22,0x3C,0x22,0xFE},
    {0x0F,0x1C,0x27,0xFE},
    {0x20,0x35,0x23,0xFE},
    {0x1E,0x30,0x23,0xFE},
    {0x14,0x19,0x26,0xFE},
    {0x1C,0x2E,0x24,0xFE},
    {0x20,0x33,0x23,0xFE},
    {0x1E,0x30,0x26,0xFE},
    {0x13,0x20,0x28,0xFE},
    {0x21,0x2B,0x26,0xFE},
    {0x18,0x20,0x28,0x96},
    {0x16,0x20,0x28,0xA2},
    {0x0D,0x1C,0x29,0xFE},
    {0x11,0x1E,0x2B,0xEA},
    {0x20,0x35,0x28,0xFE},
    {0x15,0x22,0x2A,0xF0},
    {0x22,0x32,0x28,0xFE},
    {0x17,0x27,0x2A,0xFE},
    {0x0F,0x1E,0x2B,0xFE},
    {0x1D,0x22,0x2A,0xC0},
    {0x22,0x34,0x27,0xFE},
    {0x16,0x20,0x2A,0xFE},
    {0x20,0x32,0x28,0xFE},
    {0x0E,0x21,0x2B,0xFE},
    {0x26,0x3B,0x26,0xFE},
    {0x22,0x37,0x27,0xFE},
    {0x13,0x25,0x2D,0xFE},
    {0x22,0x37,0x2A,0xFE},
    {0x15,0x22,0x2D,0xFE},
    {0x24,0x39,0x29,0xFE},
    {0x0E,0x20,0x2D,0xFE},
    {0x24,0x36,0x29,0xFE},
    {0x25,0x32,0x2A,0xFE},
    {0x1E,0x26,0x2E,0xB8},
    {0x10,0x20,0x30,0xEE},
    {0x1E,0x28,0x2E,0x5E},
    {0x24,0x39,0x2C,0xFE},
    {0x10,0x22,0x2F,0xFE},
    {0x1A,0x24,0x2E,0xFE},
    {0x15,0x27,0x2F,0xFE},
    {0x28,0x3F,0x2D,0xFE},
    {0x2C,0x48,0x2C,0xFE},
    {0x0E,0x22,0x32,0xFE},
    {0x14,0x27,0x31,0xFE},
    {0x26,0x3B,0x2E,0xFE},
    {0x1F,0x36,0x2F,0xFE},
    {0x2A,0x41,0x2D,0xFE},
    {0x15,0x22,0x31,0xFE},
    {0x26,0x3D,0x2D,0xFE},
    {0x19,0x26,0x31,0xFE},
    {0x23,0x3B,0x2E,0xFE},
    {0x12,0x24,0x31,0xFE},
    {0x2A,0x44,0x2C,0xFE},
    {0x10,0x24,0x34,0xFE},
    {0x22,0x2A,0x32,0xA8},
    {0x19,0x28,0x33,0xFE},
    {0x26,0x36,0x30,0xFE},
    {0x2A,0x3F,0x2F,0xFE},
    {0x2A,0x41,0x2F,0xFE},
    {0x2C,0x43,0x2F,0xFE},
    {0x28,0x3D,0x30,0xFE},
    {0x14,0x29,0x36,0xFE},
    {0x2D,0x3A,0x32,0xFE},
    {0x28,0x3A,0x32,0xFE},
    {0x0F,0x24,0x36,0xFE},
    {0x1B,0x2D,0x35,0xFE},
    {0x19,0x28,0x35,0xFE},
    {0x26,0x2E,0x36,0xAE},
    {0x16,0x2B,0x38,0xFE},
    {0x2C,0x3E,0x34,0xFE},
    {0x2E,0x45,0x33,0xFE},
    {0x11,0x29,0x38,0xFE},
    {0x1F,0x2F,0x36,0xFE},
    {0x2E,0x48,0x33,0xFE},
    {0x0F,0x26,0x39,0xFE},
    {0x2C,0x43,0x33,0xFE},
    {0x16,0x23,0x38,0xFE},
    {0x30,0x4A,0x35,0xFE},
    {0x1A,0x2F,0x39,0xFE},
    {0x1D,0x2C,0x39,0xFE},
    {0x32,0x4C,0x34,0xFE},
    {0x15,0x2D,0x3A,0xFE},
    {0x1C,0x33,0x39,0xFE},
    {0x11,0x28,0x3B,0xFE},
    {0x13,0x2A,0x3D,0xFE},
    {0x2F,0x49,0x37,0xFE},
    {0x1C,0x31,0x3B,0xFE},
    {0x2C,0x36,0x3C,0xB8},
    {0x1E,0x38,0x3D,0xFE},
    {0x17,0x2F,0x3E,0xFE},
    {0x1E,0x33,0x3D,0xFE},
    {0x32,0x49,0x39,0xFE},
    {0x33,0x50,0x39,0xFE},
    {0x20,0x3A,0x3C,0xFE},
    {0x19,0x31,0x3E,0xFE},
    {0x27,0x39,0x3E,0xFE},
    {0x19,0x33,0x40,0xFE},
    {0x25,0x34,0x3F,0xFE},
    {0x2E,0x3B,0x3D,0xFE},
    {0x1E,0x35,0x3F,0xFE},
    {0x29,0x3B,0x40,0xFE},
    {0x25,0x32,0x41,0xEA},
    {0x1D,0x39,0x44,0xFE},
    {0x36,0x48,0x40,0xFE},
    {0x24,0x39,0x43,0xFE},
    {0x24,0x3E,0x43,0xFE},
    {0x1D,0x35,0x44,0xFE},
    {0x22,0x37,0x44,0xFE},
    {0x22,0x34,0x44,0xFE},
    {0x18,0x35,0x45,0xFE},
    {0x21,0x3B,0x43,0xFE},
    {0x2B,0x3D,0x42,0xFE},
    {0x3A,0x4F,0x3F,0xFE},
    {0x2A,0x42,0x44,0xFE},
    {0x1F,0x39,0x46,0xFE},
    {0x2B,0x3D,0x45,0xFE},
    {0x1F,0x3B,0x46,0xFE},
    {0x23,0x3D,0x48,0xFE},
    {0x21,0x3D,0x4A,0xFA},
    {0x25,0x3D,0x4A,0xFE},
    {0x21,0x3D,0x4A,0xFE},
    {0x25,0x42,0x49,0xFE},
    {0x2A,0x3F,0x49,0xFE},
    {0x2A,0x41,0x49,0xFE},
    {0x22,0x42,0x4C,0xFA},
    {0x21,0x3B,0x4D,0xFE},
    {0x25,0x3D,0x4C,0xFA},
    {0x27,0x41,0x4C,0xFE},
    {0x29,0x46,0x4B,0xFE},
    {0x27,0x3C,0x4E,0xFA},
    {0x27,0x3F,0x4E,0xFA},
    {0x24,0x41,0x51,0xFA},
    {0x30,0x42,0x4F,0xFC},
    {0x2C,0x40,0x50,0xFE},
    {0x23,0x3A,0x51,0xFA},
    {0x2B,0x48,0x4F,0xFE},
    {0x24,0x43,0x50,0xFE},
    {0x20,0x3F,0x51,0xFE},
    {0x24,0x46,0x50,0xFE},
    {0x22,0x3C,0x51,0xFA},
    {0x32,0x49,0x53,0xFE},
    {0x2C,0x4C,0x56,0xFE},
    {0x33,0x4D,0x58,0xFE},
    {0x35,0x4F,0x5A,0xFE},
    {0x37,0x53,0x5E,0xFE},
    {0x3D,0x57,0x5F,0xFE},
    {0x41,0x5E,0x63,0xFE},
    {0x40,0x60,0x6A,0xFE},
    {0x47,0x66,0x6E,0xFE},
    {0x4F,0x69,0x71,0xFE}
};


so_hook ProcessAndUploadTexture_hook;
void ProcessAndUploadTexture
              (int glTarget,uint8_t *sourceTextureData,uint formatToSwitchParam,int isSwizzled,
               int isCompressed,uint width,uint height,uint level,uint sourcePitch,int paddingFlag,
               uint8_t * palette,uint allocateNewTexture,int keepSwizzled,ushort *alphaRange)
{
	// SO_CONTINUE(void *, ProcessAndUploadTexture_hook, glTarget, sourceTextureData, formatToSwitchParam,
	// 	isSwizzled, isCompressed, width, height, level, sourcePitch, paddingFlag,
	// 	palette, allocateNewTexture, keepSwizzled, alphaRange);
	// return;
	
	// // log the parameters with name
	// logv_error("ProcessAndUploadTexture(glTarget: %d, sourceTextureData: %p, formatToSwitchParam: %d, isSwizzled: %d, isCompressed: %d, width: %u, height: %u, level: %u, sourcePitch: %u, paddingFlag: %d, palette: %u, allocateNewTexture: %d, keepSwizzled: %d, alphaRange: %p)\n",
	// 	glTarget, sourceTextureData, formatToSwitchParam, isSwizzled, isCompressed,
	//  	width, height, level, sourcePitch, paddingFlag, palette, allocateNewTexture, keepSwizzled, alphaRange);


	if (level != 1) {
		return;
	}

	if ((formatToSwitchParam & 0xffffff7f) == 0xb && formatToSwitchParam == 139 && sourcePitch != 4096 && palette) 
	{
		//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		// For index texture
		glTexImage2D_fake(glTarget,
			0, // level
			GL_LUMINANCE, //GL_RGBA, 
			width, // whatever original width was passed to DoTheFinalGPUUpload
			height, // whatever original height
			0, // border
			GL_LUMINANCE, //GL_RGBA, // format = internalFormat
			GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, // type ?
			sourceTextureData); // data, comes from the function args

 		glTexParameteri(glTarget,0x813d,level - 1);

		if (palette != NULL) {
			//palette = (uint8_t*)Palette;
			uint32_t paletteId = isPaletteRegistered((uint32_t)palette);
			if (paletteId == 0) {
				// not registered yet
				uint32_t paletteId;		
				glGenTextures(1, &paletteId);
				registerPalette((uint32_t)palette, paletteId);
			}

			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, paletteId);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // defensive; rows are 256*4 = 1024 (already aligned)
			// fin
			glTexImage2D(GL_TEXTURE_2D, 0,
						GL_RGBA,              // internalFormat
						256, 1, 0,
						GL_RGBA,              // format
						GL_UNSIGNED_BYTE,
						palette);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			//glUniform1i(glGetUniformLocation(program, "u2"), 2);
		}

		return;
	}

	if (sourcePitch != 4096) {
		sourcePitch = width;
	}

	SO_CONTINUE(void *, ProcessAndUploadTexture_hook, glTarget, sourceTextureData, formatToSwitchParam,
		isSwizzled, isCompressed, width, height, level, sourcePitch, paddingFlag,
		palette, allocateNewTexture, keepSwizzled, alphaRange);
}

so_hook DoTheFinalGPUUpload_hook;
void DoTheFinalGPUUpload(uint32_t glTarget, uint32_t level, uint8_t (*pixelData) [16],
                        uint32_t textureFormatToSwitch, uint width, uint height, uint32_t imageSize,
						int shouldUploadToGPU)
{
	// logv_error("DoTheFinalGPUUpload(glTarget: 0x%x, level: %u, pixelData: %p, textureFormatToSwitch: 0x%x, width: %u, height: %u, imageSize: %u, shouldUploadToGPU: %d)\n",
	// 	glTarget, level, pixelData, textureFormatToSwitch, width, height, imageSize, shouldUploadToGPU);
	SO_CONTINUE(void *, DoTheFinalGPUUpload_hook, glTarget, level, pixelData,
	textureFormatToSwitch, width, height, imageSize, shouldUploadToGPU);	
}

void so_patch(void) {	
	//sceSysmoduleLoadModule(SCE_SYSMODULE_PERF);

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
	WriteCommand_hook = hook_addr(LOC(0x001dc604), (uintptr_t)&WriteCommand_Optimized);
	logv_error("WriteCommand hooked at address 0x001dc604 -> %p\n", &WriteCommand_Optimized);
	
	D3DDevice_SetVertexShaderConstantNotInline_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_SetVertexShaderConstantNotInline");
	D3DDevice_SetVertexShaderConstantFast_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_SetVertexShaderConstantFast");
	D3DDevice_SetVertexShaderConstantNotInline_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "D3DDevice_SetVertexShaderConstantNotInline"), (uintptr_t)&D3DDevice_SetVertexShaderConstantNotInline_patched);

	// _Z11coreAddTaskPFvvEiPKc coreAddTask
	coreAddTask_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z11coreAddTaskPFvvEiPKc"), (uintptr_t)&coreAddTask);
	renderDelayedShadows_hook = hook_addr(LOC(0x0013d578), (uintptr_t)&renderDelayedShadows);
	runObjects_hook = hook_addr(LOC(0x00114a1c), (uintptr_t)&runObjects);
	drawObjects_hook = hook_addr(LOC(0x00115094), (uintptr_t)&drawObjects);
	XGGetPixelBufferMinAlpha_hook = hook_addr(LOC(0x00209d0c), (uintptr_t)&XGGetPixelBufferMinAlpha);
	XGGetPixelBufferMaxAlpha_hook = hook_addr(LOC(0x00209ff0), (uintptr_t)&XGGetPixelBufferMaxAlpha);
	ProcessAndUploadTexture_hook = hook_addr(LOC(0x0021225c), (uintptr_t)&ProcessAndUploadTexture);
	DoTheFinalGPUUpload_hook = hook_addr(LOC(0x002160ec), (uintptr_t)&DoTheFinalGPUUpload);

	//uint32_t loc = LOC(0x00132474);
	//logv_error("COPY TEXTURE at %p\n", loc);
	//texture_copy_hook = hook_addr(loc, (uintptr_t)&texture_copy);

	//_Z25lowestPowerof2NotLessThani
	uintptr_t lowestPowerof2NotLessThan_addr = (uintptr_t)so_symbol(&so_mod, "_Z25lowestPowerof2NotLessThani");
	if (lowestPowerof2NotLessThan_addr == 0) {
		log_error("lowestPowerof2NotLessThan not found\n");
	} else {
		logv_error("lowestPowerof2NotLessThan found at %p\n", lowestPowerof2NotLessThan_addr);
		lowestPowerof2NotLessThan_hook = hook_addr(lowestPowerof2NotLessThan_addr, (uintptr_t)&lowestPowerof2NotLessThan);
	}

	// _ZN14D3DBaseTexture11BufferToOGLEP21RegisteredTextureDataPKvi
	uintptr_t D3DBaseTexture_BufferToOGL_addr = (uintptr_t)so_symbol(&so_mod, "_ZN14D3DBaseTexture11BufferToOGLEP21RegisteredTextureDataPKvi");
	if (D3DBaseTexture_BufferToOGL_addr == 0) {
		log_error("D3DBaseTexture_BufferToOGL not found\n");
	} else {
		logv_error("D3DBaseTexture_BufferToOGL found at %p\n", D3DBaseTexture_BufferToOGL_addr);
		//D3DBaseTexture_BufferToOGL_hook = hook_addr(D3DBaseTexture_BufferToOGL_addr, (uintptr_t)&D3DBaseTexture_BufferToOGL);
	}
	
	

	// _ZN3JBE9D3DDevice8GetFVFVSEPNS0_24FVFVertexShaderContainerERm
	uintptr_t D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice8GetFVFVSEPNS0_24FVFVertexShaderContainerERm");
	if (D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm_addr == 0) {
		log_error("D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm not found\n");
	} else {
		logv_error("D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm found at %p\n", D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm_addr);
		//D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm_hook = hook_addr(D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm_addr, (uintptr_t)&D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm);
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
	uintptr_t D3DTexture_LockRect_addr = (uintptr_t)so_symbol(&so_mod, "D3DTexture_LockRect");
	if (D3DTexture_LockRect_addr == 0) {
		log_error("D3DTexture_LockRect not found\n");
	} else {
		logv_error("D3DTexture_LockRect found at %p\n", D3DTexture_LockRect_addr);
		D3DTexture_LockRect_hook = hook_addr(D3DTexture_LockRect_addr, (uintptr_t)&D3DTexture_LockRect);
	}

	//D3DDevice_CreateTexture2
	uintptr_t D3DDevice_CreateTexture2_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_CreateTexture2");
	if (D3DDevice_CreateTexture2_addr == 0) {
		log_error("D3DDevice_CreateTexture2 not found\n");
	} else {
		logv_error("D3DDevice_CreateTexture2 found at %p\n", D3DDevice_CreateTexture2_addr);
		D3DDevice_CreateTexture2_hook = hook_addr(D3DDevice_CreateTexture2_addr, (uintptr_t)&D3DDevice_CreateTexture2);
	}

	//_ZN3JBE9D3DDevice16SetTextureStagesEm
	uintptr_t D3DDevice_SetTextureStages_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice16SetTextureStagesEm");
	if (D3DDevice_SetTextureStages_addr == 0) {
		log_error("D3DDevice_SetTextureStages not found\n");
	} else {
		logv_error("D3DDevice_SetTextureStages found at %p\n", D3DDevice_SetTextureStages_addr);
		D3DDevice_SetTextureStages_hook = hook_addr(D3DDevice_SetTextureStages_addr, (uintptr_t)&D3DDevice_SetTextureStages);
	}

	// _ZN3JBE5Input6RenderEv
	uintptr_t inputRender_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE5Input6RenderEv");
	if (inputRender_addr == 0) {
		log_error("inputRender not found\n");
	} else {
		logv_error("inputRender found at %p\n", inputRender_addr);
		//inputRender_hook = hook_addr(inputRender_addr, (uintptr_t)&inputRender);
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

	//_ZN3JBE6System10BeginFrameEv
	uintptr_t System_BeginFrame_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE6System10BeginFrameEv");
	if (System_BeginFrame_addr == 0) {
		log_error("System_BeginFrame not found\n");
	} else {
		logv_error("System_BeginFrame found at %p\n", System_BeginFrame_addr);
		//System_BeginFrame_hook = hook_addr(System_BeginFrame_addr, (uintptr_t)&System_BeginFrame);
	}

	// // _Z8gameLoopv
	// uintptr_t gameLoop_addr = (uintptr_t)so_symbol(&so_mod, "_Z8gameLoopv");
	// if (gameLoop_addr == 0) {
	// 	log_error("gameLoop not found\n");
	// } else {
	// 	logv_error("gameLoop found at %p\n", gameLoop_addr);
	// 	gameLoop_hook = hook_addr(gameLoop_addr, (uintptr_t)&gameLoop);
	// }

	// _Z14machFrameStartv
	uintptr_t machFrameStart_addr = (uintptr_t)so_symbol(&so_mod, "_Z14machFrameStartv");
	if (machFrameStart_addr == 0) {
		log_error("machFrameStart not found\n");
	} else {
		logv_error("machFrameStart found at %p\n", machFrameStart_addr);
		machFrameStart_hook = hook_addr(machFrameStart_addr, (uintptr_t)&machFrameStart);
	}

	// _Z12machFrameEndi
	uintptr_t machFrameEnd_addr = (uintptr_t)so_symbol(&so_mod, "_Z12machFrameEndi");
	if (machFrameEnd_addr == 0) {
		log_error("machFrameEnd not found\n");
	} else {
		logv_error("machFrameEnd found at %p\n", machFrameEnd_addr);
		mach_frameEnd_hook = hook_addr(machFrameEnd_addr, (uintptr_t)&machFrameEnd);
	}

	//_ZN3JBE9D3DDevice4SwapEm
	uintptr_t JBE_D3DDevice_Swap_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice4SwapEm");
	if (JBE_D3DDevice_Swap_addr == 0) {
		log_error("JBE_D3DDevice_Swap not found\n");
	} else {
		logv_error("JBE_D3DDevice_Swap found at %p\n", JBE_D3DDevice_Swap_addr);
		JBE_D3DDevice_Swap_hook = hook_addr(JBE_D3DDevice_Swap_addr, (uintptr_t)&JBE_D3DDevice_Swap);
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

	//_ZN3JBE9D3DDevice11ReadCommandEv
	D3DDevice_ReadCommand_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice11ReadCommandEv");
	if (D3DDevice_ReadCommand_addr == 0) {
		log_error("D3DDevice_ReadCommand not found\n");
	} else {
		logv_error("D3DDevice_ReadCommand found at %p\n", D3DDevice_ReadCommand_addr);
	//	D3DDevice_ReadCommand_hook = hook_addr(D3DDevice_ReadCommand_addr, (uintptr_t)&D3DDevice_ReadCommand);
	}

	// _ZN3JBE9D3DDevice22RegisterTextureCommandER14D3DBaseTextureRiS3_S3_
	uintptr_t D3DDevice_RegisterTextureCommand_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice22RegisterTextureCommandER14D3DBaseTextureRiS3_S3_");
	if (D3DDevice_RegisterTextureCommand_addr == 0) {
		log_error("D3DDevice_RegisterTextureCommand not found\n");
	} else {
		logv_error("D3DDevice_RegisterTextureCommand found at %p\n", D3DDevice_RegisterTextureCommand_addr);
		//D3DDevice_RegisterTextureCommand_hook = hook_addr(D3DDevice_RegisterTextureCommand_addr, (uintptr_t)&D3DDevice_RegisterTextureCommand);
	}


	// _ZN3JBE8SystemPF7HasNEONEv
	uintptr_t System_HasNEON_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE8SystemPF7HasNEONEv");
	if (System_HasNEON_addr == 0) {
		log_error("System_HasNEON not found\n");
	} else {
		logv_error("System_HasNEON found at %p\n", System_HasNEON_addr);
		System_HasNEON_hook = hook_addr(System_HasNEON_addr, (uintptr_t)&System_HasNEON);
	}

	// D3DDevice_Swap
	uintptr_t D3DDevice_Swap_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_Swap");
	if (D3DDevice_Swap_addr == 0) {
		log_error("D3DDevice_Swap not found\n");
	} else {
		logv_error("D3DDevice_Swap found at %p\n", D3DDevice_Swap_addr);
		D3DDevice_Swap_hook = hook_addr(D3DDevice_Swap_addr, (uintptr_t)&D3DDevice_Swap);
	}
	
	//_ZN3JBE9DisplayPF4SwapEv
	uintptr_t DisplayPF_Swap_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9DisplayPF4SwapEv");
	if (DisplayPF_Swap_addr == 0) {
		log_error("DisplayPF_Swap not found\n");
	} else {
		logv_error("DisplayPF_Swap found at %p\n", DisplayPF_Swap_addr);
		DisplayPF_Swap_hook = hook_addr(DisplayPF_Swap_addr, (uintptr_t)&DisplayPF_Swap);
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

	//MEMAllocFromExpHeapEx
	uintptr_t MEMAllocFromExpHeapEx_addr = (uintptr_t)so_symbol(&so_mod, "MEMAllocFromExpHeapEx");
	if (MEMAllocFromExpHeapEx_addr == 0) {
		log_error("MEMAllocFromExpHeapEx not found\n");
	} else {
		logv_error("MEMAllocFromExpHeapEx found at %p\n", MEMAllocFromExpHeapEx_addr);
		MEMAllocFromExpHeapEx_hook = hook_addr(MEMAllocFromExpHeapEx_addr, (uintptr_t)&MEMAllocFromExpHeapEx);
	}

	#if PROFILER_ENABLED
	install_prof_hooks();
	#endif

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
		
		// print original string
		//logv_error("Original string at %p: %s\n", addressToPatch, (char *)addressToPatch);
		kuKernelCpuUnrestrictedMemcpy((void *)addressToPatch, stringToPatch, strlen(stringToPatch)+1);
		// print new string
		//logv_error("Patched string at %p: %s\n", addressToPatch, (char *)addressToPatch);
	}
}
