#ifndef TEXTURE_PALETTE_H
#define TEXTURE_PALETTE_H

#include "bgda_types.h"
#include <so_util/so_util.h>
#include <string.h>
#define GL_TEXTURE_MAX_LEVEL 0x813d

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

typedef struct textureUniformState {
	uint32_t glTexId;
	int usePalette;  // 0 = no palette, 1 = use palette
} textureUniformState;

textureUniformState textureStates[4096];
uint32_t textureStateCount = 0;

uint32_t curTexIndex = 0;
registeredPalette registeredPalettes[4096];

typedef struct {
	unsigned char colors[256][4];
} PaletteStruct;

// Array of 1024 palettes
PaletteStruct palettes[2048];

void registerPalette(uint8_t* paletteAddr, uint32_t glTexId) {
    if (curTexIndex < 2048) {
        uint8_t* actualPaletteAddr = (uint8_t*)((uint32_t)paletteAddr & 0xfffffffe);

        registeredPalettes[curTexIndex].paletteAddr = paletteAddr;
        registeredPalettes[curTexIndex].glTexId = glTexId;

        // DEBUG: Log first few palette entries to see what colors we're getting
        // logv_error("=== Palette %u (glTexId %u) first 8 colors ===", curTexIndex, glTexId);
        // for (int i = 0; i < 8; i++) {
        //     logv_error("Color %d: R=%02X G=%02X B=%02X A=%02X", i,
        //         actualPaletteAddr[i*4+0], actualPaletteAddr[i*4+1],
        //         actualPaletteAddr[i*4+2], actualPaletteAddr[i*4+3]);
        // }

        // Swap R and B channels while copying
        for (int i = 0; i < 256; i++) {
            palettes[curTexIndex].colors[i][0] = actualPaletteAddr[i*4 + 2]; // R = source B
            palettes[curTexIndex].colors[i][1] = actualPaletteAddr[i*4 + 1]; // G = source G
            palettes[curTexIndex].colors[i][2] = actualPaletteAddr[i*4 + 0]; // B = source R
            palettes[curTexIndex].colors[i][3] = actualPaletteAddr[i*4 + 3]; // A = source A
        }

        glBindTexture(GL_TEXTURE_2D, glTexId);
        glTexImage2D_fake(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, palettes[curTexIndex].colors);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
        curTexIndex++;
    }
	else {
		log_error("registerPalette: exceeded max registered palettes\n");
	}
}

int isPaletteRegistered(uint32_t paletteAddr) {
    uint32_t maskedAddr = paletteAddr & 0xfffffffe;  // Mask the input too
    for (uint32_t i = 0; i < curTexIndex; i++) {
        uint32_t registeredMasked = (uint32_t)registeredPalettes[i].paletteAddr & 0xfffffffe;
        if (registeredMasked == maskedAddr) {
            return registeredPalettes[i].glTexId;
        }
    }
    return 0;
}

void setTextureUniformState(uint32_t glTexId, int usePalette) {
    // Check if texture already has state stored
    for (uint32_t i = 0; i < textureStateCount; i++) {
        if (textureStates[i].glTexId == glTexId) {
            textureStates[i].usePalette = usePalette;
            return;
        }
    }

    // Add new texture state
    if (textureStateCount < 4096) {
        textureStates[textureStateCount].glTexId = glTexId;
        textureStates[textureStateCount].usePalette = usePalette;
        textureStateCount++;
    }
}

int getTextureUniformState(uint32_t glTexId) {
    for (uint32_t i = 0; i < textureStateCount; i++) {
        if (textureStates[i].glTexId == glTexId) {
            return textureStates[i].usePalette;
        }
    }
    return 0; // Default: no palette
}

