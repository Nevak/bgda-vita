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
#include <vitagprof.h>

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

// #define TIME_HOOK(ret, name, ...)                                  \
//     static so_hook name##_hook;                                    \
//     static ret name##_prof(__VA_ARGS__)                            \
//     {                                                              \
//         Profiler_BeginSample("Test");                              \
//         ret _res = SO_CONTINUE(ret, name##_hook, ##__VA_ARGS__);   \
// 		Profiler_EndSample();                                      \	
//         return _res;                                               \
//     }

// TIME_HOOK(void, machFrameEnd, int p);

// typedef void (*TaskFn)(void);   /* adapt if the real prototype differs */

// struct TaskThunk {
//     TaskFn  orig;      /* the real callback                     */
//     const char *name;  /* pointer to the label in param_3       */
// };

// __attribute__((naked)) static void task_thunk_entry(void)
// {
//     __asm__ volatile (
//         "push   {r0-r3, lr}           \n"   /* save ABI-scratch regs        */
//         "ldr    r0, [sp, #20]         \n"   /* r0 = ptr to TaskThunk ctx    */
//         "ldr    r1, [r0, #4]          \n"   /* r1 = ctx->name               */
//         "bl     Profiler_BeginSample  \n"

//         "ldr    r0, [sp, #20]         \n"   /* ctx again                    */
//         "ldr    r0, [r0, #0]          \n"   /* r0 = ctx->orig               */
//         "blx    r0                    \n"   /* call real task               */

//         "ldr    r0, [sp, #20]         \n"
//         "ldr    r1, [r0, #4]          \n"
//         "bl     Profiler_EndSample    \n"

//         "pop    {r0-r3, lr}           \n"
//         "bx     lr                    \n"
//     );
// }


static const char *gCmdLabels[256] = {
    [0x00] = "0x00 SetRenderTarget",
    [0x01] = "0x01 RegisterTexture",
    [0x02] = "0x02 BufferTexture",
    [0x03] = "0x03 GenerateMipmaps",
    [0x04] = "0x04 ResolveTexture",
    [0x05] = "0x05 UnregisterTexture",
    [0x06] = "0x06 RegisterSurface",
    [0x07] = "0x07 UnregisterSurface",
    [0x08] = "0x08 CompileShader",
    [0x09] = "0x09 ThreadSignal",
    [0x0A] = "0x0A IncrementFieldCounter",
    [0x0B] = "0x0B Clear",
    [0x0C] = "0x0C Swap",
    [0x0D] = "0x0D SwapToFront",
    [0x0E] = "0x0E BeginPrimitive",
    [0x0F] = "0x0F EndPrimitive",
    [0x10] = "0x10 SetVertexData4f",
    [0x11] = "0x11 SetVertexAttribute",
    [0x12] = "0x12 SetConstantMaybe",
    [0x13] = "0x13 UpdateShaderConstant",
    [0x14] = "0x14 BindResource",
    [0x15] = "0x15 SetRenderState",
    [0x16] = "0x16 SetVertexShaderInput",
    [0x17] = "0x17 StoreShaderState",
    [0x18] = "0x18 SetVertexShader",
    [0x19] = "0x19 CopyToSemaphore",
    [0x1A] = "0x1A SetVertexShaderAgain",
    [0x1B] = "0x1B SetPixelShaderProgram",
    [0x1C] = "0x1C SetPixelShaderConstant",
    [0x1D] = "0x1D LinkPixelShaderProgram",
    [0x1E] = "0x1E SetScissors",
    [0x1F] = "0x1F SetDepthRange",
    [0x20] = "0x20 RunDynamicPushBuffer",
    [0x21] = "0x21 DrawIndexedVertices", // 33
    [0x22] = "0x22 DrawVertices",
    [0x23] = "0x23 DrawVerticesUP",
    [0x24] = "0x24 DrawIndexedVerticesInstanced", // 36
    [0x25] = "0x25 DrawVerticesInstanced",
    [0x26] = "0x26 StoreVertexData",
    [0x27] = "0x27 StoreShaderParameters",
    [0x28] = "0x28 RegisterIndexBuffer",
    [0x29] = "0x29 BufferIndexBuffer",
    [0x2A] = "0x2A UnregisterIndexBuffer",
    [0x2B] = "0x2B RegisterVertexBuffer",
    [0x2C] = "0x2C BufferVertexBuffer",
    [0x2D] = "0x2D UnregisterVertexBuffer",
    [0x2E] = "0x2E BeginVisibilityTest",
    [0x2F] = "0x2F EndVisibilityTest",
    [0x30] = "0x30 GetVisibilityTestResult",
    [0x31] = "0x31 CallFunctionPointer",
    [0x32] = "0x32 UpdateLightingColor",
    [0x33] = "0x33 UpdatePalette",
    [0x34] = "0x34 DrawExtendedParams",
    [0x35] = "0x35 DrawExtendedParamsIndexed",
    [0x36] = "0x36 CopyMemoryBlockA",
    [0x37] = "0x37 CopyMemoryBlockB",
    [0x38] = "0x38 DrawExtendedParamsC",
    [0x39] = "0x39 SetViewport",
    [0x3A] = "0x3A StoreViewportState",
    [0x3B] = "0x3B UpdateViewportState",
    [0x3C] = "0x3C StoreViewportStateIndexed",
    [0x3D] = "0x3D ComputeAspectRatio",
    [0x3E] = "0x3E ResetRenderFlag",
    [0xFF] = "0xFF SpecialCommandOffset"
};


