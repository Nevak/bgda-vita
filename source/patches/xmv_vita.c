#include "xmv_vita.h"

#include <vitasdk.h>
#include <vitaGL.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../utils/logger.h"
#include <so_util/so_util.h>
#include "utils/macros.h"

extern so_module so_mod_libxmv;
extern so_module so_mod;

so_hook sws_scale_hook_handle;
so_hook sws_getContext_hook_handle;
so_hook GetNextFrame_hook_handle;

int cur_video_width = 0;

static uint8_t *last_src[3];
static int last_srcStride[3];
static int last_height;
static uint8_t *prev_src0 = NULL;

void* sws_getContext_interceptor(int srcW, int srcH, int srcFormat, int dstW, int dstH, int dstFormat, int flags, void *srcFilter, void *dstFilter, double *param) {
    // Log sws_getContext calls for debugging
    void *ctx = SO_CONTINUE(void*, sws_getContext_hook_handle,
         srcW, srcH, srcFormat,
        dstW, dstH, dstFormat,
        flags, srcFilter, dstFilter, param);

    cur_video_width = srcW;

    logv_info("sws_getContext(src=%dx%d fmt=%d -> dst=%dx%d fmt=%d) called, ctx=%p",
              srcW, srcH, srcFormat, dstW, dstH, dstFormat, ctx);

    return ctx;
}

void sws_scale_interceptor(void *ctx, uint8_t **src, int *srcStride,
                           int srcSliceY, int srcSliceH,
                           uint8_t **dst, int *dstStride) {
    // Just save the pointers, don't copy anything
    last_src[0] = src[0]; last_src[1] = src[1]; last_src[2] = src[2];
    last_srcStride[0] = srcStride[0]; last_srcStride[1] = srcStride[1]; last_srcStride[2] = srcStride[2];
    last_height = srcSliceH;
}

int GetNextFrame_hooked(void *this, void *outputFrame, int param_2) {
    
    uint8_t *codec_ctx = *(uint8_t **)((uint8_t *)this + 0x1c);
    int *height_ptr = (int *)(codec_ctx + 0x80);
    int saved_height = *height_ptr;
    
    int ret = SO_CONTINUE(int, GetNextFrame_hook_handle, this, outputFrame, param_2);
    if (last_src[0] == prev_src0) {
        log_info("GetNextFrame: same src[0] as last time, skipping copy");
        return ret;
    }

    if (last_src[0] && last_height > 0) {
        int height = last_height;
        int yStride = last_srcStride[0];    // 672 or 880
        int uvStride = last_srcStride[1];   // 336 or 440
        int ySize = yStride * height;
        int uvSize = uvStride * (height / 2);
        
        uint8_t *out = (uint8_t *)outputFrame;
        sceDmacMemcpy(out,                    last_src[0], ySize);
        sceDmacMemcpy(out + ySize,            last_src[1], uvSize);
        sceDmacMemcpy(out + ySize + uvSize,   last_src[2], uvSize);
        
        //cur_yuv_stride = yStride;  // tell texture init to use this as width
        last_src[0] = NULL;
        // int width = cur_video_width;
        // int height = last_height;
        // int gxm_w = (width + 15) & ~15;
        // int gxm_stride_uv = ((gxm_w / 2) + 7) & ~7;
        
        // uint8_t *out = (uint8_t *)outputFrame;
        
        // // Y plane
        // for (int y = 0; y < height; y++)
        //     memcpy(out + y * gxm_w, last_src[0] + y * last_srcStride[0], width);
        
        // // U plane
        // uint8_t *uPlane = out + gxm_w * height;
        // for (int y = 0; y < height / 2; y++)
        //     memcpy(uPlane + y * gxm_stride_uv, last_src[1] + y * last_srcStride[1], width / 2);
        
        // // V plane
        // uint8_t *vPlane = uPlane + gxm_stride_uv * (height / 2);
        // for (int y = 0; y < height / 2; y++)
        //     memcpy(vPlane + y * gxm_stride_uv, last_src[2] + y * last_srcStride[2], width / 2);
    }
    
    return ret;
}

so_hook D3DDevice_SetVertexData2f_hook;
void D3DDevice_SetVertexData2f_hooked(int reg, float a, float b) {
    // get caller address
    uintptr_t caller = (uintptr_t)__builtin_return_address(0);
    //logv_debug("D3DDevice_SetVertexData2f called by %p with reg=%d, a=%f, b=%f", (void*)caller, reg, a, b);
    if (caller == LOC(0x000dd7fc) || caller == LOC(0x000dd82c)) {
        logv_debug("D3DDevice_SetVertexData2f called by %p with reg=%d, a=%f, b=%f", (void*)caller, reg, a, b);
        if (reg == 1 && a != 0.0f) {
            int hackedW = cur_video_width == 640 ? 672 : 880;

            a = (float)cur_video_width / (float)hackedW;
        }
    }
    SO_CONTINUE(void*, D3DDevice_SetVertexData2f_hook, reg, a, b);
}

void patch_xmv() {
    uintptr_t sws_scale_addr = (uintptr_t)so_symbol(&so_mod_libxmv, "sws_scale");
    uintptr_t sws_getContext_addr = (uintptr_t)so_symbol(&so_mod_libxmv, "sws_getContext");
    uintptr_t GetNextFrame_addr = (uintptr_t)so_symbol(&so_mod_libxmv, "_ZN11_XMVDecoder12GetNextFrameEPvi");
    uintptr_t D3DDevice_SetVertexData2f_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_SetVertexData2f");
    
    if (sws_scale_addr != 0) {
        sws_scale_hook_handle = hook_addr(sws_scale_addr, (uintptr_t)&sws_scale_interceptor);
        logv_info("Hooked sws_scale at %p", (void*)sws_scale_addr);
    } else {
        log_error("sws_scale not found in libxmv.so - YUV capture will fail");
    }

    if (sws_getContext_addr != 0) {
        sws_getContext_hook_handle = hook_addr(sws_getContext_addr, (uintptr_t)&sws_getContext_interceptor);
        logv_info("sws_getContext found at %p", (void*)sws_getContext_addr);
    } else {
        log_error("sws_getContext not found in libxmv.so");
    }

    if (GetNextFrame_addr != 0) {
        GetNextFrame_hook_handle = hook_addr(GetNextFrame_addr, (uintptr_t)&GetNextFrame_hooked);
        logv_info("GetNextFrame found at %p", (void*)GetNextFrame_addr);
    } else {
        log_error("GetNextFrame not found in libxmv.so");
    }

    if (D3DDevice_SetVertexData2f_addr != 0) {
        //D3DDevice_SetVertexData2f_hook = hook_addr(D3DDevice_SetVertexData2f_addr, (uintptr_t)&D3DDevice_SetVertexData2f_hooked);
        logv_info("D3DDevice_SetVertexData2f found at %p", (void*)D3DDevice_SetVertexData2f_addr);
    } else {
        log_error("D3DDevice_SetVertexData2f not found in main module");
    }

    // Force to skip the unnecessary copy in GetNextFrame
    uint32_t b_always = 0xea00000f;
    kuKernelCpuUnrestrictedMemcpy((void*)LOC_LIBXMV(0x0015b9a4), &b_always, 4);
}