void removeTextureUniformState(uint32_t glTexId) {
    for (uint32_t i = 0; i < textureStateCount; i++) {
        if (textureStates[i].glTexId == glTexId) {
            // Remove by shifting array down
            for (uint32_t j = i; j < textureStateCount - 1; j++) {
                textureStates[j] = textureStates[j + 1];
            }
            textureStateCount--;
            break;
        }
    }
}

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

    // Get current texture ID to store uniform state
    GLint currentTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);

    int usePalette = 0;
    if ((formatToSwitchParam & 0xffffff7f) == 0xb && palette)
	{
        usePalette = 1;

        if (width == 1024 && height == 1024) {
            log_error("1024 found WITH PALETTE, disabling palette");
            usePalette = 0;
        }

        uint8_t* textureDataToUpload = sourceTextureData;

        if (width != sourcePitch && width < 64) {
            // Create a tightly packed buffer
            uint8_t* packedBuffer = (uint8_t*)malloc(width * height);
            for (int y = 0; y < height; y++) {
                memcpy(packedBuffer + y * width,
                       sourceTextureData + y * sourcePitch,
                       width);
            }
            textureDataToUpload = packedBuffer;
        }

		// store the currently active texture
		GLint currentActiveTexUnit;
		glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&currentActiveTexUnit);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexParameteri(glTarget,GL_TEXTURE_MAX_LEVEL,0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// For index texture
		glTexImage2D_fake(glTarget,
			0, // level
			GL_LUMINANCE, //GL_RGBA,
			width, // whatever original width was passed to DoTheFinalGPUUpload
			height, // whatever original height
			0, // border
			GL_LUMINANCE, //GL_RGBA, // format = internalFormat
			GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, // type ?
			textureDataToUpload); // data, comes from the function args

 		glTexParameteri(glTarget,GL_TEXTURE_MAX_LEVEL,0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		if (palette != NULL && allocateNewTexture) 
        {
			glActiveTexture(GL_TEXTURE6);
            
			uint32_t paletteId = isPaletteRegistered((uint32_t)palette);
			if (paletteId == 0)
            {
				glGenTextures(1, &paletteId);
				registerPalette(palette, paletteId);
                glBindTexture(GL_TEXTURE_2D, paletteId);
			}

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // defensive; rows are 256*4 = 1024 (already aligned)

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            GLint prog = 0;
            glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
            GLint loc = glGetUniformLocation(prog, "uPalette");
            if (loc >= 0)
            {
                glUniform1i(loc, 6);
            }

			glActiveTexture(currentActiveTexUnit);
		}

        // Store the uniform state for this texture
        setTextureUniformState(currentTexture, usePalette);
        //logv_error("Stored palette state for texture %d: usePalette=%d", currentTexture, usePalette);

        // Clean up the packed buffer if we created one
        if (textureDataToUpload != sourceTextureData) {
            free(textureDataToUpload);
        }

		return;
	}

    // Store non-palette texture state
    setTextureUniformState(currentTexture, usePalette);
    logv_error("Stored non-palette state for texture %d: usePalette=%d", currentTexture, usePalette);

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

void cleanupPaletteForTexture(RegisteredTextureData* textureData) {
    //logv_error("cleanupPaletteCalled: textureData: %p", textureData);

    if (!textureData) {
        //log_error("cleanupPaletteForTexture: textureData is null, skipping");
        return;
    }

    // Fix: Take address of state, don't dereference it
    TextureStageState* textureStageState = &textureData->state;
    //logv_error("cleanupPaletteCalled: textureStageState: %p", textureStageState);

    if (textureStageState && textureStageState->palettePtr) {
        uint32_t paletteAddr = (uint32_t)textureStageState->palettePtr;

        for (uint32_t i = 0; i < curTexIndex; i++) {
            if (((uint32_t)registeredPalettes[i].paletteAddr & 0xfffffffe) ==
                (paletteAddr & 0xfffffffe)) {

                // logv_error("Cleaning up palette glTexId %u for address %p",
                //         registeredPalettes[i].glTexId, (void*)paletteAddr);

                glDeleteTextures(1, &registeredPalettes[i].glTexId);

                // Remove from registry (shift array down)
                for (uint32_t j = i; j < curTexIndex - 1; j++) {
                    registeredPalettes[j] = registeredPalettes[j + 1];
                    palettes[j] = palettes[j + 1];
                }
                curTexIndex--;
                break;
            }
        }
    }
}

// Add this hook before the existing glDeleteTextures call
so_hook D3DBaseTexture_Unregister_hook;
void D3DBaseTexture_Unregister(D3DBaseTexture *this, int param_1) {
    //logv_error("D3DBaseTexture_Unregister called with this: %p", this);

    if (!this) {
        //log_error("D3DBaseTexture_Unregister: this is null, skipping cleanup");
        SO_CONTINUE(void*, D3DBaseTexture_Unregister_hook, this, param_1);
        return;
    }

    RegisteredTextureData *textureData = this->registeredTextureData;
    //logv_error("D3DBaseTexture_Unregister: textureData: %p", textureData);

    if (textureData && textureData->glTextureId != 0) {
        // Cleanup any associated palette textures
        cleanupPaletteForTexture(textureData);
    }

    SO_CONTINUE(void*, D3DBaseTexture_Unregister_hook, this, param_1);
}

so_hook D3DDevice_UnregisterTextureCommand_hook;
void D3DDevice_UnregisterTextureCommand(void *this, RegisteredBaseTextureData *textureData, int *param_2) {
    //logv_error("D3DDevice_UnregisterTextureCommand called with this: %p, textureData: %p", this, textureData);

    if (textureData && textureData->glTextureId != 0) {
        // Cast to RegisteredTextureData since cleanupPaletteForTexture expects that type
        RegisteredTextureData *regTexData = (RegisteredTextureData*)textureData;
        cleanupPaletteForTexture(regTexData);
    }

    SO_CONTINUE(void *, D3DDevice_UnregisterTextureCommand_hook, this, textureData, param_2);
}

so_hook D3DBaseTexture_UnbufferToOGL_hook;
void D3DBaseTexture_UnbufferToOGL(D3DBaseTexture *this)
{
    RegisteredTextureData *textureData = this->registeredTextureData;
    //logv_error("D3DBaseTexture_UnbufferToOGL: textureData: %p", textureData);

    if (textureData && textureData->glTextureId != 0) {
        // Cleanup any associated palette textures
        cleanupPaletteForTexture(textureData);
    }

    SO_CONTINUE(void*, D3DBaseTexture_UnbufferToOGL_hook, this);
}

#endif