/*
 * patch.h
 *
 * Patching some of the .so internal functions or bridging them to native for
 * better compatibility.
 *
 * Copyright (C) 2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef SOLOADER_PATCH_H
#define SOLOADER_PATCH_H

#ifdef __cplusplus
extern "C" {
#endif

void so_patch();
void __aeabi_memcpy_patched(void *dst, const void *src, int n);
void __aeabi_memclr_patched(void *dst, int n);

static const char *gCmdLabels[256] = {
    [0x00] = "0x00 SetRenderTarget",
    [0x01] = "0x01 RegisterTexture",
    [0x02] = "0x02 BufferTexture",
    [0x03] = "0x03 GenerateMipmaps",
    [0x04] = "0x04 ResolveTexture",
    [0x05] = "0x05 UnregisterTexture",
    [0x06] = "0x06 RegisterSurface",
    [0x07] = "0x07 UnregisterSurface",
    [0x08] = "0x08 CompileShader",
    [0x09] = "0x09 ThreadSignal",
    [0x0A] = "0x0A IncrementFieldCounter",
    [0x0B] = "0x0B Clear",
    [0x0C] = "0x0C Swap",
    [0x0D] = "0x0D SwapToFront",
    [0x0E] = "0x0E BeginPrimitive",
    [0x0F] = "0x0F EndPrimitive",
    [0x10] = "0x10 SetVertexData4f",
    [0x11] = "0x11 SetVertexAttribute",
    [0x12] = "0x12 SetConstantMaybe",
    [0x13] = "0x13 UpdateShaderConstant",
    [0x14] = "0x14 BindResource",
    [0x15] = "0x15 SetRenderState",
    [0x16] = "0x16 SetVertexShaderInput",
    [0x17] = "0x17 StoreShaderState",
    [0x18] = "0x18 SetVertexShader",
    [0x19] = "0x19 CopyToSemaphore",
    [0x1A] = "0x1A SetVertexShaderAgain",
    [0x1B] = "0x1B SetPixelShaderProgram",
    [0x1C] = "0x1C SetPixelShaderConstant",
    [0x1D] = "0x1D LinkPixelShaderProgram",
    [0x1E] = "0x1E SetScissors",
    [0x1F] = "0x1F SetDepthRange",
    [0x20] = "0x20 RunDynamicPushBuffer",
    [0x21] = "0x21 DrawIndexedVertices", // 33
    [0x22] = "0x22 DrawVertices",
    [0x23] = "0x23 DrawVerticesUP",
    [0x24] = "0x24 DrawIndexedVerticesInstanced", // 36
    [0x25] = "0x25 DrawVerticesInstanced",
    [0x26] = "0x26 StoreVertexData",
    [0x27] = "0x27 StoreShaderParameters",
    [0x28] = "0x28 RegisterIndexBuffer",
    [0x29] = "0x29 BufferIndexBuffer",
    [0x2A] = "0x2A UnregisterIndexBuffer",
    [0x2B] = "0x2B RegisterVertexBuffer",
    [0x2C] = "0x2C BufferVertexBuffer",
    [0x2D] = "0x2D UnregisterVertexBuffer",
    [0x2E] = "0x2E BeginVisibilityTest",
    [0x2F] = "0x2F EndVisibilityTest",
    [0x30] = "0x30 GetVisibilityTestResult",
    [0x31] = "0x31 CallFunctionPointer",
    [0x32] = "0x32 UpdateLightingColor",
    [0x33] = "0x33 UpdatePalette",
    [0x34] = "0x34 DrawExtendedParams",
    [0x35] = "0x35 DrawExtendedParamsIndexed",
    [0x36] = "0x36 CopyMemoryBlockA",
    [0x37] = "0x37 CopyMemoryBlockB",
    [0x38] = "0x38 DrawExtendedParamsC",
    [0x39] = "0x39 SetViewport",
    [0x3A] = "0x3A StoreViewportState",
    [0x3B] = "0x3B UpdateViewportState",
    [0x3C] = "0x3C StoreViewportStateIndexed",
    [0x3D] = "0x3D ComputeAspectRatio",
    [0x3E] = "0x3E ResetRenderFlag",
    [0xFF] = "0xFF SpecialCommandOffset"
};


#ifdef __cplusplus
};
#endif

#endif // SOLOADER_PATCH_H
