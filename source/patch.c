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
#include <utils/trophies.h>
#include <stdio.h>
#include <vitasdk.h>

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

int ret0() { return 0; }
int ret1() { return 1; }

so_hook coreAddTask_hook;
int coreAddTask(void *param_1, int param_2, char *param_3) {
	logv_error("coreAddTask(%p, %i, %s)", param_1, param_2, param_3);
	int returnval = SO_CONTINUE(int, coreAddTask_hook, param_1, param_2, param_3);
	//logv_error("coreAddTask returned %i\n", returnval);
	return returnval;
}

extern int log_allocs;

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


static inline int UnsignedSaturate8(int x) {
    if (x < 0)   return 0;
    if (x > 255) return 255;
    return x;
}

//so_hook texture_copy_hook;

// __attribute__((naked)) 
// __attribute__((target("arm")))
// void texture_copy(void *dst, void *src, size_t size)
// {
// 	// Every local variable will increase the stack pointer by 4 bytes
// 	uint32_t* stackPtr = 0;

// 	__asm__ volatile (
// 		".arm\n"

// 		"mov %0, sp\n"

//         "push {r4-r11}\n"

//         // 3) Grab arguments from r0, r1, r2 off the stack or directly 
//         "mov r4, r0\n" // dest
//         "mov r5, r1\n" // src
//         "mov r6, r2\n" // len
// 		"mov r7, %0\n" // stack pointer to r7

//         // 4) Now call a small helper in C to do logging + call real memcpy
//         "bl texture_copy_impl\n"

//         // 5) Restore regs and return to the caller (the code after the BL)
//         "pop {r4-r11}\n"

// 		: "=r"(stackPtr)
//     );

// 	__asm__ volatile (
// 		// Force ARM mode if necessary
// 		".arm\n"

// 		// 2) Restore callee-saved registers
// 		//"pop {r4-r11}\n"

// 		"cpy r0, r7\n"

//         // The instruction: LDR PC, [PC, #-4]
//         ".word 0xe51ff004\n"
//         // The next word: absolute destination address
//         ".word 0x98522478\n"
//     );
// }

// void texture_copy_impl(void) {
//     // r4,r5,r6 hold dest, src, len
//     uintptr_t* destPtr;
//     unsigned char* src;
//     size_t len;
// 	uint32_t* stackPtr;
// 	__asm__ volatile (
// 		"mov %0, r4\n" // dest
// 		"mov %1, r5\n" // src
// 		"mov %2, r6\n" // len
// 		"mov %3, r7\n" // stack pointer
		
// 		: "=r"(destPtr), "=r"(src), "=r"(len), "=r"(stackPtr)
// 	);
	
// 	uint32_t* pitchPtr = (stackPtr + 0x56 - 0x2);
// 	uint32_t* actualWidthPtr = (stackPtr + 0x25 - 0x2);
// 	uint32_t* actualHeightPtr = (stackPtr + 0x24 - 0x2);
	
// 	int pitch = *pitchPtr;
// 	int actualHeight = *actualHeightPtr;
// 	int actualWidth = *actualWidthPtr;

// 	int rows = actualHeight;     
// 	int rowBytes = actualWidth;  
// 	unsigned char *dest = (unsigned char*)destPtr;
// 	for (int y = 0; y < rows; ++y) {
// 		//logv_error("copying row %d of %d from %p to %p, rowBytes=%d\n", y, rows, src, destPtr, rowBytes);
// 		// set the memory to all 1s just for testing
// 		//memset(dest, 0xFF, rowBytes);
// 		// src[0] = 0xFF;
// 		__aeabi_memcpy(dest, src, rowBytes);
// 		dest += pitch;
// 	 	src += rowBytes;
// 	}
// }

// void D3DDevice_SetVertexShaderConstantNotInline(int register,undefined4 pConstantData,ulong ConstantCount)
uint32_t g_pConstantData = 0;
so_hook D3DDevice_SetVertexShaderConstantNotInline_hook;
float D3DDevice_SetVertexShaderConstantNotInline(int reg, uint32_t pConstantData, uint32_t ConstantCount) {
	// Get the caller address
	// uintptr_t caller = __builtin_return_address(0);
	// if (caller == 0x98528c64)
	// {
	// 	g_pConstantData = pConstantData;
	// 	logv_error("D3DDevice_SetVertexShaderConstantNotInline: register: %i, pConstantData: 0x%x, ConstantCount: %u, caller: 0x%x\n", reg, pConstantData, ConstantCount, caller);
	// }
	return SO_CONTINUE(float, D3DDevice_SetVertexShaderConstantNotInline_hook, reg, pConstantData, ConstantCount);
}

