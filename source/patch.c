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

// void D3DDevice_SetVertexShaderConstantNotInline(int register,undefined4 pConstantData,ulong ConstantCount)
uint32_t g_pConstantData = 0;
so_hook D3DDevice_SetVertexShaderConstantNotInline_hook;
float D3DDevice_SetVertexShaderConstantNotInline(int reg, uint32_t pConstantData, uint32_t ConstantCount) {
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
