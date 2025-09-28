#ifndef TEXTURE_PALETTE_H
#define TEXTURE_PALETTE_H

#include "bgda_types.h"
#include <so_util/so_util.h>
#include <string.h>
#define GL_TEXTURE_MAX_LEVEL 0x813d

// Forward declarations for vitaGL low-level access
void* gpu_alloc_mapped_aligned(size_t alignment, size_t size, vglMemType type);
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

so_hook ProcessAndUploadTexture_hook;
void ProcessAndUploadTexture
              (int glTarget,uint8_t *sourceTextureData,uint formatToSwitchParam,int isSwizzled,
               int isCompressed,uint width,uint height,uint level, uint sourcePitch, int paddingFlag,
               uint8_t * palette,uint allocateNewTexture,int keepSwizzled,ushort *alphaRange)
{
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
        glTexImage2D_fake(glTarget, 0, GL_COLOR_INDEX8_EXT, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, indexData);

        // Clean up temporary buffer if we allocated one
        if (indexData != sourceTextureData) {
            free(indexData);
        }

        // Now set the palette data directly for this texture
        SceGxmTexture* gxmTex = vglGetGxmTexture(GL_TEXTURE_2D);
        void* paletteData = gpu_alloc_palette((uint32_t*)palette, 256, 4);
        sceGxmTextureSetPalette(gxmTex, paletteData);

        return;
	}

regular_processing:
    // For non-palette textures, use original processing
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

so_hook D3DDevice_TextureStageState_SetToGL_hook;
void D3DDevice_TextureStageState_SetToGL(D3DDevice_TextureStageState* this, uint32_t textureStageIndex, uint32_t samplerType)
{
    if (textureStageIndex != 0)
    {
        return SO_CONTINUE(void *, D3DDevice_TextureStageState_SetToGL_hook, this, textureStageIndex, samplerType);
    }

    /*
    RegisteredTextureData* textureData = this->registeredTexturePtr;
    if (textureData != 0x0) {
        //logv_error("D3DDevice_TextureStageState_SetToGL 1: textureData %p, d3dBaseTexture %p, unk_bytes[0] = 0x%02x\n", (void*)textureData, (void*)textureData->d3dBaseTexture, textureData->unk_bytes[0]);
        if (textureData->unk_bytes[0] != '\0')
        {
            textureData = textureData->d3dBaseTexture->registeredTextureData;
        }
        //logv_error("D3DDevice_TextureStageState_SetToGL 2: textureData %p has unk_bytes[0] = 0x%02x, skipping palette setup\n", (void*)textureData, textureData->unk_bytes[0]);
        if (textureData->d3dBaseTexture != 0x0) {
            // print the contents around "this"
            //logv_error("D3DDevice_TextureStageState_SetToGL 2a: textureData %p has unk_bytes[0] = 0x%02x, but d3dBaseTexture is %p\n", (void*)textureData, textureData->unk_bytes[0], (void*)textureData->d3dBaseTexture);
            //TextureStageState * textureStageState = &textureData->state;
            TextureStageState * textureStageState = &textureData->state;
            if (textureStageState != NULL) {
                //logv_error("D3DDevice_TextureStageState_SetToGL 3: textureStageState %p\n", (void*)textureStageState);
                uint8_t* palette = textureStageState->palettePtr;
                //logv_error("D3DDevice_TextureStageState_SetToGL 3a: palettePtr %p\n", (void*)textureStageState->palettePtr);
                //log_error("OK");
                //uint8_t* palette = (uint8_t*)(this + 0x80);
                if (palette) {
                    //logv_error("D3DDevice_TextureStageState_SetToGL 4: palette %p\n", (void*)palette);
                    // print the palette color values
                    // for (int i = 0; i < 256; i++) {
                    //     logv_error("Palette color %d: R=%02X G=%02X B=%02X A=%02X", i,
                    //         palette[i*4+0],
                    //         palette[i*4+1],
                    //         palette[i*4+2],
                    //         palette[i*4+3]);
                    // }

                    GLint currentActiveTexUnit;
                    glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&currentActiveTexUnit);

                    glActiveTexture(GL_TEXTURE6);

                    uint32_t paletteId = isPaletteRegistered((uint32_t)palette);
                    if (paletteId != 0) 
                    {
                        glBindTexture(GL_TEXTURE_2D, paletteId);
                        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // defensive; rows are 256*4 = 1024 (already aligned)          
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                        //GLint prog = 8;
                        GLint prog = 0;
                        glGetIntegerv(GL_CURRENT_PROGRAM, &prog); 
                        if (glIsProgram(prog))
                        {
                            //glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
                            GLint loc = glGetUniformLocation(prog, "uPalette");
                            //logv_error("Palette texture bound to unit 6, uniform location is %d\n", loc);
                            if (loc >= 0) {
                                //logv_error("33333 Palette texture (id: %d at addr: %p) bound to unit 6, uniform location is %d, progid is %d", paletteId, (void*)palette, loc, prog);
                                glUniform1i(loc, 6); // set the sampler to texture unit 6
                            }
                            else {
                                //logv_error("Could not find uniform location for uPalette 33333, progid is %d", prog);
                            }
                        }
                    }
                    else {
                        glGenTextures(1, &paletteId);
                        //logv_error("D3DDevice_TextureStageState_SetToGL: registering new palette %p with glTexId %u", (void*)palette, paletteId);

                        registerPalette(palette, paletteId);
                        glBindTexture(GL_TEXTURE_2D, paletteId);

                        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // defensive; rows are 256*4 = 1024 (already aligned)
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                        GLint prog = 8;
                        if (glIsProgram(prog))
                        {
                            //glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
                            GLint loc = glGetUniformLocation(prog, "uPalette");
                            // logv_error("Palette texture bound to unit 6, uniform location is %d\n", loc);
                            if (loc >= 0) {
                                //logv_error("4444 Palette texture (id: %d at addr: %p) bound to unit 6, uniform location is %d, progid is %d", paletteId, (void*)palette, loc, prog);
                                glUniform1i(loc, 6); // set the sampler to texture unit 6
                            }
                            else {
                                //logv_error("Could not find uniform location for uPalette 33333, progid is %d", prog);
                            }
                        }
                    }

                    // restore the previously active texture
                    glActiveTexture(currentActiveTexUnit);
                }
            }
        }
        
    }
        */
        
    // RegisteredTextureData* textureData = this->registeredTexturePtr;
    // if (textureData != 0x0) {
    //     if (textureData->d3dBaseTexture == 0x0) {
    //         return;
    //     }
    //     RegisteredTextureData* textureData2 = (RegisteredTextureData*)textureData->d3dBaseTexture->registeredTextureData;
    //     RegisteredBaseTextureData * textureDataBase = (RegisteredBaseTextureData*)textureData2;
    //     if (textureDataBase != 0x0) {
    //         TextureStageState * textureStageState = &this->textureStageStates[textureStageIndex];
    //     }
    // }
    SO_CONTINUE(void*, D3DDevice_TextureStageState_SetToGL_hook, this, textureStageIndex, samplerType);
}

