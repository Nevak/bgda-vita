#ifndef BGDA_TYPES_H
#define BGDA_TYPES_H

#include <so_util/so_util.h>
#include <stdint.h>

typedef struct RegisteredTextureData RegisteredTextureData;
typedef struct D3DPalette D3DPalette;

typedef struct __attribute__((packed)) {
    uint32_t sWrapMode;
    uint32_t tWrapMode;
    uint32_t field2_0x8;
    int minFilterMode;
    int magFilterMode;
    uint32_t mipMapMode;
    uint32_t field6_0x18;
    uint32_t field7_0x1c;
    uint32_t lodBias;
    uint32_t field9_0x24;
    uint32_t field10_0x28;
    uint32_t field11_0x2c;
    uint32_t field12_0x30;
    uint32_t field13_0x34;
    uint32_t field14_0x38;
    uint32_t field15_0x3c;
    uint32_t field16_0x40;
    uint32_t field17_0x44;
    uint32_t field18_0x48;
    uint32_t field19_0x4c;
    uint32_t field20_0x50;
    uint32_t field21_0x54;
    uint32_t field22_0x58;
    uint32_t field23_0x5c;
    uint32_t field24_0x60;
    uint32_t field25_0x64;
    uint32_t field26_0x68;
    uint32_t field27_0x6c;
    uint32_t field28_0x70;
    uint32_t borderColorARGB;
    uint32_t field30_0x78;
    uint32_t field31_0x7c;
    uint8_t *palettePtr;
} TextureStageState;

_Static_assert(offsetof(TextureStageState, palettePtr) == 0x80, "`palettePtr` is not at the right offset - fix the struct definition");

typedef struct __attribute__((packed)) { /* PlaceHolder Structure */
    uint32_t field0_0x0;
    struct RegisteredTextureData *registeredTextureData;
    //void *registeredTextureData;
    int referenceCount;
    uint32_t field6_0xc;
    uint32_t dimensionsAndFlags;
} D3DBaseTexture;

typedef struct __attribute__((packed)) { /* PlaceHolder Structure */
    uint32_t field0_0x0;
    uint32_t field1_0x4;
    int glTargetType;
    D3DBaseTexture *baseTexturePtr;
    uint32_t field4_0x10;
    uint32_t field5_0x14;
    uint32_t field6_0x18;
    uint32_t field7_0x1c;
    uint32_t glTextureId;
} RegisteredBaseTextureData;

struct __attribute__((packed)) RegisteredTextureData { /* PlaceHolder Structure */
    uint16_t field0_0x0;
    uint16_t field1_0x2;
    uint16_t someField_2;
    uint16_t someField_1;
    int glTarget;
    D3DBaseTexture *d3dBaseTexture;
    uint8_t unk_bytes[4];
    uint32_t width; /* Created by retype action */
    uint32_t height; /* Created by retype action */
    uint32_t field9_0x1c;
    uint32_t glTextureId;
    uint32_t field11_0x24;
    uint32_t *rawDataPtr;
    TextureStageState* state;
    uint32_t wrapModeU;
    uint32_t field15_0x34;
    uint32_t minFilter;
    uint32_t field17_0x3c;
    uint32_t maxFilter;
    uint32_t field19_0x44;
    uint32_t field20_0x48;
    uint32_t borderColor; /* Created by retype action */
    uint32_t field22_0x50;
    uint32_t field23_0x54;
    uint32_t field24_0x58;
    uint32_t field25_0x5c;
    uint32_t field26_0x60;
    uint32_t field27_0x64;
    uint32_t field28_0x68;
    uint32_t field29_0x6c;
    uint32_t field30_0x70;
    uint32_t field31_0x74;
    uint32_t field32_0x78;
    uint32_t field33_0x7c;
    uint32_t field34_0x80;
    uint32_t field35_0x84;
    uint32_t field36_0x88;
    uint32_t field37_0x8c;
    uint32_t field38_0x90;
    uint32_t field39_0x94;
    uint32_t field40_0x98;
    uint32_t field41_0x9c;
    uint32_t lodBias; /* Created by retype action */
    uint32_t field43_0xa4;
    uint32_t field44_0xa8;
    uint32_t field45_0xac;
    uint32_t stateChangeMask;
    uint32_t lastFrameUsed;
    uint32_t samplerType;
};

_Static_assert(offsetof(RegisteredTextureData, state) == 0x2c,
               "`state` is not at the right offset - fix the struct or add packed!");

typedef struct __attribute__((packed)) {
    TextureStageState state;
    // TextureStageState* state;
    // uint32_t wrapModeU; /* Created by retype action */
    // uint32_t wrapModeV;
    // uint32_t minFilter; /* Created by retype action */
    // uint32_t field4_0x10;
    // uint32_t magFilter; /* Created by retype action */
    // uint32_t field6_0x18;
    // uint32_t field7_0x1c;
    // uint32_t borderColor; /* Created by retype action */
    // uint32_t field9_0x24;
    // uint32_t field10_0x28;
    // uint32_t field11_0x2c;
    // uint32_t field12_0x30;
    // uint32_t field13_0x34;
    // uint32_t field14_0x38;
    // uint32_t field15_0x3c;
    // uint32_t field16_0x40;
    // uint32_t field17_0x44;
    // uint32_t field18_0x48;
    // uint32_t field19_0x4c;
    // uint32_t field20_0x50;
    // uint32_t field21_0x54;
    // uint32_t field22_0x58;
    // uint32_t field23_0x5c;
    // uint32_t field24_0x60;
    // uint32_t field25_0x64;
    // uint32_t field26_0x68;
    // uint32_t field27_0x6c;
    // uint32_t field28_0x70;
    // float lodBias; /* Created by retype action */
    // uint32_t field30_0x78;
    // uint32_t field31_0x7c;
    // uint32_t field32_0x80;
    RegisteredTextureData *registeredTexturePtr; /* Created by retype action */
} D3DDevice_TextureStageState;

_Static_assert(offsetof(D3DDevice_TextureStageState, registeredTexturePtr) == 0x84,
               "`registeredTexturePtr` is not at the ritght offset - fix the struct or add packed!");

_Static_assert(offsetof(TextureStageState, lodBias) == 0x20,
               "`lodBias` is not at the right offset - fix the struct or add packed!");

_Static_assert(offsetof(TextureStageState, palettePtr) == 0x80,
               "`registeredTexturePtr` is not at the ritght offset - fix the struct or add packed!");



#endif