/*
 * patch.c
 *
 * Patching some of the .so internal functions or bridging them to native for
 * better compatibility.
 *
 * Copyright (C) 2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "patch.h"

#include <kubridge.h>
#include <so_util/so_util.h>
#include <utils/trophies.h>
#include <stdio.h>
#include <vitasdk.h>

#ifdef __cplusplus
extern "C" {
#endif
extern so_module so_mod;
#ifdef __cplusplus
};
#endif

#include "utils/logger.h"
#include <stdbool.h>

so_hook achieve_hook, stage_hook, takamatsu_hook, takamatsu2_hook, lumpLoad_hook, memPrintFree_hook, listInit_hook;


int setAchieve(void *this, int id, int unlock) {
	printf("setAchieve(%i, %i)\n", id, unlock);
	trophies_unlock(id + 1);
	
	return SO_CONTINUE(int, achieve_hook, this, id, unlock);
}

int ret0() { return 0; }
int ret1() { return 1; }

so_hook _machInit_hook;

void exit_process() {
	sceKernelExitProcess(0);
}

int lumpLoad(char *param_1) {
	printf("lumpLoad(%s)\n", param_1);
	return SO_CONTINUE(int, lumpLoad_hook, param_1);
}

void machInit() {
	printf("machInit()\n");
	return SO_CONTINUE(void *, _machInit_hook);
}

// void createFile(char* param_1, int param_2) {
// 	printf("createFile()\n");
// 	return SO_CONTINUE(void *, _createFile_hook, param_1, param_2);
// }

//MC_Init
so_hook MC_Init_hook;
int MC_Init(int param_1) {
	logv_error("MC_Init(%i)\n", param_1);
	int returnval = SO_CONTINUE(int, MC_Init_hook, param_1);
	logv_error("MC_Init returned %i", returnval);
	return returnval;
}

//coreAddTask
so_hook coreAddTask_hook;
int coreAddTask(void *param_1, int param_2, char *param_3) {
	logv_error("coreAddTask(%p, %i, %s)", param_1, param_2, param_3);
	int returnval = SO_CONTINUE(int, coreAddTask_hook, param_1, param_2, param_3);
	//logv_error("coreAddTask returned %i\n", returnval);
	return returnval;
}

//XInitCloud
so_hook XInitCloud_hook;
int XInitCloud(int param_1, void *param_2, int param_3, void *param_4, int param_5) {
	logv_error("XInitCloud(%i, %p, %i, %p, %i)", param_1, param_2, param_3, param_4, param_5);
	int returnval = SO_CONTINUE(int, XInitCloud_hook, param_1, param_2, param_3, param_4, param_5);
	logv_error("XInitCloud returned %i", returnval);
	return returnval;
}

// ShaderManager_LoadProgram
so_hook ShaderManager_LoadProgram_hook;
/* JBE::ShaderManager::LoadProgram(JBE::ShaderProgram&, JBE::ShaderManager::VertexDef const&, int,
   JBE::ShaderManager::PixelDef const&, unsigned int, int
   (*)(JBE::Container<JBE::Util::AlignedPtr<char const> >::Iterator&)) */
void ShaderManager_LoadProgram(void *thisptr, void *param_1, int param_2, void *param_3, unsigned int param_4, int (*param_5)(void *)) {
	logv_error("ShaderManager_LoadProgram(%p, %p, %i, %p, %u, %p)\n", thisptr, param_1, param_2, param_3, param_4, param_5);

	//uVar4 = *(uint *)(param_4 + 0x20);
	int uVar4 = *(int *)((int)param_3 + 0x20);
	logv_error("uVar4: %i\n", uVar4);

	SO_CONTINUE(int, ShaderManager_LoadProgram_hook, thisptr, param_1, param_2, param_3, param_4, param_5);
	log_error("ShaderManager_LoadProgram returned");
}

// gameLoop
so_hook gameLoop_hook;
void gameLoop() {
	log_error("gameLoop()");
	SO_CONTINUE(void *, gameLoop_hook);
	log_error("gameLoop returned");
}

// machMpegLoop
so_hook machMpegLoop_hook;


void machMpegLoop(char *param_1,char *param_2,void *param_3,int param_4,char *param_5,
	char *param_6,int param_7,bool param_8,bool param_9)
	{
		logv_error("machMpegLoop(%s)\n", param_5);
		SO_CONTINUE(void *, machMpegLoop_hook, param_1, param_2, param_3, param_4, param_5, param_6, param_7, param_8);
		log_error("machMpegLoop returned\n");	
	}

