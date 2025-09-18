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

    int alignment = 64;
    int aligned = (dimension + (alignment - 1)) & ~(alignment - 1);
    if (aligned < alignment) 
		aligned = alignment;
    return aligned;
}

//void D3DDevice_SetVertexShaderConstantNotInline(int register,undefined4 pConstantData,ulong ConstantCount)
//uint32_t g_pConstantData = 0;
so_hook D3DDevice_SetVertexShaderConstantNotInline_hook;
void D3DDevice_SetVertexShaderConstantNotInline_patched(int reg, uint32_t pConstantData, uint32_t ConstantCount) {
	//Profiler_BeginSample("D3DDevice_SetVertexShaderConstantNotInline");
	D3DDevice_SetVertexShaderConstantFastFn D3DDevice_SetVertexShaderConstantFast = (D3DDevice_SetVertexShaderConstantFastFn)D3DDevice_SetVertexShaderConstantFast_addr;
	D3DDevice_SetVertexShaderConstantFast(reg, pConstantData, ConstantCount);
	//Profiler_EndSample();
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

		float scale[4] = {scaleX, scaleY, 0.0f, 0.0f};
		SO_CONTINUE(float, D3DDevice_SetVertexShaderConstantNotInline_hook, 24, (uint32_t)scale, 1);

	}
	SO_CONTINUE(void *, D3DDevice_SetTexture_hook, param_1, param_2);
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

so_hook XGSetTextureHeader_hook;
int XGSetTextureHeader(uint32_t width, uint32_t height, int levels, uint32_t usage, uint32_t format, uint32_t pool, uint32_t *pTexture, uint32_t data, uint32_t pitch) {
	// For paletted textures (format 0xb), use width as pitch to avoid padding
	if ((format & 0xffffff7f) == 0xb) {
		uint32_t originalPitch = pitch;
		pitch = width; // Use actual width instead of padded pitch
		//logv_error("XGSetTextureHeader: Overriding pitch for paletted texture format 0x%x, width=%u, original pitch=%u, new pitch=%u", format, width, originalPitch, pitch);
	}

	return SO_CONTINUE(int, XGSetTextureHeader_hook, width, height, levels, usage, format, pool, pTexture, data, pitch);
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
  #ifndef ENABLE_NPOT_TEXTURES
      memcpy(dst, src, n);
      return;
  #endif

      int* caller = __builtin_return_address(0);
      if (caller == LOC(0x00132478)) {
			// Only apply custom logic if width != pitch (i.e., NPOT texture)
			if (g_width == g_pitch) {
				//log_error("Texture is already PoT-aligned, using normal memcpy");
				memcpy(dst, src, n);
				return;
			}
          int actualHeight = g_height;
          int actualWidth = g_width;
          int pitch = g_pitch;
          logv_error("NPOT memcpy: w=%d h=%d pitch=%d n=%d", g_width, g_height, g_pitch, n);

          // Verify the size makes sense
          if (n != actualHeight * actualWidth) {
              logv_error("Size mismatch! Expected %d, got %d - falling back to memcpy",
                         actualHeight * actualWidth, n);
              memcpy(dst, src, n);
              return;
          }

          unsigned char *dest = (unsigned char*)dst;
          unsigned char *source = (unsigned char*)src;

          logv_error("Copying %d rows of %d bytes each, dest pitch=%d",
                     actualHeight, actualWidth, pitch);

		// for (int y = 0; y < actualHeight; ++y) {
		// 	memcpy(dest, source, actualWidth);  // Copy 192 bytes
		// 	dest += pitch;                      // Skip to next row: +256 bytes
		// 	source += actualWidth;              // Skip to next row: +192 bytes
		// }
		
		for (int y = 0; y < actualHeight; ++y) {
			// Fill the row with a test pattern
			for (int x = 0; x < actualWidth; ++x) {
				if (x == actualWidth - 1) {
					// Last pixel of each row = green
					dest[x] = 255;
				} else {
					// Normal pixels = red
					dest[x] = 128;
				}
			}

			dest += pitch;
			source += actualWidth;
		}

          log_error("NPOT copy completed");
          return;
      }

      memcpy(dst, src, n);
  }

#endif