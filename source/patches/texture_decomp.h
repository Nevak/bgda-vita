#ifndef TEXTURE_DECOMP_H
#define TEXTURE_DECOMP_H
#include <so_util/so_util.h>

extern float g_uvFactor;
extern float g_uvFactorY;
uintptr_t D3DDevice_SetVertexShaderConstantNotInline_addr;
uintptr_t D3DDevice_SetVertexShaderConstantFast_addr;
typedef void (*D3DDevice_SetVertexShaderConstantNotInlineFn)(int reg, uint32_t pConstantData, uint32_t ConstantCount);
typedef void (*D3DDevice_SetVertexShaderConstantFastFn)(int reg, uint32_t pConstantData, uint32_t ConstantCount);

#define ENABLE_NPOT_TEXTURES

so_hook lowestPowerof2NotLessThan_hook;
int lowestPowerof2NotLessThan(int dimension) {
#ifndef ENABLE_NPOT_TEXTURES
	return SO_CONTINUE(int, lowestPowerof2NotLessThan_hook, dimension);
#endif


	uint32_t caller = (uint32_t)__builtin_return_address(0);
	if (caller != 0x98521dec && caller != 0x98521dcc) {
	 	return SO_CONTINUE(int, lowestPowerof2NotLessThan_hook, dimension);
	}

    int alignment = 32;
    int aligned = (dimension + (alignment - 1)) & ~(alignment - 1);
    if (aligned < alignment) 
		aligned = alignment;
    return aligned;
}

so_hook D3DDevice_SetVertexShaderConstantNotInline_hook;
void D3DDevice_SetVertexShaderConstantNotInline_patched(int reg, uint32_t pConstantData, uint32_t ConstantCount) {
	D3DDevice_SetVertexShaderConstantFastFn D3DDevice_SetVertexShaderConstantFast = (D3DDevice_SetVertexShaderConstantFastFn)D3DDevice_SetVertexShaderConstantFast_addr;
	D3DDevice_SetVertexShaderConstantFast(reg, pConstantData, ConstantCount);
}

so_hook D3DDevice_SetTexture_hook;
void D3DDevice_SetTexture(uint32_t param_1, int param_2) {
#ifndef ENABLE_NPOT_TEXTURES
	return SO_CONTINUE(void *, D3DDevice_SetTexture_hook, param_1, param_2);
#endif

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

		// if (width == 227 && height == 256){
		// 	logv_error("FOUND D3DDevice_SetTexture: w: %i, h: %i\n", width, height);

		// }



		// log the width and height


		int potWidth = 1;
		while (potWidth < width) {
			potWidth <<= 1;
		}
		if (potWidth < 32)
			potWidth = 32;

		int potHeight = 1;

		while (potHeight < height) {
			potHeight <<= 1;
		}
		if (potHeight < 32)
			potHeight = 32;
		//logv_error("D3DDevice_SetTexture: p2w: %i, p2h: %i\n", potWidth, potHeight);

		// #define MAX_TEXTURE_DIM 256
		// if (potWidth > MAX_TEXTURE_DIM || potHeight > MAX_TEXTURE_DIM) {
		// 	// Calculate uniform scale factor based on larger dimension
		// 	int max_dim = (potWidth > potHeight) ? potWidth : potHeight;
		// 	float scale = (float)max_dim / MAX_TEXTURE_DIM;

		// 	potWidth = (int)(potWidth / scale);
		// 	potHeight = (int)(potHeight / scale);
		// 	logv_error("D3DDevice_SetTexture: scaled to w: %i, h: %i\n", potWidth, potHeight);
		// }
			
		// calculate the scale factor for width and height to pass it to the shader so it can scale the UV coordinates
		float scaleX = ((float)potWidth / (float)width);
		float scaleY = ((float)potHeight / (float)height);


		float scale[4] = {scaleX, scaleY, 0.0f, 0.0f};
		SO_CONTINUE(float, D3DDevice_SetVertexShaderConstantNotInline_hook, 24, (uint32_t)scale, 1);
	}
	SO_CONTINUE(void *, D3DDevice_SetTexture_hook, param_1, param_2);
}

so_hook XGSetTextureHeader_hook;
int XGSetTextureHeader(uint32_t width, uint32_t height, int levels, uint32_t usage, uint32_t format, uint32_t pool, uint32_t *pTexture, uint32_t data, uint32_t pitch) {
	// For paletted textures (format 0xb), use width as pitch to avoid padding
	if ((format & 0xffffff7f) == 0xb && width > 64) {
		uint32_t originalPitch = pitch;
		pitch = width; // Use actual width instead of padded pitch
	}

	return SO_CONTINUE(int, XGSetTextureHeader_hook, width, height, levels, usage, format, pool, pTexture, data, pitch);
}

so_hook RegisteredVertexData_GetPatchedData_hook;
void RegisteredVertexData_GetPatchedData(void* param_1, int offset, int size)
{
	//sceRazorCpuPushMarkerWithHud("RegisteredVertexData_GetPatchedData", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
    SO_CONTINUE(void*, RegisteredVertexData_GetPatchedData_hook, param_1, offset, size);
	//sceRazorCpuPopMarker();
}

void patch_texture_decom()
{
	uintptr_t addr = so_symbol(&so_mod, "_ZN20RegisteredVertexData14GetPatchedDataERNS_14PatchContainerEjj");
	logv_error("_ZN20RegisteredVertexData14GetPatchedDataERNS_14PatchContainerEjj at %p", addr);
	RegisteredVertexData_GetPatchedData_hook = hook_addr(addr, (uintptr_t)RegisteredVertexData_GetPatchedData);
}
#endif