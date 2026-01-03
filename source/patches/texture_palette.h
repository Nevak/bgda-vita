#ifndef TEXTURE_PALETTE_H
#define TEXTURE_PALETTE_H

#include "bgda_types.h"
#include <so_util/so_util.h>
#include <string.h>
#include <psp2/kernel/processmgr.h>

#ifdef PROFILER_ENABLED
#include <utils/prof.h>
#include <libperf.h>
#endif

#define GL_TEXTURE_MAX_LEVEL 0x813d

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


extern float g_ProcessAndUploadTextureMs;

so_hook ProcessAndUploadTexture_hook;
void ProcessAndUploadTexture
              (int glTarget, uint8_t *sourceTextureData, uint formatToSwitchParam, int isSwizzled,
               int isCompressed, uint width, uint height, uint level, uint sourcePitch, int paddingFlag,
               uint8_t *palette, uint allocateNewTexture, int keepSwizzled, ushort *alphaRange)
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
        if (width != sourcePitch && width < 64) {
             // Allocate and copy row by row
             indexData = malloc(width * height);
             if (!indexData) {
                 log_error("Failed to allocate texture index data");
                 goto regular_processing;
             }
             for (int y = 0; y < height; y++) {
                 memcpy(indexData + y * width,
                        sourceTextureData + y * sourcePitch,
                        width);
             }
             //logv_error("Row-by-row copy: width=%d, height=%d, sourcePitch=%d", width, height, sourcePitch);
        }
        else
        {
            indexData = sourceTextureData;
        }
        
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
            logv_error("Freeing old palette: %p (width=%u, height=%u)", oldPalette, width, height);
            gpu_free_palette(oldPalette);
            sceGxmTextureSetPalette(gxmTex, NULL);  // Clear the pointer to avoid use-after-free
        }

        void* paletteData = gpu_alloc_palette((uint32_t*)palette, 256, 4);
        sceGxmTextureSetPalette(gxmTex, paletteData);

        uint64_t timeEnd = sceKernelGetProcessTimeWide();
        float elapsedMs = (timeEnd - timeStart) / 1000.0f;
        g_ProcessAndUploadTextureMs += elapsedMs;

        return;
	}

    // Check for format 0x12 (video texture - BGRA format on Android)
    if (formatToSwitchParam == 0x12 && !palette) {
        #ifdef PROFILER_ENABLED
        sceRazorCpuPushMarkerWithHud("Upload as BGRA texture", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
        #endif 

        // Upload as BGRA texture directly
        glTexImage2D(glTarget, 0, GL_BGRA,
                     width, height, 0,
                     GL_BGRA, GL_UNSIGNED_BYTE, sourceTextureData);

        uint64_t timeEnd = sceKernelGetProcessTimeWide();
        float elapsedMs = (timeEnd - timeStart) / 1000.0f;
        g_ProcessAndUploadTextureMs += elapsedMs;
        
        #ifdef PROFILER_ENABLED
        sceRazorCpuPopMarker();
        #endif

        return;
    }

regular_processing:
    #ifdef PROFILER_ENABLED
    sceRazorCpuPushMarkerWithHud("regular_processing", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
    #endif 

    logv_debug("regular_processing: format=0x%X (w=%d, h=%d, pitch=%d, paddingFlag=0x%X)",
               formatToSwitchParam, width, height, sourcePitch, paddingFlag);

	SO_CONTINUE(void*, ProcessAndUploadTexture_hook, glTarget, sourceTextureData, formatToSwitchParam,
		isSwizzled, isCompressed, width, height, level, sourcePitch, paddingFlag,
		palette, allocateNewTexture, keepSwizzled, alphaRange);
    
    #ifdef PROFILER_ENABLED
    sceRazorCpuPopMarker();
    #endif

	// uint64_t timeEnd = sceKernelGetProcessTimeWide();
	// float elapsedMs = (timeEnd - startTime) / 1000.0f;
	// g_ProcessAndUploadTextureMs += elapsedMs;
}

so_hook DoTheFinalGPUUpload_hook;
void DoTheFinalGPUUpload(uint32_t glTarget, uint32_t level, uint8_t (*pixelData) [16],
                        uint32_t textureFormatToSwitch, uint width, uint height, uint32_t imageSize,
						int shouldUploadToGPU)
{
    #ifdef PROFILER_ENABLED
    sceRazorCpuPushMarkerWithHud("DoTheFinalGPUUpload", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
    #endif

	SO_CONTINUE(void *, DoTheFinalGPUUpload_hook, glTarget, level, pixelData, textureFormatToSwitch, width, height, imageSize, shouldUploadToGPU);	

    #ifdef PROFILER_ENABLED
    sceRazorCpuPopMarker();
    #endif
}

// so_hook D3DBaseTexture_GetInfo_hook;
// void D3DBaseTexture_GetInfo(void *this, uint32_t *pFormat, int *isCompressed,int *isSwizzled, uint32_t *width, uint32_t *height)
// {
//     log_error("D3DBaseTexture_GetInfo called");

//     SO_CONTINUE(void*, D3DBaseTexture_GetInfo_hook, this, pFormat, isCompressed, isSwizzled, width, height);
// }

typedef void (*D3DBaseTexture_GetInfo_t)(void*,uint32_t *, int *,int *, uint32_t *, uint32_t *);


so_hook D3DTexture_LockRect_hook;
void D3DTexture_LockRect(void *pThis, uint32_t Level, int *pLockedRect, int *pRect, int flags)
{
    // Call original
    SO_CONTINUE(void*, D3DTexture_LockRect_hook, pThis, Level, pLockedRect, pRect, flags);

    // Check if this is a video texture with the problematic 4096 pitch
    // Video textures are 640 pixels wide with 4096 byte pitch (Xbox alignment)
    if (pLockedRect[0] == 4096) {

        uint32_t format;
        int isCompressed;
        int isSwizzled;
        uint32_t width;
        uint32_t height;
        D3DBaseTexture_GetInfo_t D3DBaseTexture_GetInfo = (D3DBaseTexture_GetInfo_t)((uintptr_t)LOC(0x00209c1c));

        D3DBaseTexture_GetInfo(pThis, &format, &isCompressed, &isSwizzled, &width, &height);


       // Video textures are always 640 pixels wide, BGRA format (4 bytes per pixel)
       //if (width == 640) {
       {
            uint32_t correct_pitch = width * 4;  // 2560 bytes

           // logv_error("D3DTexture_LockRect: Video texture detected (width=%d), fixing pitch 4096 → %d",width,
            //           correct_pitch);

            pLockedRect[0] = correct_pitch;
        }

        //pLockedRect[0] = width*4;
    }
}

so_hook D3DDevice_TextureStageState_SetToGL_hook;
void D3DDevice_TextureStageState_SetToGL(D3DDevice_TextureStageState* this, uint32_t textureStageIndex, uint32_t samplerType)
{
    #ifdef PROFILER_ENABLED
    sceRazorCpuPushMarkerWithHud("D3DDevice_TextureStageState_SetToGL", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
    #endif

    SO_CONTINUE(void*, D3DDevice_TextureStageState_SetToGL_hook, this, textureStageIndex, samplerType);
    
    #ifdef PROFILER_ENABLED
    sceRazorCpuPopMarker();
    #endif
}

so_hook D3DDevice_UnregisterTextureCommand_hook;
void D3DDevice_UnregisterTextureCommand(void *this, RegisteredBaseTextureData *textureData, int *param_2) {
    //logv_error("[UnregisterTextureCommand] textureData=%p, glTexId=%u", textureData, textureData ? textureData->glTextureId : 0);

    if (textureData && textureData->glTextureId != 0) {
        // Free the palette if this is a paletted texture
        // NOTE: We bind the texture but DON'T restore the old binding
        // TextureDeleted() will handle unbinding properly
        glBindTexture(GL_TEXTURE_2D, textureData->glTextureId);

        SceGxmTexture* gxmTex = vglGetGxmTexture(GL_TEXTURE_2D);
        if (gxmTex) {
            void* paletteData = sceGxmTextureGetPalette(gxmTex);
            if (paletteData) {
                //logv_error("[UnregisterTextureCommand] Freeing palette: %p", paletteData);
                gpu_free_palette(paletteData);
                sceGxmTextureSetPalette(gxmTex, NULL);  // Clear palette pointer to prevent vitaGL from accessing freed memory
            }
        }
    }

    SO_CONTINUE(void *, D3DDevice_UnregisterTextureCommand_hook, this, textureData, param_2);
}

#endif