// static TaskFn make_standalone_stub(TaskFn orig, const char *label)
// {
//     /* 1. allocate RW-X memory for ctx + 2 instructions (12 bytes) */
//     size_t  sz   = sizeof(struct TaskThunk) + 12;
//     uint8_t *mem = sceClibMemalign(4, sz);            /* Vita SDK’s aligned malloc */
//    // sceKernelDClearWritebackDCache(mem, sz);          /* ensure coherency         */

//     struct TaskThunk *ctx = (struct TaskThunk*)mem;
//     ctx->orig  = orig;
//     ctx->name  = label;

//     /* 2. patch the two ARM instructions that jump to task_thunk_entry */
//     uint32_t  *code = (uint32_t*)(ctx + 1);

//     /* ldr r0, =ctx          (literal 4 bytes after the two instructions) */
//     code[0] = 0x4801;                /* Thumb:  LDR r0, [PC, #4] */
//     /* bx  task_thunk_entry */
//     code[1] = 0x4700 | (((uintptr_t)task_thunk_entry & 0xFFFFFFFE) >> 1);

//     /* literal pool entry   */
//     code[2] = (uint32_t)ctx;

//     /* Flush so the CPU sees the new instructions */
//     //sceKernelDcacheWritebackInvalidateRange(code, 12);
//     //sceKernelIcacheInvalidateRange(code, 12);

//     /* Return pointer to the first instruction (Thumb bit set) */
//     return (TaskFn)((uintptr_t)code | 1);
// }

/*
coreAddTask(0x984acaf4, 19, StatCache)←[0m
coreAddTask(0x984d0a4c, 20, Async Load/Save Daemon)←[0m
coreAddTask(0x985485a0, 110, Dialog Demon)←[0m
coreAddTask(0x984c7e78, 125, Script Demon)←[0m
coreAddTask(0x98504a1c, 10, runObjects)←[0m
coreAddTask(0x98505094, 21, drawObjects)←[0m
coreAddTask(0x9857b2a0, 28, drawFloorSprites (must be before drawworld))←[0m
coreAddTask(0x98593eb8, 15, updateParticles)←[0m
coreAddTask(0x985923ec, 65, drawParticles)←[0m
coreAddTask(0x985a8c98, 3, transformSceneLight)←[0m
coreAddTask(0x9852d578, 22, RenderDelayedShadows)←[0m
coreAddTask(0x9852e50c, 1, camera)←[0m
coreAddTask(0x9852edb8, 20, drawWorld)←[0m
coreAddTask(0x985277a0, 23, delayDrawWorld)←[0m
coreAddTask(0x98564d48, 50, HelpMessage)←[0m
coreAddTask(0x9852e50c, 1, camera)←[0m
coreAddTask(0x9852edb8, 20, drawWorld)←[0m
coreAddTask(0x985277a0, 23, delayDrawWorld)←[0m
coreAddTask(0x98564d48, 50, HelpMessage)←[0m
coreAddTask(0x9852e50c, 1, camera)←[0m
coreAddTask(0x9852edb8, 20, drawWorld)←[0m
coreAddTask(0x985277a0, 23, delayDrawWorld)←[0m
coreAddTask(0x98564d48, 50, HelpMessage)←[0m
coreAddTask(0x98563a90, 50, HUD)←[0m
*/