so_hook XMVDecoder_CreateDecoderForFile_hook;
int XMVDecoder_CreateDecoderForFile(int param_1, char * fileName, int param_3) {
	logv_error("XMVDecoder_CreateDecoderForFile(%s)\n", fileName);
	int returnval = SO_CONTINUE(int, XMVDecoder_CreateDecoderForFile_hook, param_1, fileName, param_3);
	logv_error("XMVDecoder_CreateDecoderForFile returned %i\n", returnval);
	return returnval;
}

so_hook jbe_android_main_sub_hook;
void JBE_android_main_sub(void *param_1) {
	log_error("JBE_android_main_sub() entered\n");
	SO_CONTINUE(void *, jbe_android_main_sub_hook, param_1);
	log_error("JBE_android_main_sub returned\n");
}

so_hook loader_load_hook;
void loader_load(void *param_1, char *param_2, void (*param_3)(void *), void *param_4, void *param_5, int param_6, void *param_7, int *param_8) {
	logv_error("loader_load(%s)\n", param_2);
	SO_CONTINUE(void *, loader_load_hook, param_1, param_2, param_3, param_4, param_5, param_6, param_7, param_8);
	log_error("loader_load returned\n");
}

so_hook lumpFindResource_hook;
int lumpFindResource(char *param_1, char *param_2) {
	logv_error("lumpFindResource(%s, %s)\n", param_1, param_2);
	return SO_CONTINUE(int, lumpFindResource_hook, param_1, param_2);
	//log_error("lumpFindResource returned\n");
}

so_hook pvr_error_output_debug_hook;
void pvr_error_output_debug(char *param_1, ...) {
	// log all the parameters
	logv_error("pvr_error_output_debug(%s)\n", param_1);
	SO_CONTINUE(void *, pvr_error_output_debug_hook, param_1);
	log_error("pvr_error_output_debug returned\n");
}

so_hook pvr_texture_load_from_pointer_hook;
int pvr_texture_load_from_pointer(void *param_1, void *param_2, void *param_3, unsigned int param_4, void *param_5) {
	logv_error("pvr_texture_load_from_pointer(%p, %p, %p, %u, %p)\n", param_1, param_2, param_3, param_4, param_5);
	int returnval = SO_CONTINUE(int, pvr_texture_load_from_pointer_hook, param_1, param_2, param_3, param_4, param_5);
	logv_error("pvr_texture_load_from_pointer returned %i\n", returnval);
	return returnval;
}


so_hook d3d_base_texture_get_info_hook;
void d3d_base_texture_get_info(void *thisptr, void *pFormat, int *param_2, void *param_3, int *param_4, int *param_5) {
	//logv_error("d3d_base_texture_get_info(%p, %p, %p, %p, %p)\n", thisptr, pFormat, param_2, param_3, param_4);
	SO_CONTINUE(void *, d3d_base_texture_get_info_hook, thisptr, pFormat, param_2, param_3, param_4, param_5);
	// print the content of pFormat
	logv_error("\t d3d_base_texture_get_info format: 0x%x\n", *(int *)pFormat);
	//log_error("d3d_base_texture_get_info returned\n");
}

so_hook xg_is_swizzled_format_hook;
bool xg_is_swizzled_format(int param_1) {
	logv_error("xg_is_swizzled_format(0x%x)\n", param_1);
	bool returnval = SO_CONTINUE(bool, xg_is_swizzled_format_hook, param_1);
	logv_error("xg_is_swizzled_format returned %i\n", returnval);
	return returnval;
}

so_hook xg_is_compressed_format_hook;
bool xg_is_compressed_format(int param_1) {
	logv_error("xg_is_compressed_format(0x%x)\n", param_1);
	bool returnval = SO_CONTINUE(bool, xg_is_compressed_format_hook, param_1);
	logv_error("xg_is_compressed_format returned %i\n", returnval);
	return returnval;
}

so_hook xg_bytes_per_pixel_from_format_hook;
int xg_bytes_per_pixel_from_format(int param_1) {
	logv_error("xg_bytes_per_pixel_from_format(0x%x)\n", param_1);
	int returnval = SO_CONTINUE(int, xg_bytes_per_pixel_from_format_hook, param_1);
	logv_error("xg_bytes_per_pixel_from_format returned %i\n", returnval);
	return returnval;
}

so_hook texProcessDecompress_hook;
void texProcessDecompress() {
	log_error("texProcessDecompress()\n");
	SO_CONTINUE(void *, texProcessDecompress_hook);
	log_error("texProcessDecompress returned\n");
}