// void D3DDevice_SetTexture(ulong param_1,int param_2)
so_hook D3DDevice_SetTexture_hook;
extern float g_uvFactor;
extern float g_uvFactorY;
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
			D3DDevice_SetVertexShaderConstantNotInline(24, (uint32_t)scale, 1);
		}
		else {
			float scale[4] = {scaleX, scaleY, 0.0f, 0.0f};
			//float scale[4] = {g_uvFactor * scaleX, g_uvFactorY * scaleY, 0.0f, 0.0f};

			D3DDevice_SetVertexShaderConstantNotInline(24, (uint32_t)scale, 1);
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

float total_ov_read_time = 0.0f;
uint32_t frameNum = 0;
so_hook SND_Frame_hook;
// Measure the time it takes to execute SND_Frame
void SND_Frame() {
	float timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, SND_Frame_hook);
	float timeAfter = sceKernelGetProcessTimeWide();
	if (timeAfter - timeNow > 2000) {
		logv_error("[%d] SND_Frame took %f ms\n", frameNum, (timeAfter - timeNow) / 1000);
		logv_error("[%d] total_ov_read_time: %f ms\n", frameNum, total_ov_read_time);
	}

	total_ov_read_time = 0.0f;
	++frameNum;
}

//void SND_PlaySoundNew(void *param_1,Point3 param_2,float param_3,float param_4,float param_5)
so_hook SND_PlaySoundNew_hook;
void SND_PlaySoundNew(void *param_1,uint64_t param_2,float param_3,float param_4,float param_5) {
	logv_error("SND_PlaySoundNew(%p, %p, %f, %f, %f)\n", param_1, param_2, param_3, param_4, param_5);
	SO_CONTINUE(void *, SND_PlaySoundNew_hook, param_1, param_2, param_3, param_4, param_5);
	//logv_error("SND_PlaySoundNew returned %i\n", returnval);
}

so_hook DirectSoundDoWork_hook;
void DirectSoundDoWork(void) {
	float timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, DirectSoundDoWork_hook);
	float timeAfter = sceKernelGetProcessTimeWide();
	if (timeAfter - timeNow > 2000) {
		logv_error("DirectSoundDoWork took %f ms\n", (timeAfter - timeNow) / 1000);
	}
	//logv_error("DirectSoundDoWork returned %i\n", returnval);
}

so_hook ov_raw_seek_hook;
void ov_raw_seek(int *param_1,uint param_2,uint param_3,int param_4) {
	float timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, ov_raw_seek_hook, param_1, param_2, param_3, param_4);
	float timeAfter = sceKernelGetProcessTimeWide();
	if (timeAfter - timeNow > 2000) {
		logv_error("ov_raw_seek took %f ms\n", (timeAfter - timeNow) / 1000);
	}
}

so_hook SND_StartStream_hook;
// _Z15SND_StartStreamiPKciii
void SND_StartStream(int param_1,char *param_2,int param_3,int param_4,int param_5) {
	//logv_error("SND_StartStream(%i, %s, %i, %i)\n", param_1, param_2, param_3, param_4);
	float timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, SND_StartStream_hook, param_1, param_2, param_3, param_4, param_5);
	float timeAfter = sceKernelGetProcessTimeWide();
	if (timeAfter - timeNow > 2000) {
		logv_error("[%d] SND_StartStream took %f ms\n",frameNum, (timeAfter - timeNow) / 1000);
	}
}

so_hook ov_read_hook;
// void ov_read(int param_1,ushort *param_2,undefined4 param_3,int param_4,int param_5,int param_6,undefined4 *param_7)
void ov_read(int param_1, uint16_t *param_2, uint32_t param_3, int param_4, int param_5, int param_6, uint32_t *param_7) {
	float timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, ov_read_hook, param_1, param_2, param_3, param_4, param_5, param_6, param_7);
	float timeAfter = sceKernelGetProcessTimeWide();
	total_ov_read_time += (timeAfter - timeNow) / 1000;
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
			sceClibMemcpy(destPtr, src, rowBytes);
			destPtr += pitch;
			src += rowBytes;
		}

		return;
	}

	sceClibMemcpy(dst, src, n);
}