so_hook coreAddTask_hook;
int coreAddTask(void *fn, int prio, char *name) {
	logv_error("coreAddTask(%p, %i, %s)", fn, prio, name);
	// ignore if task is StatCache
	if (name && strcmp(name, "StatCache") == 0) {
		//logv_error("Ignoring StatCache task\n");
		return 0;
	}
	
	// if (name && strcmp(name, "drawObjects") == 0) {
	// 	log_error("Ignoring drawObjects task\n");
	// 	return 0;
	// }


	if (name && strcmp(name, "RenderDelayedShadows") == 0) {
	   	log_error("Ignoring renderDelayedShadows task\n");
	   	return 0;
	}

	// 	if (name && strcmp(name, "delayDrawWorld") == 0) {
	//   	log_error("Ignoring delayDrawWorld task\n");
	//   	return 0;
	// }
		

	// int returnval = SO_CONTINUE(int, coreAddTask_hook, param_1, param_2, param_3);
	// //logv_error("coreAddTask returned %i\n", returnval);
	// return returnval;

	    /* Turn the user-supplied callback into a profiled stub */
   // TaskFn wrapped = make_standalone_stub((TaskFn)fn, name);

    /* Pass *our* stub to the original scheduler */
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
void frontEndDoControllerScreenInput(int *param_1, int *param_2) {

}

so_hook inputRender_hook;
void inputRender(void *thisptr) {	
}

so_hook virtualControlsRender_hook;
void virtualControlsRender(void *thisptr) {
	//logv_error("virtualControlsRender()\n");
	//SO_CONTINUE(void *, virtualControlsRender_hook);
}

so_hook writeConfigDirect_hook;
void writeConfigDirect() {
	//log_error("writeConfigDirect()\n");
	//SO_CONTINUE(void *, writeConfigDirect_hook);
}



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


// void D3DDevice_SetTexture(ulong param_1,int param_2)
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
void D3DDevice_SetVertexShaderConstantNotInline(int reg, uint32_t pConstantData, uint32_t ConstantCount) {
	Profiler_BeginSample("D3DDevice_SetVertexShaderConstantNotInline");
	//SO_CONTINUE(float, D3DDevice_SetVertexShaderConstantNotInline_hook, reg, pConstantData, ConstantCount);
	D3DDevice_SetVertexShaderConstantFastFn D3DDevice_SetVertexShaderConstantFast = (D3DDevice_SetVertexShaderConstantFastFn)D3DDevice_SetVertexShaderConstantFast_addr;
	D3DDevice_SetVertexShaderConstantFast(reg, pConstantData, ConstantCount);
	Profiler_EndSample();
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
			//logv_error("D3DDevice_SetTexture: potWidth: %i, potHeight: %i\n", potWidth, potHeight);
			//logv_error("D3DDevice_SetTexture: scaleX: %f, scaleY: %f\n", scaleX, scaleY);
			//logv_error("D3DDevice_SetTexture: g_uvFactor: %f, g_uvFactorY: %f\n", g_uvFactor, g_uvFactorY);
			float scale[4] = {g_uvFactor * scaleX, g_uvFactorY * scaleY, 0.0f, 0.0f};
			// if (g_uvFactor > 0.0f) {
			// 	scale[0] = 1.0f;
			// 	scale[1] = 1.0f;
			// }
			//logv_error("D3DDevice_SetTexture: scaleX: %f, scaleY: %f\n", g_uvFactor,  g_uvFactor *);
			//D3DDevice_SetVertexShaderConstantNotInlineFn D3DDevice_SetVertexShaderConstantNotInline = (D3DDevice_SetVertexShaderConstantNotInlineFn)D3DDevice_SetVertexShaderConstantNotInline_addr;
			//D3DDevice_SetVertexShaderConstantNotInline(24, (uint32_t)scale, 1);
			SO_CONTINUE(float, D3DDevice_SetVertexShaderConstantNotInline_hook, 24, (uint32_t)scale, 1);
		}
		else {
			float scale[4] = {scaleX, scaleY, 0.0f, 0.0f};
			//float scale[4] = {g_uvFactor * scaleX, g_uvFactorY * scaleY, 0.0f, 0.0f};
			//D3DDevice_SetVertexShaderConstantNotInlineFn D3DDevice_SetVertexShaderConstantNotInline = (D3DDevice_SetVertexShaderConstantNotInlineFn)D3DDevice_SetVertexShaderConstantNotInline_addr;
			//D3DDevice_SetVertexShaderConstantNotInline(24, (uint32_t)scale, 1);
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


void __aeabi_memcpy_patched(void *dst, const void *src, int n) {
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





// PROF_HOOK_VOID(objectDrawDelayedDrawObjects, "_Z28objectDrawDelayedDrawObjectsv", (void))
// PROF_HOOK_VOID(animFrame, "_Z9animFramev", (void))
// PROF_HOOK_VOID(texProcessLoad, "_Z14texProcessLoadv", (void))
// PROF_HOOK_VOID(texProcessDecompress, "_Z20texProcessDecompressv", (void))
// PROF_HOOK_VOID(machBlankScreen, "_Z15machBlankScreen", (void))
// PROF_HOOK_VOID(padProcess, "_Z10padProcessv", (void))
// PROF_HOOK_VOID(lightVU0StoppedProcessing, "_Z25lightVU0StoppedProcessingv", (void))
// PROF_HOOK_VOID(gameCameraTask, "_Z14gameCameraTaskv", (void))
PROF_HOOK_VOID(gameDrawWorld, "_Z13gameDrawWorldv", (void))
PROF_HOOK_VOID(worldDoDelayDrawTask, "_Z20worldDoDelayDrawTaskv", (void))
// PROF_HOOK_VOID(drawObjects, "_Z12drawObjectsv", (void))
// PROF_HOOK_VOID(runObjects, "_Z10runObjectsv", (void))
// PROF_HOOK_VOID(floorDraw, "_Z9floorDrawv", (void))
// PROF_HOOK_VOID(snd_frame, "_Z9SND_Framev", (void))
// PROF_HOOK_VOID(cdProcess, "_Z9cdProcessi", (int param_1), param_1)
// PROF_HOOK_VOID(worldPlotRouteProcess, "_Z21worldPlotRouteProcessi", (int param_1), (param_1))
// PROF_HOOK_VOID(updateParticles, "_Z8P_Updatev", (void))
// //PROF_HOOK_VOID(D3DDevice_ReadCommand, "_ZN3JBE9D3DDevice11ReadCommandEv", (int* param_1), (param_1));
// PROF_HOOK_VOID(DrawVertices, "_ZN3JBE9D3DDevice12DrawVerticesE17_D3DPRIMITIVETYPEmm", (int *param_1, int *param_2, int *param_3, int *param_4), param_1, param_2, param_3, param_4);
// PROF_HOOK_VOID(DrawVerticesIndexed, "_ZN3JBE9D3DDevice19DrawIndexedVerticesE17_D3DPRIMITIVETYPEjPKt", (int *thisptr, int param_1, int param_2, ushort *param_3), thisptr, param_1, param_2, param_3);
// //PROF_HOOK_VOID(JBE_D3DDevice_SetRenderState, "_ZN3JBE9D3DDevice14SetRenderStateE19_D3DRENDERSTATETYPEm", (int *thisptr, int param_2, uint32_t param_3), (thisptr, param_2, param_3))
//_ZN3JBE9D3DDevice22RegisterTextureCommandER14D3DBaseTextureRiS3_S3_ JBE::D3DDevice::RegisterTextureCommand
// PROF_HOOK_VOID(D3DDevice_RegisterTextureCommand, "_ZN3JBE9D3DDevice22RegisterTextureCommandER14D3DBaseTextureRiS3_S3_", 
// 			   (void *pThis, int *param_2, int *param_3, int *param_4), pThis, param_2, param_3, param_4);
//PROF_HOOK_VOID(machFrameEnd, "_Z12machFrameEndi", (int p), (p))
PROF_HOOK_VOID(EndFrameFence, "_ZN3JBE9DisplayPF13EndFrameFenceEv", (int* param_1), (param_1));
PROF_HOOK_VOID(SystemUpdate, "_ZN3JBE6System6UpdateEv", (void));
PROF_HOOK_VOID(SwapToFront, "_ZN3JBE9D3DDevice11SwapToFrontEi", (int* param_1), (param_1));
PROF_HOOK_VOID(DisplaySwap, "_ZN3JBE9DisplayPF4SwapEv", (void *param_1), (param_1));
//_ZN3JBE9D3DDevice11CommitStateEv
PROF_HOOK_VOID(D3DDevice_CommitState, "_ZN3JBE9D3DDevice11CommitStateEv", (void *param_1), (param_1));
PROF_HOOK_VOID(Blit, "_ZN3JBE9DisplayPF4BlitEiiiiRKNS_13ShaderProgramEi", (int* param_1, int param_2, int param_3, int param_4, void *param_5, int param_6), (param_1, param_2, param_3, param_4, param_5, param_6));
//_ZN3JBE9D3DDevice16SetTextureStagesEm
//PROF_HOOK_VOID(D3DDevice_SetTextureStages, "_ZN3JBE9D3DDevice16SetTextureStagesEm", (uint8_t *param_1, uint32_t param_2), (param_1, param_2));
//_ZN3JBE9D3DDevice17UpdateComboStatesEv
 PROF_HOOK_VOID(D3DDevice_UpdateComboStates, "_ZN3JBE9D3DDevice17UpdateComboStatesEv", (int *param_1), (param_1));
// _ZN3JBE9D3DDevice26SetVertexShaderInputDirectE
PROF_HOOK_VOID(D3DDevice_SetVertexShaderInputDirect, "_ZN3JBE9D3DDevice26SetVertexShaderInputDirectE", (int *thisptr, int param_2, int param_3, int param_4), (thisptr, param_2, param_3, param_4));
//PROF_HOOK_VOID(D3DDevice_SetVertexShaderConstantNotInlineX, "D3DDevice_SetVertexShaderConstantNotInline", (int* thisptr, int reg, uint32_t pConstantData, uint64_t ConstantCount), (reg, pConstantData, ConstantCount));
// PROF_HOOK_RET(uint32_t, WorldClipCubeToFrustum, "_Z22worldClipCubeToFrustumPA2_fi", (float *param_1, int param_2), (param_1, param_2));
// PROF_HOOK_RET(uint32_t, worldClipCubeToClipFrustum, "_Z26worldClipCubeToClipFrustumPA2_fi", (float *param_1, int param_2), (param_1, param_2));


// _ZN3JBE9D3DDevice8GetFVFVSEPNS0_24FVFVertexShaderContainerERm
// PROF_HOOK_RET(int, D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm, "_ZN3JBE9D3DDevice8GetFVFVSEPNS0_24FVFVertexShaderContainerERm", (void *param_1, int *param_2), (param_1, param_2));

void install_prof_hooks(void) {
	//PROF_ATTACH(gameLoop, "_Z8gameLoopv");
	//PROF_ATTACH(machFrameEnd, "_Z12machFrameEndi");
	// PROF_ATTACH(objectDrawDelayedDrawObjects, "_Z28objectDrawDelayedDrawObjectsv");
	// PROF_ATTACH(animFrame, "_Z9animFramev");
	// PROF_ATTACH(texProcessLoad, "_Z14texProcessLoadv");
	// PROF_ATTACH(texProcessDecompress, "_Z20texProcessDecompressv");
	// PROF_ATTACH(machBlankScreen, "_Z15machBlankScreen");
	// PROF_ATTACH(padProcess, "_Z10padProcessv");
	// PROF_ATTACH(lightVU0StoppedProcessing, "_Z25lightVU0StoppedProcessingv");
	// PROF_ATTACH(gameCameraTask, "_Z14gameCameraTaskv");
	PROF_ATTACH(gameDrawWorld, "_Z13gameDrawWorldv");
	PROF_ATTACH(worldDoDelayDrawTask, "_Z20worldDoDelayDrawTaskv");
	// PROF_ATTACH(drawObjects, "_Z12drawObjectsv");
	// PROF_ATTACH(runObjects, "_Z10runObjectsv");
	// PROF_ATTACH(floorDraw, "_Z9floorDrawv");
	// PROF_ATTACH(snd_frame, "_Z9SND_Framev");
	// PROF_ATTACH(cdProcess, "_Z9cdProcessi");
	// PROF_ATTACH(worldPlotRouteProcess, "_Z21worldPlotRouteProcessi");
	// PROF_ATTACH(updateParticles, "_Z8P_Updatev");
	// //PROF_ATTACH(D3DDevice_ReadCommand, "_ZN3JBE9D3DDevice11ReadCommandEv");
	// PROF_ATTACH(DrawVertices, "_ZN3JBE9D3DDevice12DrawVerticesE17_D3DPRIMITIVETYPEmm");
	// PROF_ATTACH(DrawVerticesIndexed, "_ZN3JBE9D3DDevice19DrawIndexedVerticesE17_D3DPRIMITIVETYPEjPKt");
	// //PROF_ATTACH(JBE_D3DDevice_SetRenderState, "_ZN3JBE9D3DDevice14SetRenderStateE19_D3DRENDERSTATETYPEm");
	//PROF_ATTACH(D3DDevice_RegisterTextureCommand, "_ZN3JBE9D3DDevice22RegisterTextureCommandER14D3DBaseTextureRiS3_S3_");
	PROF_ATTACH(EndFrameFence, "_ZN3JBE9DisplayPF13EndFrameFenceEv");
	PROF_ATTACH(SystemUpdate, "_ZN3JBE6System6UpdateEv");
	PROF_ATTACH(SwapToFront, "_ZN3JBE9D3DDevice11SwapToFrontEi");
	PROF_ATTACH(DisplaySwap, "_ZN3JBE9DisplayPF4SwapEv");
	PROF_ATTACH(Blit, "_ZN3JBE9DisplayPF4BlitEiiiiRKNS_13ShaderProgramEi");
	PROF_ATTACH(D3DDevice_CommitState, "_ZN3JBE9D3DDevice11CommitStateEv");
	// _ZN3JBE9D3DDevice26SetVertexShaderInputDirectE
	//PROF_ATTACH(D3DDevice_SetVertexShaderInputDirect, "_ZN3JBE9D3DDevice26SetVertexShaderInputDirectE");
//	 PROF_ATTACH(D3DDevice_SetTextureStages, "_ZN3JBE9D3DDevice16SetTextureStagesEm");
	//PROF_ATTACH(D3DDevice_UpdateComboStates, "_ZN3JBE9D3DDevice17UpdateComboStatesEv");
	//PROF_ATTACH(D3DDevice_SetVertexShaderConstantNotInlineX, "D3DDevice_SetVertexShaderConstantNotInline");
	// PROF_ATTACH(WorldClipCubeToFrustum, "_Z22worldClipCubeToFrustumPA2_fi");
	// PROF_ATTACH(worldClipCubeToClipFrustum, "_Z26worldClipCubeToClipFrustumPA2_fi");
}

so_hook machFrameStart_hook;

uint32_t frameStartCallCount = 0;
uint32_t frameStartCalledAtTime;
uint32_t frameStartToEndTime = 0;
		
void machFrameStart(int p) {
	sceKernelChangeThreadCpuAffinityMask(sceKernelGetThreadId(), SCE_KERNEL_CPU_MASK_USER_0);

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
	//logv_error("DisplayPF_Swap(%p)\n", param_1);
	//Profiler_BeginSample("DisplayPF::Swap");
	SO_CONTINUE(void *, DisplayPF_Swap_hook, param_1);
	//Profiler_EndSample();
	//log_error("DisplayPF_Swap finished\n");
	
	// only print every 100 frames
	static int frameCount = 0;
	frameCount++;
	if (frameCount == 1) {
		lastFrameTime = sceKernelGetProcessTimeWide();
	}
	bool shouldLog = (frameCount % 100 == 0);
	if (log_profiler && shouldLog) {
		//sceClibPrintf("Saving profiling output\n");
		//char fname[256];
		//sprintf(fname, "ux0:data/prof_%d.out", profiling_idx++);
		//gprof_stop(fname, 1);

		// Continue the profiler
		//gprof_start();

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
	sceKernelChangeThreadCpuAffinityMask(sceKernelGetThreadId(), SCE_KERNEL_CPU_MASK_USER_1);
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
	
			Profiler_BeginSample(label);
			fnReadCommand(device);
			Profiler_EndSample();
		}
		else {
			// Just read the command without profiling
			ReadCommandCustom(thisPtr);
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


//static const char *gCmdLabels[256];

// __attribute__((constructor))
// static void init_cmd_labels(void)
// {
//     static char buf[256][13];   /* "cmdType_0xFF\0" = 12 chars + NUL */
//     for (int i = 0; i < 256; ++i) {
//         snprintf(buf[i], sizeof buf[i], "cmdType_0x%02X", i);
//         gCmdLabels[i] = buf[i];
//     }
// }

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

so_hook D3DDevice_Swap_hook;


uint64_t lastFrameTimeMainThread = 0;
void D3DDevice_Swap(int flags) {
	Profiler_BeginSample("D3DDevice_Swap (main thread)");
	SO_CONTINUE(void *, D3DDevice_Swap_hook, flags);
	Profiler_EndSample();

	// if (log_profiler) {
	// 	// uint64_t timeNow = sceKernelGetProcessTimeWide();
	// 	// uint64_t frameTime = timeNow - lastFrameTimeMainThread;
	// 	// logv_error("[t:%d]D3DDevice_Swap delta %f ms (%d) thread ID: 0x%X, flags: %x\n",
	// 	// 	timeNow, (float)frameTime / 1000.0f, (int)frameTime, sceKernelGetThreadId(), flags);
	// 	// lastFrameTimeMainThread = timeNow;

	// 	Profiler_PrintAll();
	// 	Profiler_ResetAll();
	// }
}

so_hook renderDelayedShadows_hook;
void renderDelayedShadows(void) {
	//logv_error("renderDelayedShadows()\n");
	Profiler_BeginSample("renderDelayedShadows");
	SO_CONTINUE(void *, renderDelayedShadows_hook);
	Profiler_EndSample();
	//log_error("renderDelayedShadows finished\n");
}
so_hook runObjects_hook;
void runObjects(void) {
	//logv_error("runObjects()\n");
	Profiler_BeginSample("runObjects");
	SO_CONTINUE(void *, runObjects_hook);
	Profiler_EndSample();
	//log_error("runObjects finished\n");
}
so_hook drawObjects_hook;
void drawObjects(void) {
	//logv_error("drawObjects()\n");
	Profiler_BeginSample("drawObjects");
	SO_CONTINUE(void *, drawObjects_hook);
	Profiler_EndSample();
	//log_error("drawObjects finished\n");
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
    Profiler_BeginSample("worldClipCubeToFrustum");

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
                
                Profiler_EndSample();
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
    
    Profiler_EndSample();
    return clippedPlanes;
}


so_hook worldClipCubeToClipFrustum_hook;
uint32_t worldClipCubeToClipFrustum(float *cubeVertices, int clippedPlanes)
{	
	Profiler_BeginSample("worldClipCubeToClipFrustum");
	uint32_t x = SO_CONTINUE(uint32_t, worldClipCubeToClipFrustum_hook, cubeVertices, clippedPlanes);
	Profiler_EndSample();
	//log_error("worldClipCubeToClipFrustum finished\n");
	return x;
}

so_hook worldClipCubeToFrustumOnce_hook;
uint32_t worldClipCubeToFrustumOnce(float *cubeVertices, int clippedPlanes)
{
	Profiler_BeginSample("worldClipCubeToFrustumOnce");
	uint32_t x = SO_CONTINUE(uint32_t, worldClipCubeToFrustumOnce_hook, cubeVertices, clippedPlanes);
	Profiler_EndSample();
	return x;
}

so_hook D3DDevice_SetTextureStages_hook;
void D3DDevice_SetTextureStages(uint8_t *param_1, uint32_t param_2) {
	//logv_error("D3DDevice_SetTextureStages(%p, %u)\n", param_1, param_2);
	Profiler_BeginSample("D3DDevice_SetTextureStages");
	SO_CONTINUE(void *, D3DDevice_SetTextureStages_hook, param_1, param_2);
	Profiler_EndSample();
	//log_error("D3DDevice_SetTextureStages finished\n");
}
// _ZN3JBE9D3DDevice8GetFVFVSEPNS0_24FVFVertexShaderContainerERm
so_hook D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm_hook;
int D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm(void *thisptr, uintptr_t* container, uint32_t param_2) {
	//logv_error("D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm(%p, %p, %u)\n", thisptr, container, param_2);
	Profiler_BeginSample("D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm");
	int result = SO_CONTINUE(int, D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm_hook, thisptr, container, param_2);
	Profiler_EndSample();
	//logv_error("D3DDevice_GetFVFVSEPNS0_24FVFVertexShaderContainerERm finished with result %d\n", result);
	return result;
}

// _ZN14D3DBaseTexture11BufferToOGLEP21RegisteredTextureDataPKvi 
so_hook D3DBaseTexture_BufferToOGL_hook;
void D3DBaseTexture_BufferToOGL(void *pThis, void *pTexData, const void *pBuffer, int size) {
	//logv_error("D3DBaseTexture_BufferToOGL(%p, %p, %p, %d)\n", pThis, pTexData, pBuffer, size);
	Profiler_BeginSample("D3DBaseTexture_BufferToOGL");
	SO_CONTINUE(void *, D3DBaseTexture_BufferToOGL_hook, pThis, pTexData, pBuffer, size);
	Profiler_EndSample();
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



void so_patch(void) {	

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
	D3DDevice_SetVertexShaderConstantNotInline_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_SetVertexShaderConstantNotInline");
	D3DDevice_SetVertexShaderConstantFast_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_SetVertexShaderConstantFast");
	D3DDevice_SetVertexShaderConstantNotInline_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "D3DDevice_SetVertexShaderConstantNotInline"), (uintptr_t)&D3DDevice_SetVertexShaderConstantNotInline);

	// _Z11coreAddTaskPFvvEiPKc coreAddTask
	coreAddTask_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z11coreAddTaskPFvvEiPKc"), (uintptr_t)&coreAddTask);
	renderDelayedShadows_hook = hook_addr(LOC(0x0013d578), (uintptr_t)&renderDelayedShadows);
	runObjects_hook = hook_addr(LOC(0x00114a1c), (uintptr_t)&runObjects);
	drawObjects_hook = hook_addr(LOC(0x00115094), (uintptr_t)&drawObjects);

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

	// // _Z8memAllociPKc
	// uintptr_t memAlloc_addr = (uintptr_t)so_symbol(&so_mod, "_Z8memAllociPKc");
	// if (memAlloc_addr == 0) {
	// 	log_error("memAlloc not found\n");
	// } else {
	// 	logv_error("memAlloc found at %p\n", memAlloc_addr);
	// 	memAlloc_hook = hook_addr(memAlloc_addr, (uintptr_t)&memAlloc);
	// }

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

	//install_prof_hooks();

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