so_hook pixel_program_data_init_hook;
void pixel_program_data_init(void *thisptr, void *param_1, unsigned int param_2) {
	logv_error("pixel_program_data_init(%p, %p, %u)\n", thisptr, param_1, param_2);

	int iVar4 = *(int *)(param_1 + 0x1c);
	int iVar1 = iVar4;
	// Log both variables
	logv_error("iVar4: %i\n", iVar4);
	logv_error("iVar1: %i\n", iVar1);

	char ***ppcVar6;
	char **ppcVar7;
	if (iVar4 < 0) {	
	  	iVar1 = -iVar4;
	}
	if (iVar4 != 0) {
		ppcVar6 = *(char ***)(param_1 + 0x24);
		uint uVar8 = *(uint *)(param_1 + 0x2c);
		ppcVar7 = ppcVar6;
		
		do {
			// Print a message
			// Print the content of ppcVar7 as a string
			logv_error("pixel_program_data_init: ppcVar7: %s\n", *ppcVar7);
			logv_error("pixel_program_data_init: uVar8: %u\n", uVar8);
			if ((uVar8 & 1) == 0) {
				log_error("pixel_program_data_init: uVar8 & 1 == 0\n");
			}
			else {
				log_error("pixel_program_data_init: uVar8 & 1 != 0\n");
			}

			ppcVar7 = ppcVar7 + 2;
			uVar8 = uVar8 >> 1;
		} while (ppcVar7 != ppcVar6 + iVar1 * 2);
	}

	if (param_2 == 2)
	{
		
		// Write 1 to the last bit of param_1 + 0x1c
		//*(int *)(param_1 + 0x2c) = iVar4 | 1;	
	}
	
	SO_CONTINUE(void *, pixel_program_data_init_hook, thisptr, param_1, param_2);
	log_error("pixel_program_data_init returned\n");
}

so_hook stage_shader_program_hook;
void stage_shader_program(void *thisptr, void *param_1, void *param_2, unsigned int param_3) {
	logv_error("StageShaderProgram<>::Compile(%p, %p, %p, %u)\n", thisptr, param_1, param_2, param_3);
	SO_CONTINUE(void *, stage_shader_program_hook, thisptr, param_1, param_2, param_3);
	log_error("StageShaderProgram<>::Compile returned\n");
}

so_hook xg_set_program_hook;
void xg_set_program(void *param_1, void *param_2, void *param_3) {
	logv_error("XGSetProgram(%p, %p, %p)\n", param_1, param_2, param_3);
	SO_CONTINUE(void *, xg_set_program_hook, param_1, param_2, param_3);
	log_error("XGSetProgram returned\n");
}

so_hook xg_link_program_hook;
void xg_link_program(void *param_1, void *param_2, void *param_3) {
	logv_error("XGLinkProgram(%p, %p, %p)\n", param_1, param_2, param_3);
	SO_CONTINUE(void *, xg_link_program_hook, param_1, param_2, param_3);
	log_error("XGLinkProgram returned\n");
}

so_hook isES31_hook;
int isES31() {
	log_error("isES31()\n");
	int ret = SO_CONTINUE(int, isES31_hook);
	logv_error("isES31 returned value %i\n", ret);
	return ret;
}

so_hook isES3_hook;
int isES3() {
	log_error("isES3()\n");
	int ret = SO_CONTINUE(int, isES3_hook);
	logv_error("isES3 returned value %i\n", ret);
	return ret;
}

so_hook d3d_texture_stage_state_set_to_gl_hook;
void d3d_texture_stage_state_set_to_gl(void *thisptr, unsigned int param_1, unsigned int param_2, unsigned int param_3) {
	logv_error("d3d_texture_stage_state_set_to_gl(%p, %u, %u, %u)\n", thisptr, param_1, param_2, param_3);
	SO_CONTINUE(void *, d3d_texture_stage_state_set_to_gl_hook, thisptr, param_1, param_2, param_3);
	log_error("d3d_texture_stage_state_set_to_gl returned\n");
}

so_hook d3d_resolve_msaa_hook;
void d3d_resolve_msaa(void *thisptr, int param_1) {
	logv_error("d3d_resolve_msaa(%p, %i)\n", thisptr, param_1);
	SO_CONTINUE(void *, d3d_resolve_msaa_hook, thisptr, param_1);
	log_error("d3d_resolve_msaa returned\n");
}

