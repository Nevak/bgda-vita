#ifndef TEXTURE_PALETTE_H
#define TEXTURE_PALETTE_H

#include "bgda_types.h"
#include <so_util/so_util.h>
#include <string.h>
#include <psp2/kernel/processmgr.h>
#define GL_TEXTURE_MAX_LEVEL 0x813d

// Forward declarations for vitaGL low-level access
void* gpu_alloc_mapped_aligned(size_t alignment, size_t size, vglMemType type);
void* gpu_alloc_palette(uint32_t* palette, uint32_t count, uint32_t bpe);
void gpu_free_palette(void* palette);
SceGxmTexture* vglGetGxmTexture(GLenum target);
void vglSetTexPalette(SceGxmTexture *texture, void *data);

// SCE GXM constants
#define SCE_GXM_PALETTE_ALIGNMENT 64

// OpenGL palette constants
#ifndef GL_COLOR_TABLE
#define GL_COLOR_TABLE 0x80D0
#endif
#ifndef GL_COLOR_INDEX8_EXT
#define GL_COLOR_INDEX8_EXT 0x80E5
#endif

/*
    Call chain：
        1. [ReadCommand from any draw command]
        2. void JBE::D3DDevice::CommitState(D3DDevice *thisPtr)
        3. void __thiscall JBE::D3DDevice::SetTextureStages(D3DDevice *this,ulong stageStateMask)
    X   4. void __thiscall JBE::D3DDevice::TextureStageState::SetToGL(TextureStageState *this,ulong textureStageIndex,Enum samplerType)
        5. void __thiscall TextureStageState::SetToGL(TextureStageState *this,ulong updateFlags,RegisteredBaseTextureData *registeredTexPtr, Enum samplerType)
        6. void __thiscall D3DBaseTexture::BufferToOGL(D3DBaseTexture *this,RegisteredTextureData *registeredTex,D3DPalette *palette, int sample_count)
        7. void ProcessAndUploadTexture(...)
        8. void DoTheFinalGPUUpload(...)
        9. glTexImage2D
*/

so_hook XGGetPixelBufferMinAlpha_hook;
uint XGGetPixelBufferMinAlpha(uint8_t (*param_1) [16], uint32_t param_2,int param_3,int param_4) {
    return 0;
	//return SO_CONTINUE(uint, XGGetPixelBufferMinAlpha_hook, param_1, param_2, param_3, param_4);
}

so_hook XGGetPixelBufferMaxAlpha_hook;
uint XGGetPixelBufferMaxAlpha(uint8_t (*param_1) [16], uint32_t param_2,int param_3,int param_4) {
	return 255;
	//return SO_CONTINUE(uint, XGGetPixelBufferMaxAlpha_hook, param_1, param_2, param_3, param_4);
}

// Native GXM palette implementation - no shader-based palette management needed

// External timing accumulator
extern float g_ProcessAndUploadTextureMs;

so_hook ProcessAndUploadTexture_hook;
void ProcessAndUploadTexture
              (int glTarget,uint8_t *sourceTextureData,uint formatToSwitchParam,int isSwizzled,
               int isCompressed,uint width,uint height,uint level, uint sourcePitch, int paddingFlag,
               uint8_t * palette,uint allocateNewTexture,int keepSwizzled,ushort *alphaRange)
{
	uint64_t timeStart = sceKernelGetProcessTimeWide();

	if (level != 1)
    {
		return;
	}

    // Check for palette format
    if ((formatToSwitchParam & 0xffffff7f) == 0xb && palette)
	{
        uint8_t* indexData;
        indexData = sourceTextureData;
        // if (width == sourcePitch) {
        //     // Direct use of source data
        //     indexData = sourceTextureData;
        //     //logv_error("Using source data directly: %d bytes", width * height);
        // } else if (width < 64) {
        //     // Allocate and copy row by row
        //     indexData = malloc(width * height);
        //     if (!indexData) {
        //         log_error("Failed to allocate texture index data");
        //         goto regular_processing;
        //     }
        //     for (int y = 0; y < height; y++) {
        //         memcpy(indexData + y * width,
        //                sourceTextureData + y * sourcePitch,
        //                width);
        //     }
        //     logv_error("Row-by-row copy: width=%d, height=%d, sourcePitch=%d", width, height, sourcePitch);
        // } else {
        //     // Allocate and direct copy
        //     indexData = malloc(width * height);
        //     if (!indexData) {
        //         log_error("Failed to allocate texture index data");
        //         goto regular_processing;
        //     }
        //     memcpy(indexData, sourceTextureData, width * height);
        //     logv_error("Allocated and copied: %d bytes", width * height);
        // }

        // Upload the paletted texture
        //logv_error("Calling glTexImage2D_fake with internalFormat=GL_COLOR_INDEX8_EXT (0x%X)", GL_COLOR_INDEX8_EXT);
        glTexImage2D(glTarget, 0, GL_COLOR_INDEX8_EXT, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, indexData);

        // Clean up temporary buffer if we allocated one
        if (indexData != sourceTextureData) {
            free(indexData);
        }

        // Now set the palette data directly for this texture
        SceGxmTexture* gxmTex = vglGetGxmTexture(GL_TEXTURE_2D);

        // Free existing palette if there is one (in case texture is being re-uploaded)
        void* oldPalette = sceGxmTextureGetPalette(gxmTex);
        if (oldPalette) {
            log_error("Freeing old palette");
            gpu_free_palette(oldPalette);
        }

        void* paletteData = gpu_alloc_palette((uint32_t*)palette, 256, 4);
        sceGxmTextureSetPalette(gxmTex, paletteData);

        uint64_t timeEnd = sceKernelGetProcessTimeWide();
        float elapsedMs = (timeEnd - timeStart) / 1000.0f;
        g_ProcessAndUploadTextureMs += elapsedMs;

        return;
	}

regular_processing:
    // For non-palette textures, use original processing
	SO_CONTINUE(void *, ProcessAndUploadTexture_hook, glTarget, sourceTextureData, formatToSwitchParam,
		isSwizzled, isCompressed, width, height, level, sourcePitch, paddingFlag,
		palette, allocateNewTexture, keepSwizzled, alphaRange);

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeStart) / 1000.0f;
	g_ProcessAndUploadTextureMs += elapsedMs;
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

