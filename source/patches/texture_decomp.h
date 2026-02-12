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
// #ifndef ENABLE_NPOT_TEXTURES
// 	return SO_CONTINUE(int, lowestPowerof2NotLessThan_hook, dimension);
// #endif


// 	uint32_t caller = (uint32_t)__builtin_return_address(0);
// 	if (caller != 0x98521dec && caller != 0x98521dcc) {
// 	 	return SO_CONTINUE(int, lowestPowerof2NotLessThan_hook, dimension);
// 	}

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

		// log the width and height
		//logv_error("D3DDevice_SetTexture: w: %i, h: %i\n", width, height);


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

			
		// calculate the scale factor for width and height to pass it to the shader so it can scale the UV coordinates
		float scaleX = ((float)potWidth / (float)width);
		float scaleY = ((float)potHeight / (float)height);

		float scale[4] = {scaleX, scaleY, 0.0f, 0.0f};
		SO_CONTINUE(float, D3DDevice_SetVertexShaderConstantNotInline_hook, 24, (uint32_t)scale, 1);
	}
    else if (caller == LOC(0x000dd618))
    {
        //logv_debug("D3DDevice_SetTexture called by %p with param_1=%d, param_2=%d", (void*)caller, param_1, param_2);
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

        //logv_debug("D3DDevice_SetTexture called by %p with w=%d, h=%d", (void*)caller, width, height);

        int videoStride = width == 640 ? 672 : 880;
        float scaleX = ((float)width / (float)videoStride);
        float scale[4] = {scaleX, 1.0f, 0.0f, 0.0f};
        SO_CONTINUE(float, D3DDevice_SetVertexShaderConstantNotInline_hook, 1, (uint32_t)scale, 1);
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

so_hook D3DDevice_CreateTexture2_hook;
extern float g_D3DDevice_CreateTexture2Ms;
uint32_t XGBytesPerPixelFromFormat(uint32_t param_1)
{
  uint32_t uVar1;
  
  uVar1 = 4;
  switch(param_1) {
  case 0:
  case 1:
  case 0xb:
  case 0xe:
  case 0xf:
  case 0x13:
  case 0x19:
  case 0x1b:
  case 0x1f:
  case 0x55:
  case 0x5d:
  case 0x5e:
  case 0x62:
  case 0x8b:
  case 0xd5:
    return 1;
  case 2:
  case 3:
  case 4:
  case 5:
  case 0x10:
  case 0x11:
  case 0x16:
  case 0x17:
  case 0x1a:
  case 0x1c:
  case 0x1d:
  case 0x20:
  case 0x27:
  case 0x28:
  case 0x29:
  case 0x2c:
  case 0x2d:
  case 0x30:
  case 0x31:
  case 0x32:
  case 0x35:
  case 0x37:
  case 0x38:
  case 0x39:
  case 0x3d:
  case 0x3e:
  case 0x50:
  case 0x51:
  case 0x52:
  case 0xd0:
  case 0xd1:
  case 0xd2:
    return 2;
  case 6:
  case 7:
  case 0x12:
  case 0x1e:
  case 0x24:
  case 0x25:
  case 0x2a:
  case 0x2b:
  case 0x2e:
  case 0x2f:
  case 0x33:
  case 0x3a:
  case 0x3b:
  case 0x3c:
  case 0x3f:
  case 0x40:
  case 0x41:
  case 0x53:
  case 0x56:
  case 0x57:
  case 0x81:
  case 0xd3:
  case 0xd6:
  case 0xd7:
    break;
  default:
    uVar1 = 0;
  }
  return uVar1;
}



int LZCOUNT(unsigned int x) {
    if (x == 0) {
        return 32;
    }
    
    int n = 0;
    
    if ((x & 0xFFFF0000) == 0) { n += 16; x <<= 16; }
    if ((x & 0xFF000000) == 0) { n += 8;  x <<= 8;  }
    if ((x & 0xF0000000) == 0) { n += 4;  x <<= 4;  }
    if ((x & 0xC0000000) == 0) { n += 2;  x <<= 2;  }
    if ((x & 0x80000000) == 0) { n += 1;           }
    
    return n;
}

typedef void (*D3DBaseTexture_Register_t)(void *this,void *param_1,int param_2,int param_3,int param_4,int param_5);
typedef void* (*JBE_Mem_Alloc_t)(uint param_1,int param_2,int param_3,char *param_4,...);
typedef uint (*D3DResource_AddRef_t)(void *param_1);


D3DBaseTexture* D3DDevice_CreateTexture2(
    int width,
    int height,
    uint32_t depth,
    int levels,
    uint32_t usage,
    uint32_t format,
    uint32_t resourceType)
{
    uintptr_t caller = (uintptr_t)__builtin_return_address(0);

	D3DBaseTexture_Register_t D3DBaseTexture_Register = (D3DBaseTexture_Register_t)LOC(0x00213b10);
	JBE_Mem_Alloc_t JBE_Mem_Alloc = (JBE_Mem_Alloc_t)LOC(0x0021ceac);
	D3DResource_AddRef_t D3DResource_AddRef = (D3DResource_AddRef_t)LOC(0x001db52c);

    int bytesPerPixel;
    uint32_t pitch;
    uint32_t allocationSize;
    void* textureHeader;
    void* textureData = NULL;
    uint32_t poolClass;
    int log2Width;
    int needsRoundUp;
    
    bytesPerPixel = XGBytesPerPixelFromFormat(format);
    


    if (levels == 0) {
        levels = 1;
    }
    
    log2Width = 0;
    needsRoundUp = 1;
    
    uint32_t widthCheck = (width + 1) >> 1;
    if (widthCheck != 0) {
        uint32_t temp = widthCheck;
        int bitCount = 0;
        
        int leadingZeros = __builtin_clz(temp);  // ARM CLZ instruction
        log2Width = 32 - leadingZeros;
        
        needsRoundUp = 0;
        while (temp > 0) {
            needsRoundUp += (temp & 1);
            temp >>= 1;
        }
        needsRoundUp--;
        
        if (needsRoundUp != 0) {
            needsRoundUp = 1;
        }
    }
    
    pitch = bytesPerPixel << (needsRoundUp + log2Width);
    
    if (bytesPerPixel == 0) {
        pitch = (1 << (needsRoundUp + log2Width)) >> 1;
    }
    
    if (pitch < 0x40) {
        pitch = 0x40;
    }
    
    allocationSize = 0x14;
    
    if ((usage & 0x3) == 0) {
        allocationSize = pitch * height + 0x14;
    }

    if (caller == 0x984cd330) 
    {
        textureHeader = vgl_malloc(allocationSize, VGL_MEM_VRAM);
    }
    else
    {
        textureHeader = JBE_Mem_Alloc(allocationSize, 0, 4, "D3DTexture");
    }

    if (textureHeader == NULL) {
        return NULL;
    }
    
    XGSetTextureHeader(width, height, levels, usage, format, 0, textureHeader, 0, pitch);

    D3DResource_AddRef(textureHeader);
    
    if ((usage & 0x3) == 0) {
        textureData = (char*)textureHeader + 0x14;
    }

    poolClass = 4;
    if ((int)usage < 0) {
        poolClass = 9;
    }
    else if ((usage & 0x40000000) == 0) {
        poolClass = (usage >> 28) & 0x2;
    }
    
    D3DBaseTexture_Register(textureHeader, textureData, 0, (textureData == NULL) ? 1 : 0, 0, poolClass);
    
    return (D3DBaseTexture*)textureHeader;
}
#endif

so_hook D3DDevice_SelectVertexShader_hook;
void D3DDevice_SelectVertexShader(void *shader, void *unkn) {
    SO_CONTINUE(void *, D3DDevice_SelectVertexShader_hook, shader, unkn);
    // If using the flat.xvu shader, 
    // set the UV scale factor to 1.0 by default.
    // The patch in D3DDevice_SetTexture_hook will override it with the correct value 
    // for video textures, but for regular textures we want it to be 1.0
    if (shader == *LOC(0x004c28c0)){
        float scale[4] = {1.0f, 1.0f, 0.0f, 0.0f};
        SO_CONTINUE(float, D3DDevice_SetVertexShaderConstantNotInline_hook, 1, (uint32_t)scale, 1);
    }
}