so_hook d3d_buffer_to_ogl_hook;
void d3d_buffer_to_ogl(void *thisptr, void *param_1, void *param_2, int param_3) {
	logv_error("d3d_buffer_to_ogl(%p, %p, %p, %i)\n", thisptr, param_1, param_2, param_3);
	SO_CONTINUE(void *, d3d_buffer_to_ogl_hook, thisptr, param_1, param_2, param_3);
	log_error("d3d_buffer_to_ogl returned\n");
}

so_hook swap_hook;
void swap(void *thisptr, int param_1) {
	logv_error("swap(%p, %i)\n", thisptr, param_1);
	SO_CONTINUE(void *, swap_hook, thisptr, param_1);
	log_error("swap returned\n");
}

so_hook pixel_stage_shader_ctr_hook;
void pixel_stage_shader_ctr(void *thisptr, char *param_1) {
	logv_error("PixelStageShader::PixelStageShader(%p, %s)\n", thisptr, param_1);
	// Print the param_1 as a string
	logv_error("PixelStageShader::PixelStageShader: source: %s\n", param_1);
	SO_CONTINUE(void *, pixel_stage_shader_ctr_hook, thisptr, param_1);
	// print the value of thisptr + 0x2c as hex
	logv_error("PixelStageShader::PixelStageShader: thisptr + 0x2c: 0x%x\n", *(int *)((int)thisptr + 0x2c));
	log_error("PixelStageShader::PixelStageShader returned\n");
}