// void cleanupPaletteForTexture(RegisteredTextureData* textureData) {
//     //logv_error("cleanupPaletteCalled: textureData: %p", textureData);

//     if (!textureData) {
//         //log_error("cleanupPaletteForTexture: textureData is null, skipping");
//         return;
//     }

//     // Fix: Take address of state, don't dereference it
//     TextureStageState* textureStageState = &textureData->state;
//     //logv_error("cleanupPaletteCalled: textureStageState: %p", textureStageState);

//     if (textureStageState && textureStageState->palettePtr) {
//         uint32_t paletteAddr = (uint32_t)textureStageState->palettePtr;

//         for (uint32_t i = 0; i < curTexIndex; i++) {
//             if (((uint32_t)registeredPalettes[i].paletteAddr & 0xfffffffe) ==
//                 (paletteAddr & 0xfffffffe)) {

//                 // logv_error("Cleaning up palette glTexId %u for address %p",
//                 //         registeredPalettes[i].glTexId, (void*)paletteAddr);

//                 glDeleteTextures(1, &registeredPalettes[i].glTexId);

//                 // Remove from registry (shift array down)
//                 for (uint32_t j = i; j < curTexIndex - 1; j++) {
//                     registeredPalettes[j] = registeredPalettes[j + 1];
//                     palettes[j] = palettes[j + 1];
//                 }
//                 curTexIndex--;
//                 break;
//             }
//         }
//     }
// }

// Add this hook before the existing glDeleteTextures call
so_hook D3DBaseTexture_Unregister_hook;
void D3DBaseTexture_Unregister(D3DBaseTexture *this, int param_1) {
    //logv_error("D3DBaseTexture_Unregister called with this: %p", this);

    // if (!this) {
    //     //log_error("D3DBaseTexture_Unregister: this is null, skipping cleanup");
    //     SO_CONTINUE(void*, D3DBaseTexture_Unregister_hook, this, param_1);
    //     return;
    // }

    // RegisteredTextureData *textureData = this->registeredTextureData;
    // //logv_error("D3DBaseTexture_Unregister: textureData: %p", textureData);

    // if (textureData && textureData->glTextureId != 0) {
    //     // Cleanup any associated palette textures
    //     cleanupPaletteForTexture(textureData);
    // }

    SO_CONTINUE(void*, D3DBaseTexture_Unregister_hook, this, param_1);
}

so_hook D3DDevice_UnregisterTextureCommand_hook;
void D3DDevice_UnregisterTextureCommand(void *this, RegisteredBaseTextureData *textureData, int *param_2) {
    //logv_error("D3DDevice_UnregisterTextureCommand called with this: %p, textureData: %p", this, textureData);

    // if (textureData && textureData->glTextureId != 0) {
    //     // Cast to RegisteredTextureData since cleanupPaletteForTexture expects that type
    //     RegisteredTextureData *regTexData = (RegisteredTextureData*)textureData;
    //     cleanupPaletteForTexture(regTexData);
    // }

    SO_CONTINUE(void *, D3DDevice_UnregisterTextureCommand_hook, this, textureData, param_2);
}

so_hook D3DBaseTexture_UnbufferToOGL_hook;
void D3DBaseTexture_UnbufferToOGL(D3DBaseTexture *this)
{
    // RegisteredTextureData *textureData = this->registeredTextureData;
    // //logv_error("D3DBaseTexture_UnbufferToOGL: textureData: %p", textureData);

    // if (textureData && textureData->glTextureId != 0) {
    //     // Cleanup any associated palette textures
    //     cleanupPaletteForTexture(textureData);
    // }

    SO_CONTINUE(void*, D3DBaseTexture_UnbufferToOGL_hook, this);
}

#endif