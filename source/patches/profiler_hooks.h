#ifndef PROFILER_HOOKS_H
#define PROFILER_HOOKS_H
#ifdef PROFILER_ENABLED
#include <so_util/so_util.h>
#include "utils/prof.h"

PROF_HOOK_VOID(gameDrawWorld, "_Z13gameDrawWorldv", (void))
PROF_HOOK_VOID(worldDoDelayDrawTask, "_Z20worldDoDelayDrawTaskv", (void))
PROF_HOOK_VOID(EndFrameFence, "_ZN3JBE9DisplayPF13EndFrameFenceEv", (int* param_1), (param_1));
PROF_HOOK_VOID(SystemUpdate, "_ZN3JBE6System6UpdateEv", (void));
PROF_HOOK_VOID(SwapToFront, "_ZN3JBE9D3DDevice11SwapToFrontEi", (int* param_1), (param_1));
PROF_HOOK_VOID(DisplaySwap, "_ZN3JBE9DisplayPF4SwapEv", (void *param_1), (param_1));
PROF_HOOK_VOID(D3DDevice_CommitState, "_ZN3JBE9D3DDevice11CommitStateEv", (void *param_1), (param_1));
PROF_HOOK_VOID(Blit, "_ZN3JBE9DisplayPF4BlitEiiiiRKNS_13ShaderProgramEi", (int* param_1, int param_2, int param_3, int param_4, void *param_5, int param_6), (param_1, param_2, param_3, param_4, param_5, param_6));
PROF_HOOK_VOID(D3DDevice_UpdateComboStates, "_ZN3JBE9D3DDevice17UpdateComboStatesEv", (int *param_1), (param_1));
PROF_HOOK_VOID(D3DDevice_SetVertexShaderInputDirect, "_ZN3JBE9D3DDevice26SetVertexShaderInputDirectE", (int *thisptr, int param_2, int param_3, int param_4), (thisptr, param_2, param_3, param_4));
PROF_HOOK_VOID(D3DBaseTexture_BufferToOGL, "_ZN14D3DBaseTexture11BufferToOGLEP21RegisteredTextureDataPKvi", (void *thisptr, void* param_1, void const* param_2, int param_3), thisptr, param_1, param_2, param_3);
PROF_HOOK_VOID(Squish_DecompressImage, "_ZN6squish15DecompressImageEPhiiPKvi", (unsigned char *pDst, int width, int height, const void *pSrc, int flags), pDst, width, height, pSrc, flags);
PROF_HOOK_VOID(DecompressBlockAlpha, "_Z20decompressBlockAlphaPhS_iiii", (unsigned char *pDst, unsigned char *pSrc, int width, int height, int flags), pDst, pSrc, width, height, flags);
PROF_HOOK_VOID(XGUnswizzleRect_NOTXDK, "XGUnswizzleRect_NOTXDK", (int param_1, uint32_t param_2, uint32_t param_3, uint32_t param_4, int param_5, int param_6, int param_7), param_1, param_2, param_3, param_4, param_5, param_6, param_7);
PROF_HOOK_VOID(JBE_D3DDevice_TextureStageState_ManyParams, "_ZN3JBE9D3DDevice17TextureStageState7SetToGLEmN13XGSamplerType4EnumE", (uint8_t *param_1, uint32_t param_2, uint32_t param_3), param_1, param_2, param_3);
PROF_HOOK_VOID(TextureStageState_SetToGL, "_ZN17TextureStageState7SetToGLEmP25RegisteredBaseTextureDataN13XGSamplerType4EnumE", (uint8_t *param_1, uint32_t param_2, uint32_t param_3), param_1, param_2, param_3);
PROF_HOOK_VOID(JBE_ThreadSleep, "_ZN3JBE6Thread5SleepEj", (unsigned int param_1), param_1);

void install_prof_hooks(void) {
	PROF_ATTACH(gameDrawWorld, "_Z13gameDrawWorldv");
	PROF_ATTACH(worldDoDelayDrawTask, "_Z20worldDoDelayDrawTaskv");
	PROF_ATTACH(EndFrameFence, "_ZN3JBE9DisplayPF13EndFrameFenceEv");
	PROF_ATTACH(SystemUpdate, "_ZN3JBE6System6UpdateEv");
	PROF_ATTACH(SwapToFront, "_ZN3JBE9D3DDevice11SwapToFrontEi");
	PROF_ATTACH(DisplaySwap, "_ZN3JBE9DisplayPF4SwapEv");
	PROF_ATTACH(Blit, "_ZN3JBE9DisplayPF4BlitEiiiiRKNS_13ShaderProgramEi");
	PROF_ATTACH(D3DDevice_CommitState, "_ZN3JBE9D3DDevice11CommitStateEv");
	PROF_ATTACH(D3DBaseTexture_BufferToOGL, "_ZN14D3DBaseTexture11BufferToOGLEP21RegisteredTextureDataPKvi");
	PROF_ATTACH(Squish_DecompressImage, "_ZN6squish15DecompressImageEPhiiPKvi");
	PROF_ATTACH(DecompressBlockAlpha, "_Z20decompressBlockAlphaPhS_iiii");
	PROF_ATTACH(XGUnswizzleRect_NOTXDK, "XGUnswizzleRect_NOTXDK");
	PROF_ATTACH(TextureStageState_SetToGL, "_ZN3JBE9D3DDevice17TextureStageState7SetToGLEmN13XGSamplerType4EnumE");
	PROF_ATTACH(JBE_ThreadSleep, "_ZN3JBE6Thread5SleepEj");
}

#else
// Stub function when profiling is disabled
void install_prof_hooks(void) {
	// No-op when profiling is disabled
}
#endif
#endif