void so_patch(void) {

	log_error("Patching .so functions\n");

	
	

	//_ZN3EXT6IsES31Ev
	//undefined4 EXT::IsES31(void)
	//isES31_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN3EXT6IsES31Ev"), (uintptr_t)&isES31);

	//_ZN3EXT5IsES3Ev
	//undefined4 EXT::IsES3(void)
	//isES3_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN3EXT5IsES3Ev"), (uintptr_t)&isES3);

	// _Z7MC_Initi MC_Init
	//MC_Init_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z7MC_Initi"), (uintptr_t)&MC_Init);

	// _ZN16PixelStageShaderC1EPKc
	//pixel_stage_shader_ctr_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN16PixelStageShaderC1EPKc"), (uintptr_t)&pixel_stage_shader_ctr);

	// _Z11coreAddTaskPFvvEiPKc coreAddTask
	coreAddTask_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z11coreAddTaskPFvvEiPKc"), (uintptr_t)&coreAddTask);

	// _ZN3JBE9D3DDevice11SwapToFrontEi
	//swap_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice11SwapToFrontEi"), (uintptr_t)&swap);

	// _ZN14D3DBaseTexture11BufferToOGLEP21RegisteredTextureDataPKvi
	uintptr_t d3d_buffer_to_ogl_addr = (uintptr_t)so_symbol(&so_mod, "_ZN14D3DBaseTexture11BufferToOGLEP21RegisteredTextureDataPKvi");
	if (d3d_buffer_to_ogl_addr == NULL) {
		log_error("d3d_buffer_to_ogl not found\n");
	} else {
		logv_error("d3d_buffer_to_ogl found at %p\n", d3d_buffer_to_ogl_addr);
		//d3d_buffer_to_ogl_hook = hook_addr(d3d_buffer_to_ogl_addr, (uintptr_t)&d3d_buffer_to_ogl);
	}


	// _Z16lumpFindResourcePKcS0_
	uintptr_t lumpFindResource_addr = (uintptr_t)so_symbol(&so_mod, "_Z16lumpFindResourcePKcS0_");
	if (lumpFindResource_addr == NULL) {
		log_error("lumpFindResource not found\n");
	} else {
		logv_error("lumpFindResource found at %p\n", lumpFindResource_addr);
		//lumpFindResource_hook = hook_addr(lumpFindResource_addr, (uintptr_t)&lumpFindResource);
	}

	// _ZN3JBE9D3DDevice11ResolveMSAAEi
	uintptr_t d3d_resolve_msaa_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice11ResolveMSAAEi");
	if (d3d_resolve_msaa_addr == NULL) {
		log_error("d3d_resolve_msaa not found\n");
	} else {
		logv_error("d3d_resolve_msaa found at %p\n", d3d_resolve_msaa_addr);
		//d3d_resolve_msaa_hook = hook_addr(d3d_resolve_msaa_addr, (uintptr_t)&d3d_resolve_msaa);
	}

	// _ZN3JBE9D3DDevice17TextureStageState7SetToGLEmN13XGSamplerType4EnumE
	uintptr_t d3d_texture_stage_state_set_to_gl_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice17TextureStageState7SetToGLEmN13XGSamplerType4EnumE");
	if (d3d_texture_stage_state_set_to_gl_addr == NULL) {
		log_error("d3d_texture_stage_state_set_to_gl not found\n");
	} else {
		logv_error("d3d_texture_stage_state_set_to_gl found at %p\n", d3d_texture_stage_state_set_to_gl_addr);
		//d3d_texture_stage_state_set_to_gl_hook = hook_addr(d3d_texture_stage_state_set_to_gl_addr, (uintptr_t)&d3d_texture_stage_state_set_to_gl);
	}


	// _ZN16PixelProgramData4InitEPK16PixelStageShaderj
	uintptr_t PixelProgramData_Init_addr = (uintptr_t)so_symbol(&so_mod, "_ZN16PixelProgramData4InitEPK16PixelStageShaderj");
	if (PixelProgramData_Init_addr == NULL) {
		log_error("PixelProgramData_Init not found\n");
	} else {
		logv_error("PixelProgramData_Init found at %p\n", PixelProgramData_Init_addr);
		//pixel_program_data_init_hook = hook_addr(PixelProgramData_Init_addr, (uintptr_t)&pixel_program_data_init);
	}

	// _ZN3JBE13ShaderManager11LoadProgramERNS_13ShaderProgramERKNS0_9VertexDefEiRKNS0_8PixelDefEjPFiRNS_9ContainerINS_4Util10AlignedPtrIKcEEE8IteratorEE
	uintptr_t ShaderManager_LoadProgram_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE13ShaderManager11LoadProgramERNS_13ShaderProgramERKNS0_9VertexDefEiRKNS0_8PixelDefEjPFiRNS_9ContainerINS_4Util10AlignedPtrIKcEEE8IteratorEE");
	if (ShaderManager_LoadProgram_addr == NULL) {
		log_error("ShaderManager_LoadProgram not found\n");
	} else {
		logv_error("ShaderManager_LoadProgram found at %p\n", ShaderManager_LoadProgram_addr);
		//ShaderManager_LoadProgram_hook = hook_addr(ShaderManager_LoadProgram_addr, (uintptr_t)&ShaderManager_LoadProgram);
	}

	// _ZN18StageShaderProgramI16PixelStageShader16PixelProgramDataE7CompileEPcm
	uintptr_t stage_addr = (uintptr_t)so_symbol(&so_mod, "_ZN18StageShaderProgramI16PixelStageShader16PixelProgramDataE7CompileEPcm");
	if (stage_addr == NULL) {
		log_error("StageShaderProgram not found\n");
	} else {
		logv_error("StageShaderProgram found at %p\n", stage_addr);
		//stage_shader_program_hook = hook_addr(stage_addr, (uintptr_t)&stage_shader_program);
	}

	// _Z12XGSetProgramPKmPK18_D3DPixelShaderDef
	uintptr_t XGSetProgram_addr = (uintptr_t)so_symbol(&so_mod, "_Z12XGSetProgramPKmPK18_D3DPixelShaderDef");
	if (XGSetProgram_addr == NULL) {
		log_error("XGSetProgram not found\n");
	} else {
		logv_error("XGSetProgram found at %p\n", XGSetProgram_addr);
		//xg_set_program_hook = hook_addr(XGSetProgram_addr, (uintptr_t)&xg_set_program);
	}

	// _Z13XGLinkProgramPKmPK18_D3DPixelShaderDef
	uintptr_t XGLinkProgram_addr = (uintptr_t)so_symbol(&so_mod, "_Z13XGLinkProgramPKmPK18_D3DPixelShaderDef");
	if (XGLinkProgram_addr == NULL) {
		log_error("XGLinkProgram not found\n");
	} else {
		logv_error("XGLinkProgram found at %p\n", XGLinkProgram_addr);
		//xg_link_program_hook = hook_addr(XGLinkProgram_addr, (uintptr_t)&xg_link_program);
	}

	// _Z20texProcessDecompressv
	// uintptr_t texProcessDecompress_addr = (uintptr_t)so_symbol(&so_mod, "_Z20texProcessDecompressv");
	// if (texProcessDecompress_addr == NULL) {
	// 	log_error("texProcessDecompress not found\n");
	// } else {
	// 	logv_error("texProcessDecompress found at %p\n", texProcessDecompress_addr);
	// 	texProcessDecompress_hook = hook_addr(texProcessDecompress_addr, (uintptr_t)&texProcessDecompress);
	// }


	// _Z26PVRTTextureLoadFromPointerPKvPjS0_bjS0_
	// uintptr_t PVRTTextureLoadFromPointer_addr = (uintptr_t)so_symbol(&so_mod, "_Z26PVRTTextureLoadFromPointerPKvPjS0_bjS0_");
	// if (PVRTTextureLoadFromPointer_addr == NULL) {
	// 	log_error("PVRTTextureLoadFromPointer not found\n");
	// } else {
	// 	logv_error("PVRTTextureLoadFromPointer found at %p\n", PVRTTextureLoadFromPointer_addr);
	// 	pvr_texture_load_from_pointer_hook = hook_addr(PVRTTextureLoadFromPointer_addr, (uintptr_t)&pvr_texture_load_from_pointer);
	// }

	// // _Z20PVRTErrorOutputDebugPKcz
	// uintptr_t PVRTErrorOutputDebug_addr = (uintptr_t)so_symbol(&so_mod, "_Z20PVRTErrorOutputDebugPKcz");
	// if (PVRTErrorOutputDebug_addr == NULL) {
	// 	log_error("PVRTErrorOutputDebug not found\n");
	// } else {
	// 	logv_error("PVRTErrorOutputDebug found at %p\n", PVRTErrorOutputDebug_addr);
	// 	//pvr_error_output_debug_hook = hook_addr(PVRTErrorOutputDebug_addr, (uintptr_t)&pvr_error_output_debug);
	// }

	// //_ZNK14D3DBaseTexture7GetInfoER10_D3DFORMATRiS2_RmS3_
	// uintptr_t D3DBaseTexture_GetInfo_addr = (uintptr_t)so_symbol(&so_mod, "_ZNK14D3DBaseTexture7GetInfoER10_D3DFORMATRiS2_RmS3_");
	// if (D3DBaseTexture_GetInfo_addr == NULL) {
	// 	log_error("D3DBaseTexture_GetInfo not found\n");
	// } else {
	// 	logv_error("D3DBaseTexture_GetInfo found at %p\n", D3DBaseTexture_GetInfo_addr);
	// 	//d3d_base_texture_get_info_hook = hook_addr(D3DBaseTexture_GetInfo_addr, (uintptr_t)&d3d_base_texture_get_info);
	// }

	// bool XGIsSwizzledFormat(int param_1)
	// uintptr_t XGIsSwizzledFormat_addr = (uintptr_t)so_symbol(&so_mod, "XGIsSwizzledFormat");
	// if (XGIsSwizzledFormat_addr == NULL) {
	// 	log_error("XGIsSwizzledFormat not found\n");
	// } else {
	// 	logv_error("XGIsSwizzledFormat found at %p\n", XGIsSwizzledFormat_addr);
	// 	xg_is_swizzled_format_hook = hook_addr(XGIsSwizzledFormat_addr, (uintptr_t)&xg_is_swizzled_format);
	// }
	// //bool XGIsCompressedFormat(uint param_1)
	// uintptr_t XGIsCompressedFormat_addr = (uintptr_t)so_symbol(&so_mod, "XGIsCompressedFormat");
	// if (XGIsCompressedFormat_addr == NULL) {
	// 	log_error("XGIsCompressedFormat not found\n");
	// } else {
	// 	logv_error("XGIsCompressedFormat found at %p\n", XGIsCompressedFormat_addr);
	// 	xg_is_compressed_format_hook = hook_addr(XGIsCompressedFormat_addr, (uintptr_t)&xg_is_compressed_format);
	// }

 	// undefined4 XGBytesPerPixelFromFormat(undefined4 param_1)
	uintptr_t XGBytesPerPixelFromFormat_addr = (uintptr_t)so_symbol(&so_mod, "XGBytesPerPixelFromFormat");
	if (XGBytesPerPixelFromFormat_addr == NULL) {
		log_error("XGBytesPerPixelFromFormat not found\n");
	} else {
		logv_error("XGBytesPerPixelFromFormat found at %p\n", XGBytesPerPixelFromFormat_addr);
		//xg_bytes_per_pixel_from_format_hook = hook_addr(XGBytesPerPixelFromFormat_addr, (uintptr_t)&xg_bytes_per_pixel_from_format);
	}
	
	// _Z8lumpLoadPKc	
	//lumpLoad_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z8lumpLoadPKc"), (uintptr_t)&lumpLoad);

	// _Z12machMpegLoopPKcS0_PFivEiS0_S0_ibb machMpegLoop
	// uintptr_t machMpegLoop_addr = (uintptr_t)so_symbol(&so_mod, "_Z12machMpegLoopPKcS0_PFivEiS0_S0_ibb");
	// if (machMpegLoop_addr == NULL) {
	// 	log_error("machMpegLoop not found\n");
	// } else {
	// 	logv_error("machMpegLoop found at %p\n", machMpegLoop_addr);
	// 	machMpegLoop_hook = hook_addr(machMpegLoop_addr, (uintptr_t)&machMpegLoop);
	// }

	// 2106a0
	// undefined4 XMVDecoder_CreateDecoderForFile(undefined4 param_1,undefined4 param_2,undefined4 param_3)
	// uintptr_t XMVDecoder_CreateDecoderForFile_addr = so_mod.text_base + 0x1106a0;
	// if (XMVDecoder_CreateDecoderForFile_addr == NULL) {
	// 	log_error("XMVDecoder_CreateDecoderForFile not found\n");
	// } else {
	// 	logv_error("XMVDecoder_CreateDecoderForFile found at %p\n", XMVDecoder_CreateDecoderForFile_addr);
	// 	//hook_addr(XMVDecoder_CreateDecoderForFile_addr, (uintptr_t)&ret0);
	// }


	// Patch the hardcoded "sampler" string in the text section
	// This is used in the shader manager to load the sampler
	// I had to rename the sampler field in the shaders to "sam" cause in Cg it's a reserved keyword

	uintptr_t sampler = so_mod.text_base + 0x0009719f;
	// // Print the original string to see if we're at the right place
	// logv_error("Original sampler string: %s\n", (char *)sampler);
	// // Patch the string
	// kuKernelCpuUnrestrictedMemcpy((void *)sampler, "plersam", 8);
	// // Print the new string to see if it was patched correctly
	// logv_error("Patched sampler string: %s\n", (char *)sampler);


	sampler = so_mod.text_base + 0x0008d0e9 + 23;
	// Print the original string to see if we're at the right place
	logv_error("Original sampler string #2: %s\n", (char *)sampler);
	// Patch the string
	kuKernelCpuUnrestrictedMemcpy((void *)sampler, "plersam", 7);
	// Print the new string to see if it was patched correctly
	logv_error("Patched sampler string #2: %s\n", (char *)sampler);

	sampler = so_mod.text_base + 0x0008d0e9 + 99;
	// Print the original string to see if we're at the right place
	logv_error("Original sampler string #3: %s\n", (char *)sampler);
	// Patch the string
	kuKernelCpuUnrestrictedMemcpy((void *)sampler, "plersam", 7);
	// Print the new string to see if it was patched correctly
	logv_error("Patched sampler string #3: %s\n", (char *)sampler);

	
	// Will delete: patch for fixing comparison > -1 for uniform locations
	//uintptr_t addressToPatch = so_mod.text_base + 0x001fb260 - 0x00010000;
	// Print the next 8 bytes to see if we're at the right place
	//logv_error("Original bytes at %p: %x %x %x %x %x %x %x %x\n", addressToPatch, *(uint8_t *)addressToPatch, *(uint8_t *)(addressToPatch + 1), *(uint8_t *)(addressToPatch + 2), *(uint8_t *)(addressToPatch + 3), *(uint8_t *)(addressToPatch + 4), *(uint8_t *)(addressToPatch + 5), *(uint8_t *)(addressToPatch + 6), *(uint8_t *)(addressToPatch + 7));
	// Patch the bytes with 01 00 71 e3 61 ff ff 0a ( != -1 instead of > -1)
	//kuKernelCpuUnrestrictedMemcpy((void *)addressToPatch, "\x01\x00\x71\xe3\x61\xff\xff\x0a", 8);

	// Patch the bytes with NOPs
	// kuKernelCpuUnrestrictedMemcpy((void *)addressToPatch, "\x00\x00\x00\x00\x00\x00\x00\x00", 8);

	// Print the new bytes to see if it was patched correctly
	//logv_error("Patched bytes at %p: %x %x %x %x %x %x %x %x\n", addressToPatch, *(uint8_t *)addressToPatch, *(uint8_t *)(addressToPatch + 1), *(uint8_t *)(addressToPatch + 2), *(uint8_t *)(addressToPatch + 3), *(uint8_t *)(addressToPatch + 4), *(uint8_t *)(addressToPatch + 5), *(uint8_t *)(addressToPatch + 6), *(uint8_t *)(addressToPatch + 7));


	// uintptr_t menuTexture = so_mod.text_base + 0x000a3a91 - 0x00010000;
	// // Print the original string to see if we're at the right place
	// logv_error("Original menuTexture string: %s\n", (char *)menuTexture);
	// // Patch the string and add the null terminator cause original string was longer "frontendmenulong.tex"
	// kuKernelCpuUnrestrictedMemcpy((void *)menuTexture, "frontendmenu.tex", 17);
	// // Print the new string to see if it was patched correctly
	// logv_error("Patched menuTexture string: %s\n", (char *)menuTexture);


	// _ZN3JBE13ShaderManager11LoadProgramERNS_13ShaderProgramERKNS0_9VertexDefEiRKNS0_8PixelDefEjPFiRNS_9ContainerINS_4Util10AlignedPtrIKcEEE8IteratorEE
	//ShaderManager_LoadProgram_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN3JBE13ShaderManager11LoadProgramERNS_13ShaderProgramERKNS0_9VertexDefEiRKNS0_8PixelDefEjPFiRNS_9ContainerINS_4Util10AlignedPtrIKcEEE8IteratorEE"), (uintptr_t)&ShaderManager_LoadProgram);

	//memPrintFree_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z12memPrintFreev"), (uintptr_t)&memPrintFree);
	// // "_Z8listInitv"
	// listInit_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z8listInitv"), (uintptr_t)&listInit);
	// // _Z8machInitv
	// _machInit_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z8machInitv"), (uintptr_t)&machInit);
	// // undefined4 QueryPerformanceFrequency(undefined4 *param_1)
	// _queryPerformanceFrequency_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "QueryPerformanceFrequency"), (uintptr_t)&queryPerformanceFrequency);

	
	// // void Direct3DCreate8(void)
	// uintptr_t addr = (uintptr_t)so_symbol(&so_mod, "Direct3DCreate8");
	// if (addr == NULL) {
	// 	printf("Direct3DCreate8 not found\n");
	// } else {
	// 	printf("Direct3DCreate8 found at %p\n", addr);
	// 	_direct3DCreate8_hook = hook_addr(addr, (uintptr_t)&direct3DCreate8);
	// }



	// void CreateFileA(byte *param_1,int param_2)
	// _createFile_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "CreateFileA"), (uintptr_t)&createFile);
	// //_ZN3JBE4File4OpenEPKcNS0_4ModeE
	// // undefined4 __thiscall JBE::File::Open(File *this,char *param_1,Mode param_2)
	// _openFile_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN3JBE4File4OpenEPKcNS0_4ModeE"), (uintptr_t)&openFile);


	// // padInit
	// _padInit_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z7padInitv"), (uintptr_t)&padInit);
	// // cdInit
	// _cdInit_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z6cdInitv"), (uintptr_t)&cdInit);
	// // emathInit
	// _emathInit_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z9emathInitv"), (uintptr_t)&emathInit);
	

	// // Trophies support
	// achieve_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN7Achieve10setAchieveEii"), (uintptr_t)&setAchieve);
	
	// // Disable anything stage related for Takamatsu Castle to not tank framerate
	// I_HeapKaraLoop = so_symbol(&so_mod, "I_HeapKaraLoop");
	// takamatsu_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z17I_TakamatsuSummerv"), (uintptr_t)&TakamatsuSummer);
	// takamatsu2_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z17I_TakamatsuWinterv"), (uintptr_t)&TakamatsuWinter);
	
	// // Kill "PertBoss" spawning in Money Pit. No idea what this is but seems to help with framerate tanking
	// uint16_t instr16 = 0xd0c9; // beq #0xffffff96
	// kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x10eb7c), &instr16, 2);
	
	// // Killing S/N-Fire elements in Money Pit. Seems to help framerate with little changes to the actual stage
	// uint32_t instr32 = 0xaf41f43f;
	// kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x10e55e), &instr32, 4);
	// instr32 = 0xaf35f43f;
	// kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x10e702), &instr32, 4);
	// instr32 = 0xaf3df43f;
	// kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x10e90e), &instr32, 4);
	// instr32 = 0xaf3ff43f;
	// kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x10eaf6), &instr32, 4);
	// instr32 = 0xaf4ef47f;
	// kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x10ed64), &instr32, 4);
	// instr32 = 0xaf54f47f;
	// kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x10ef6c), &instr32, 4);
	
	// // Paralyze mice in Money Pit to save on framerate taxing
	// hook_addr((uintptr_t)so_symbol(&so_mod, "_Z11I_ObjMouse0v"), (uintptr_t)&ret0);
	
	// // Kill ring edge particles spawning. Seems to not affect graphics in any way but helps in Money Pit.
	// hook_addr((uintptr_t)so_symbol(&so_mod, "_Z24I_CreateRingEdgeParticleP7FVECTORS0_S0_P7FMATRIX"), (uintptr_t)&ret0);

	// // Prevent game from crashing when attempting to exit it
	// hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN11SoundOpenSL8shutdownEv"), (uintptr_t)&exit_process);
}