void so_patch(void) {

	log_error("Patching .so functions\n");
	
	D3DDevice_SetTexture_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "D3DDevice_SetTexture"), (uintptr_t)&D3DDevice_SetTexture);
	D3DDevice_SetVertexShaderConstantNotInline_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "D3DDevice_SetVertexShaderConstantNotInline"), (uintptr_t)&D3DDevice_SetVertexShaderConstantNotInline);

	// _Z11coreAddTaskPFvvEiPKc coreAddTask
	coreAddTask_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z11coreAddTaskPFvvEiPKc"), (uintptr_t)&coreAddTask);
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

	// _ZN3JBE5Input6RenderEv
	uintptr_t inputRender_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE5Input6RenderEv");
	if (inputRender_addr == 0) {
		log_error("inputRender not found\n");
	} else {
		logv_error("inputRender found at %p\n", inputRender_addr);
		inputRender_hook = hook_addr(inputRender_addr, (uintptr_t)&inputRender);
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

	// _Z9SND_Framev
	uintptr_t SND_Frame_addr = (uintptr_t)so_symbol(&so_mod, "_Z9SND_Framev");
	if (SND_Frame_addr == 0) {
		log_error("SND_Frame not found\n");
	} else {
		logv_error("SND_Frame found at %p\n", SND_Frame_addr);
		//SND_Frame_hook = hook_addr(SND_Frame_addr, (uintptr_t)&SND_Frame);
	}

	// _Z16SND_PlaySoundNewPv6Point3fff
	uintptr_t SND_PlaySoundNew_addr = (uintptr_t)so_symbol(&so_mod, "_Z16SND_PlaySoundNewPv6Point3fff");
	if (SND_PlaySoundNew_addr == 0) {
		log_error("SND_PlaySoundNew not found\n");
	} else {
		logv_error("SND_PlaySoundNew found at %p\n", SND_PlaySoundNew_addr);
		//SND_PlaySoundNew_hook = hook_addr(SND_PlaySoundNew_addr, (uintptr_t)&SND_PlaySoundNew);
	}

	// DirectSoundDoWork
	uintptr_t DirectSoundDoWork_addr = (uintptr_t)so_symbol(&so_mod, "DirectSoundDoWork");
	if (DirectSoundDoWork_addr == 0) {
		log_error("DirectSoundDoWork not found\n");
	} else {
		logv_error("DirectSoundDoWork found at %p\n", DirectSoundDoWork_addr);
		//DirectSoundDoWork_hook = hook_addr(DirectSoundDoWork_addr, (uintptr_t)&DirectSoundDoWork);
	}

	// void ov_raw_seek(int *param_1,undefined4 param_2,uint param_3,int param_4)
	uintptr_t ov_raw_seek_addr = (uintptr_t)so_symbol(&so_mod, "ov_raw_seek");
	if (ov_raw_seek_addr == 0) {
		log_error("ov_raw_seek not found\n");
	} else {
		logv_error("ov_raw_seek found at %p\n", ov_raw_seek_addr);
		//ov_raw_seek_hook = hook_addr(ov_raw_seek_addr, (uintptr_t)&ov_raw_seek);
	}

	//_Z15SND_StartStreamiPKciii
	uintptr_t SND_StartStream_addr = (uintptr_t)so_symbol(&so_mod, "_Z15SND_StartStreamiPKciii");
	if (SND_StartStream_addr == 0) {
		log_error("SND_StartStream not found\n");
	} else {
		logv_error("SND_StartStream found at %p\n", SND_StartStream_addr);
		//SND_StartStream_hook = hook_addr(SND_StartStream_addr, (uintptr_t)&SND_StartStream);
	}

	// void ov_read(int param_1,ushort *param_2,undefined4 param_3,int param_4,int param_5,int param_6,undefined4 *param_7)
	uintptr_t ov_read_addr = (uintptr_t)so_symbol(&so_mod, "ov_read");
	if (ov_read_addr == 0) {
		log_error("ov_read not found\n");
	} else {
		logv_error("ov_read found at %p\n", ov_read_addr);
		//ov_read_hook = hook_addr(ov_read_addr, (uintptr_t)&ov_read);
	}

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