so_hook D3DDevice_TextureStageState_SetToGL_hook;
void D3DDevice_TextureStageState_SetToGL(D3DDevice_TextureStageState* this, uint32_t textureStageIndex, uint32_t samplerType)
{
    SO_CONTINUE(void*, D3DDevice_TextureStageState_SetToGL_hook, this, textureStageIndex, samplerType);
}

// Add this hook before the existing glDeleteTextures call
so_hook D3DBaseTexture_Unregister_hook;
void D3DBaseTexture_Unregister(D3DBaseTexture *this, int param_1) {
    RegisteredTextureData *textureData = this->registeredTextureData;

    if (textureData && textureData->glTextureId != 0) {
        // Free the palette if this is a paletted texture
        GLint oldTexture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
        glBindTexture(GL_TEXTURE_2D, textureData->glTextureId);

        SceGxmTexture* gxmTex = vglGetGxmTexture(GL_TEXTURE_2D);
        if (gxmTex) {
            void* paletteData = sceGxmTextureGetPalette(gxmTex);
            if (paletteData) {
                //logv_error("Freeing palette at: %p", paletteData);
                gpu_free_palette(paletteData);
            }
            else {
                logv_error("Failed to get palette for tex: %p", gxmTex);
            }
        }
        else {
            log_error("Texture has no gxmTex");
        }

        glBindTexture(GL_TEXTURE_2D, oldTexture);
    }
    else {
        log_error("textureData is null or glTextureId is 0");
    }

    SO_CONTINUE(void*, D3DBaseTexture_Unregister_hook, this, param_1);
}

so_hook D3DDevice_UnregisterTextureCommand_hook;
void D3DDevice_UnregisterTextureCommand(void *this, RegisteredBaseTextureData *textureData, int *param_2) {
    if (textureData && textureData->glTextureId != 0) {
        // Free the palette if this is a paletted texture
        GLint oldTexture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
        glBindTexture(GL_TEXTURE_2D, textureData->glTextureId);

        SceGxmTexture* gxmTex = vglGetGxmTexture(GL_TEXTURE_2D);
        if (gxmTex) {
            void* paletteData = sceGxmTextureGetPalette(gxmTex);
            if (paletteData) {
                //logv_error("Freeing palette at: %p", paletteData);
                gpu_free_palette(paletteData);
            }
            else {
                logv_error("Failed to get palette for tex: %p", gxmTex);
            }
        }
        else {
            log_error("Texture has no gxmTex");
        }

        glBindTexture(GL_TEXTURE_2D, oldTexture);
    }
    else {
        log_error("textureData is null or glTextureId is 0");
    }

    SO_CONTINUE(void *, D3DDevice_UnregisterTextureCommand_hook, this, textureData, param_2);
}

so_hook D3DBaseTexture_UnbufferToOGL_hook;
void D3DBaseTexture_UnbufferToOGL(D3DBaseTexture *this)
{
    RegisteredTextureData *textureData = this->registeredTextureData;

    if (textureData && textureData->glTextureId != 0) {
        // Free the palette if this is a paletted texture
        GLint oldTexture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
        glBindTexture(GL_TEXTURE_2D, textureData->glTextureId);

        SceGxmTexture* gxmTex = vglGetGxmTexture(GL_TEXTURE_2D);
        if (gxmTex) {
            void* paletteData = sceGxmTextureGetPalette(gxmTex);
            if (paletteData) {
                //logv_error("Freeing palette at: %p", paletteData);
                gpu_free_palette(paletteData);
            }
            else {
                logv_error("Failed to get palette for tex: %p", gxmTex);
            }
        }
        else {
            log_error("Texture has no gxmTex");
        }

        glBindTexture(GL_TEXTURE_2D, oldTexture);
    }
    else {
        log_error("textureData is null or glTextureId is 0");
    }

    SO_CONTINUE(void*, D3DBaseTexture_UnbufferToOGL_hook, this);
}

#endif