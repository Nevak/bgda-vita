/*
 * lockrect_struct.c
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
extern so_module so_mod_libxmv;

#ifdef __cplusplus
};
#endif

#include "utils/logger.h"
#include <stdbool.h>

so_hook achieve_hook, stage_hook, takamatsu_hook, takamatsu2_hook, lumpLoad_hook, memPrintFree_hook, listInit_hook;


// int setAchieve(void *this, int id, int unlock) {
// 	printf("setAchieve(%i, %i)\n", id, unlock);
// 	trophies_unlock(id + 1);
	
// 	return SO_CONTINUE(int, achieve_hook, this, id, unlock);
// }

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

so_hook av_log_hook;
//void av_log(int *param_1,int log_type, char* format, ...)
void av_log(void *param_1, int log_type, char *format, ...) {
	// // take the variable arguments
	// va_list args;
	// // print the entire message
	// char string[512];
	
	// va_start(args, format);
    
    // sceClibVsnprintf(string, sizeof(string), format, args);
    // va_end(args);

	// logv_error("av_log(%p, %i, %s)\n", param_1, log_type, string);

	// //SO_CONTINUE(void *, av_log_hook, param_1, log_type, format);


}


so_hook hasNEON_hook;
int hasNEON() {
	log_error("hasNEON()\n");
	int ret = SO_CONTINUE(int, hasNEON_hook);
	logv_error("hasNEON returned value %i\n", ret);
	return ret;
}

so_hook gameLoop_hook;
void gameLoop() {
	float timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, gameLoop_hook);
	float timeAfter = sceKernelGetProcessTimeWide();
	logv_error("gameLoop took %f ms\n", (timeAfter - timeNow) / 1000);
}

so_hook machFrameStart_hook;
float lastTime = 0;
float startTime = 0;
uint32_t frameCount = 0;
void machFrameStart() {
	float timeBefore = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, machFrameStart_hook);
	float timeAfter = sceKernelGetProcessTimeWide();
	frameCount++;
	
	startTime = timeBefore;

	float deltaTime = timeAfter - timeBefore;
	if (deltaTime > 20000) {
		logv_error("#%d frame machFrameStart self took %f ms\n", frameCount, deltaTime / 1000);
	}

	//float timeNow = sceKernelGetProcessTimeWide();
	//float deltaTime = timeNow - lastTime;
	//lastTime = timeNow;
	// if (deltaTime > 50000) {
	// 	logv_error("#%d frame machFrameStart took %f ms\n", frameCount, deltaTime / 1000);
	// }
}

so_hook machFrameEnd_hook;
void machFrameEnd(int param_1) {
	float timeBefore = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, machFrameEnd_hook, param_1);
	float timeNow = sceKernelGetProcessTimeWide();
	float deltaTimeIn = timeNow - timeBefore;
	if (deltaTimeIn > 20000) {
		logv_error("#%d frame machFrameEnd(%d) self took %f ms\n", frameCount, param_1, deltaTimeIn / 1000);
	}

//	float timeNow = sceKernelGetProcessTimeWide();
	float deltaTime = timeNow - lastTime;
	lastTime = timeNow;
	//frameCount++;
	if (deltaTime > 20000) {
	 	logv_error("#%d frame end-to-end took %f ms\n", frameCount, deltaTime / 1000);
	}

	deltaTime = timeNow - startTime;
	if (deltaTime > 20000) {
		logv_error("#%d frame start-to-end took %f ms\n", frameCount, deltaTime / 1000);
	}
}

so_hook objectDrawDelayedDrawObjects_hook;
void objectDrawDelayedDrawObjects() {
	float timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, objectDrawDelayedDrawObjects_hook);
	float timeAfter = sceKernelGetProcessTimeWide();
	if (timeAfter - timeNow > 1000) {
		logv_error("objectDrawDelayedDrawObjects took %f ms\n", (timeAfter - timeNow) / 1000);
	}
}

so_hook SND_Frame_hook;
void SND_Frame() {
// 	float timeNow = sceKernelGetProcessTimeWide();
// 	SO_CONTINUE(void *, SND_Frame_hook);
// 	float timeAfter = sceKernelGetProcessTimeWide();
// 	if (timeAfter - timeNow > 10000) {
// 		logv_error("SND_Frame took %f ms\n", (timeAfter - timeNow) / 1000);
// 	}
}

so_hook animFrame_hook;
void animFrame() {
	float timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, animFrame_hook);
	float timeAfter = sceKernelGetProcessTimeWide();
	if (timeAfter - timeNow > 1) {
		logv_error("animFrame took %f ms\n", (timeAfter - timeNow) / 1000);
	}
}

so_hook cdProcess_hook;
void cdProcess(int param_1) {
	float timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, cdProcess_hook, param_1);
	float timeAfter = sceKernelGetProcessTimeWide();
	if (timeAfter - timeNow > 1) {
		logv_error("cdProcess took %f ms\n", (timeAfter - timeNow) / 1000);
	}
}

so_hook texProcessDecompress_hook;
void texProcessDecompress() {
	float timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, texProcessDecompress_hook);
	float timeAfter = sceKernelGetProcessTimeWide();
	if (timeAfter - timeNow > 1) {
		logv_error("texProcessDecompress took %f ms\n", (timeAfter - timeNow) / 1000);
	}
}

so_hook worldPlotRouteProcess_hook;
void worldPlotRouteProcess(int param_1) {
	float timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, worldPlotRouteProcess_hook, param_1);
	float timeAfter = sceKernelGetProcessTimeWide();
	if (timeAfter - timeNow > 1) {
		logv_error("worldPlotRouteProcess took %f ms\n", (timeAfter - timeNow) / 1000);
	}
}

extern uint32_t world_elements_count;
so_hook gameLoadWorld_hook;
void gameLoadWorld(char *param_1) {
	world_elements_count = 0;
	logv_error("gameLoadWorld(%s)\n", param_1);
	SO_CONTINUE(void *, gameLoadWorld_hook, param_1);
	logv_error("gameLoadWorld(%s) loaded %d elements\n", param_1, world_elements_count);
}

extern int log_allocs;


so_hook machHostOpen_hook;
int machHostOpen(char *param_1, char *param_2) {
	logv_error("machHostOpen(%s, %s)\n", param_1, param_2);
	return SO_CONTINUE(void *, machHostOpen_hook, param_1, param_2);
}

so_hook renderTouchIcons_hook;
void renderTouchIcons(void *param_1) {

}

so_hook usingTouchscreen_hook;
bool usingTouchscreen() {
	return false;
}

so_hook updateCheats_hook;
void updateCheats() {

}

so_hook frontEndDoControllerScreenInput_hook;
void frontEndDoControllerScreenInput(int *param_1, int *param_2) {

}

so_hook lump_set_discardable_hook;
void lump_set_discardable(char *param_1) {
	logv_error("lump_set_discardable(%s)\n", param_1);
	SO_CONTINUE(void *, lump_set_discardable_hook, param_1);
}

so_hook lumpClear_hook;
void lumpClear(int param_1) {
	logv_error("lumpClear(%i)\n", param_1);
	SO_CONTINUE(void *, lumpClear_hook, param_1);
}

so_hook worldFreeWorld_hook;
void worldFreeWorld(void *param_1) {
	logv_error("worldFreeWorld(%p)\n", param_1);
	SO_CONTINUE(void *, worldFreeWorld_hook, param_1);
}

so_hook d3d_resource_release_hook;
void d3d_resource_release(void *thisptr) {
	logv_error("d3d_resource_release(%p)\n", thisptr);
	SO_CONTINUE(void *, d3d_resource_release_hook, thisptr);
}

so_hook d3d_delete_resource_async_hook;
void d3d_delete_resource_async(void *thisptr) {
	logv_error("d3d_delete_resource_async(%p)\n", thisptr);
	SO_CONTINUE(void *, d3d_delete_resource_async_hook, thisptr);
}

so_hook inputRender_hook;
void inputRender(void *thisptr) {	
}

so_hook virtualControlsCtor_hook;
void virtualControlsCtor(void *thisptr) {
	log_error("virtualControlsCtor()\n");
}

so_hook virtualControlsRender_hook;
void virtualControlsRender(void *thisptr) {
	//logv_error("virtualControlsRender()\n");
	//SO_CONTINUE(void *, virtualControlsRender_hook);
}

so_hook d3dDevice_swap_hook;
void d3dDevice_swap(uint param_1) {
	float timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, d3dDevice_swap_hook, param_1);
	float timeAfter = sceKernelGetProcessTimeWide();
	if (timeAfter - timeNow > 10000) {
		logv_error("d3dDevice_swap(%d) took %f ms\n", param_1, (timeAfter - timeNow) / 1000);
	}
}

so_hook systemUpdate_hook;
void systemUpdate() {
	float timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void *, systemUpdate_hook);
	float timeAfter = sceKernelGetProcessTimeWide();
	if (timeAfter - timeNow > 10000) {
		logv_error("system_update took %f ms\n", (timeAfter - timeNow) / 1000);
	}
}

so_hook d3d_set_gamma_ramp_hook;
void d3d_set_gamma_ramp(void *thisptr, void *param_1) {
	logv_error("d3d_set_gamma_ramp(%p, %p)\n", thisptr, param_1);
	SO_CONTINUE(void *, d3d_set_gamma_ramp_hook, thisptr, param_1);
}

so_hook touchControllerUpdate_hook;
// JBE::TouchController::Update(TouchController *this,int *param_1,uint param_2,int param_3)
void touchControllerUpdate(void *thisptr, int *param_1, unsigned int param_2, int param_3) {

}

so_hook d3dDeviceReadCommand_hook;
void d3dDeviceReadCommand(void *thisptr) {
	logv_error("d3dDeviceReadCommand(%p)\n", thisptr);
	SO_CONTINUE(void *, d3dDeviceReadCommand_hook, thisptr);
}

so_hook ogg_alloc_hook;
void *ogg_alloc(int size) {
	if (log_allocs == 1)
	{
		logv_error("ogg_alloc(%u)\n", size);
	}

	void *returnval = NULL;
	returnval = SO_CONTINUE(void *, ogg_alloc_hook, size);
	return returnval;
}

so_hook d3d_create_texture2_hook;
void *d3d_create_texture2(int param_1, int param_2, int param_3, int param_4, unsigned int param_5, int param_6) {
	//void *returnval = NULL;
	//world_elements_count++;

	logv_error("d3d_create_texture2(%i, %i, %i, %i, %u, %i)\n", param_1, param_2, param_3, param_4, param_5, param_6);
	void * returnval = SO_CONTINUE(void *, d3d_create_texture2_hook, param_1, param_2, param_3, param_4, param_5, param_6);

	return returnval;
}


so_hook xg_set_texture_header_hook;
// undefined4 XGSetTextureHeader(uint param_1,uint param_2,int param_3,uint param_4,uint param_5,undefined4 param_6,uint *param_7,uint param_8,uint param_9)
int xg_set_texture_header(uint param_1, uint param_2, int param_3, uint param_4, uint param_5, uint param_6, uint *param_7, uint param_8, uint param_9) {
	logv_error("XGSetTextureHeader: param_1: %i, param_2: %i, param_3: %i, param_4: %i, param_5: %i, param_6: %i, param_7: %p, param_8: %i, param_9: %i\n", param_1, param_2, param_3, param_4, param_5, param_6, param_7, param_8, param_9);
	if (param_9 > 0)
	{
		

		int alignment = 4;
		int aligned = (param_1 + (alignment - 1)) & ~(alignment - 1);
		if (aligned < alignment) aligned = alignment;
		param_9 = aligned;
	}
	
	int returnval = SO_CONTINUE(int, xg_set_texture_header_hook, param_1, param_2, param_3, param_4, param_5, param_6, param_7, param_8, param_9);
	//logv_error("XGSetTextureHeader returned %i\n", returnval);
	return returnval;
}


// _ZN3JBE7AudioPF12StreamThread10ThreadFuncEv
so_hook audio_thread_hook;
void audio_thread() {
	log_error("audio_thread()\n");
	while (1) {
		if (log_allocs == 1)
		{
			log_error("audio_thread\n");
			// crash the game
			void *ptr = NULL;
			*(int *)ptr = 0;
		}
		usleep(0x40 * 1000);
	}
}

so_hook writeConfigDirect_hook;
void writeConfigDirect() {
	log_error("writeConfigDirect()\n");
	//SO_CONTINUE(void *, writeConfigDirect_hook);
}



so_hook lowestPowerof2NotLessThan_hook;
int lowestPowerof2NotLessThan(int dimension) {

	//return (dimension >> 0x12 & 0xffffffc0) + 0x40;

	//return SO_CONTINUE(int, lowestPowerof2NotLessThan_hook, dimension);
	
	// 0x98521dcc -> width
	// 0x98521dec -> height
	
	// // print the caller address
	uintptr_t caller = __builtin_return_address(0);
	if (caller != 0x98521dec && caller != 0x98521dcc) {
		return SO_CONTINUE(int, lowestPowerof2NotLessThan_hook, dimension);
	}
	//return dimension;

    int alignment = 4;
    int aligned = (dimension + (alignment - 1)) & ~(alignment - 1);
    if (aligned < alignment) 
		aligned = alignment;
    return aligned;
}


/* worldAllocateSegments(_worldHeader*) */
#define LOC(x) (int *)(so_mod.text_base + x - 0x00010000)
#define CONCAT22(high16, low16) ( \
    ( ((uint32_t)(high16) & 0xFFFF) << 16 ) | \
      ((uint32_t)(low16)  & 0xFFFF)          \
)
#define CONCAT44(high32, low32) ( \
    ( ((uint64_t)(high32) & 0xFFFFFFFFULL) << 32 ) | \
      (  (uint64_t)(low32)  & 0xFFFFFFFFULL )        \
)


static inline int UnsignedSaturate8(int x) {
    if (x < 0)   return 0;
    if (x > 255) return 255;
    return x;
}

// machHostSeek
static int (*machHostSeek)(int, int, int) = NULL;
// _Z12machHostReadiPvi
static int (*machHostRead)(int, void *, int) = NULL;
// _Z13machHostClosei
static void (*machHostClose)(int) = NULL;
// _Z16lockLoadingMutexb
static void (*lockLoadingMutex)(bool) = NULL;
// _Z19releaseLoadingMutexv
static void (*releaseLoadingMutex)(void) = NULL;
// D3DDevice_CreatePalette2
static uint32_t (*D3DDevice_CreatePalette2)(int) = NULL;
// D3DPalette_Lock2
static int (*D3DPalette_Lock2)(uint32_t, int) = NULL;
// D3DDevice_CreateTexture2
static uint32_t (*D3DDevice_CreateTexture2)(int, int, int, int, int, int, int) = NULL;
// D3DTexture_LockRect
static void (*D3DTexture_LockRect)(void *, int, int *, void *, int) = NULL;
// D3DTexture_UnlockRect
static void (*D3DTexture_UnlockRect)(uint32_t, int) = NULL;
// D3DBaseTexture_GetInfo
static void (*D3DBaseTexture_GetInfo)(void *, int, int *, int *, int *, int *) = NULL;

so_hook d3d_texture_lock_rect_hook;
void d3d_texture_lock_rect(uint32_t *pThis, uint32_t Level, int *pLockedRect, void *pRect, int Flags) {
	logv_error("d3d_texture_lock_rect: pThis: %p, Level: %i, pLockedRect: %p, pRect: %p, Flags: %i\n", pThis, Level, pLockedRect, pRect, Flags);
	// Call the original function
	SO_CONTINUE(void *, d3d_texture_lock_rect_hook, pThis, Level, pLockedRect, pRect, Flags);

	// call D3DBaseTexture_GetInfo
	int format = 0;
	int w = 0;
	int h = 0;
	int unk = 0;
	int unk2 = 0;
	D3DBaseTexture_GetInfo(pThis, &unk, &format, &unk2, &w, &h);
	// log the width and height
	logv_error("d3d_texture_lock_rect: w: %i, h: %i\n", w, h);
	// log the pitch
	logv_error("d3d_texture_lock_rect: pLockedRect: %i\n", *pLockedRect);

	int dimension = w;
	int alignment = 4;
	int aligned = (dimension + (alignment - 1)) & ~(alignment - 1);
	if (aligned < alignment) 
		aligned = alignment;

	*pLockedRect = aligned;

}


typedef struct _worldHeader _worldHeader, *P_worldHeader;

struct _worldHeader { /* PlaceHolder Structure */
    int field0_0x0;
    uint8_t field1_0x4;
    uint8_t field2_0x5;
    uint8_t field3_0x6;
    uint8_t field4_0x7;
    uint8_t field5_0x8;
    uint8_t field6_0x9;
    uint8_t field7_0xa;
    uint8_t field8_0xb;
    uint8_t field9_0xc;
    uint8_t field10_0xd;
    uint8_t field11_0xe;
    uint8_t field12_0xf;
    uint8_t field13_0x10;
    uint8_t field14_0x11;
    uint8_t field15_0x12;
    uint8_t field16_0x13;
    uint8_t field17_0x14;
    uint8_t field18_0x15;
    uint8_t field19_0x16;
    uint8_t field20_0x17;
    uint8_t field21_0x18;
    uint8_t field22_0x19;
    uint8_t field23_0x1a;
    uint8_t field24_0x1b;
    uint8_t field25_0x1c;
    uint8_t field26_0x1d;
    uint8_t field27_0x1e;
    uint8_t field28_0x1f;
    uint8_t field29_0x20;
    uint8_t field30_0x21;
    uint8_t field31_0x22;
    uint8_t field32_0x23;
    int field33_0x24;
    uint8_t field34_0x28;
    uint8_t field35_0x29;
    uint8_t field36_0x2a;
    uint8_t field37_0x2b;
    uint8_t field38_0x2c;
    uint8_t field39_0x2d;
    uint8_t field40_0x2e;
    uint8_t field41_0x2f;
    uint8_t field42_0x30;
    uint8_t field43_0x31;
    uint8_t field44_0x32;
    uint8_t field45_0x33;
    uint8_t field46_0x34;
    uint8_t field47_0x35;
    uint8_t field48_0x36;
    uint8_t field49_0x37;
    uint8_t field50_0x38;
    uint8_t field51_0x39;
    uint8_t field52_0x3a;
    uint8_t field53_0x3b;
    uint8_t field54_0x3c;
    uint8_t field55_0x3d;
    uint8_t field56_0x3e;
    uint8_t field57_0x3f;
    uint8_t field58_0x40;
    uint8_t field59_0x41;
    uint8_t field60_0x42;
    uint8_t field61_0x43;
    uint8_t field62_0x44;
    uint8_t field63_0x45;
    uint8_t field64_0x46;
    uint8_t field65_0x47;
    uint8_t field66_0x48;
    uint8_t field67_0x49;
    uint8_t field68_0x4a;
    uint8_t field69_0x4b;
    uint8_t field70_0x4c;
    uint8_t field71_0x4d;
    uint8_t field72_0x4e;
    uint8_t field73_0x4f;
    uint8_t field74_0x50;
    uint8_t field75_0x51;
    uint8_t field76_0x52;
    uint8_t field77_0x53;
    uint8_t field78_0x54;
    uint8_t field79_0x55;
    uint8_t field80_0x56;
    uint8_t field81_0x57;
    int field82_0x58;
    int field83_0x5c;
    uint8_t field84_0x60;
    uint8_t field85_0x61;
    uint8_t field86_0x62;
    uint8_t field87_0x63;
    int field88_0x64;
    uint8_t field89_0x68;
    uint8_t field90_0x69;
    uint8_t field91_0x6a;
    uint8_t field92_0x6b;
    uint8_t field93_0x6c;
    uint8_t field94_0x6d;
    uint8_t field95_0x6e;
    uint8_t field96_0x6f;
    uint8_t field97_0x70;
    uint8_t field98_0x71;
    uint8_t field99_0x72;
    uint8_t field100_0x73;
    int field101_0x74;
};

void worldAllocateSegments(_worldHeader *param_1)
{
	uint uVar1;
	char *pcVar2;
	int iVar3;
	uint uVar4;
	void *pTexFile;
	int iVar6;
	int *piVar7;
	int iVar8;
	int iVar9;
	int texWidth;
	int texHeight;
	uint32_t pallette;
	int iVar13;
	char cVar14;
	uint16_t *puVar15;
	int iVar16;
	int iVar17;
	int iVar18;
	int iVar19;
	int iVar20;
	int totalPixels;
	int iVar22;
	uint uVar23;
	int *piVar24;
	short *psVar25;
	uint8_t *pbVar26;
	int iVar27;
	int iVar28;
	uint64_t *puVar29;
	int iVar30;
	int iVar31;
	int iVar32;
	int iVar33;
	int iVar34;
	int iVar35;
	int iVar36;
	int *piVar37;
	void *__newTexPtr;
	uint uVar38;
	bool bVar39;
	float fVar40;
	int iVar41;

	//uint32_t local_598 = 0;
	//uint32_t uStack_594 = 0;
	// struct
	// {
	// 	uint32_t local_650;
	// 	uint32_t local_598;	/* data */
	// } patch;
	uint32_t local_650;

	struct {
		uint32_t local_598;
		uint32_t uStack_594;
		uint64_t uStack_590;
		uint64_t local_588;
		uint64_t uStack_580;
		uint64_t local_578;
		uint64_t uStack_570;
		uint64_t local_568;
		uint64_t uStack_560;
		uint64_t local_558;
		uint64_t uStack_550;
		uint64_t local_548;
		uint64_t uStack_540;
		uint64_t local_538;
		uint64_t uStack_530;
		uint64_t local_528;
		uint64_t uStack_520;
		uint64_t local_518;
		uint64_t uStack_510;
		uint64_t local_508;
		uint64_t uStack_500;
		uint64_t local_4f8;
		uint64_t uStack_4f0;
		uint64_t local_4e8;
		uint64_t uStack_4e0;
		uint64_t local_4d8;
		uint64_t uStack_4d0;
		uint64_t local_4c8;
		uint64_t uStack_4c0;
		uint64_t local_4b8;
		uint64_t uStack_4b0;
		uint64_t local_4a8;
		uint64_t uStack_4a0;
		uint16_t huffman_table [512];
	} lockrect_struct;


	char acStack_98 [80];


	// coreCurrentWorld is at 0x0054cad8 in so_mod
	char * coreCurrentWorld = LOC(0x0054cad8);
	logv_error("worldAllocateSegments: coreCurrentWorld: %s\n", coreCurrentWorld);
	
	iVar3 = param_1->field0_0x0;
	if (0 < iVar3) {

		logv_error("worldAllocateSegments: iVar3: %i\n", iVar3);

		iVar27 = 0;
		iVar19 = 0;
		do {
			if ((*(ushort *)(param_1->field33_0x24 + iVar27 + 0x30) & 0x800) != 0) {	
				sprintf((char *)lockrect_struct.huffman_table,"%s.lmp",*(uint32_t *)(param_1->field33_0x24 + iVar27));
				logv_error("worldAllocateSegments: will load %s\n", (char *)lockrect_struct.huffman_table);
				lumpLoad((char *)lockrect_struct.huffman_table);
				iVar3 = param_1->field0_0x0;
			}

			iVar19 = iVar19 + 1;
			iVar27 = iVar27 + 0x38;
		} while (iVar19 < iVar3);
	}
	if (param_1->field101_0x74 == 0) {
		param_1->field101_0x74 = 1;
		iVar27 = param_1->field82_0x58;
		iVar36 = param_1->field83_0x5c;
		iVar19 = param_1->field88_0x64; // Pointer to some world data
		

		sprintf(acStack_98,"res\\%s.tex",coreCurrentWorld);
		iVar3 = machHostOpen(acStack_98,"rb");
		uVar4 = machHostSeek(iVar3,0,2);
		logv_error("worldAllocateSegments: seek uVar4: %u\n", uVar4); // uVar4: 3383469 (.tex file size)
		pTexFile = malloc(uVar4);	// Just load the entire .tex file into memory
		machHostSeek(iVar3,0,0);
		machHostRead(iVar3,pTexFile,uVar4);
		machHostClose(iVar3);

		logv_error("worldAllocateSegments: iVar36: %i\n", iVar36); // 5050
		logv_error("worldAllocateSegments: iVar27: %i\n", iVar27); // 4949
		logv_error("worldAllocateSegments: iVar19: 0x%X\n", iVar19);

		if (iVar36 < iVar27) {
			free(pTexFile);
		}
		else {
			__newTexPtr = (void *)0x0;
			iVar3 = 0;
			do { // while (iVar3 != (iVar36 - iVar27) + 1);  0101+1
				// Probably loops for each texture
				// Maybe iterate map objects and find their texture in the .tex file ?
				iVar6 = *(int *)(iVar19 + iVar3 * 8 + 4); // ivar19 is the pointer to some world data like 0x833DA220 (probably the .gob file)
				bVar39 = iVar6 != 0; // probably iVar6 here is like a boolean flag
				if (bVar39) {
					// if not 0, then iVar6 is the offset in the .tex file to the next group
					iVar6 = *(int *)(iVar19 + iVar3 * 8);
				}
				if (bVar39 && iVar6 != 0) { 
					logv_error("worldAllocateSegments: iVar6: %i\n", iVar6); // 2048 in the first iteration. All zeros in tavern.txt file
					iVar20 = *(int *)((int)pTexFile + iVar6);					// iVar20 is the number of objects in the group
					logv_error("worldAllocateSegments: iVar20: %i\n", iVar20);
					piVar7 = (int *)malloc(iVar20 * 0x38 | 4);
					*(int **)(iVar19 + iVar3 * 8) = piVar7;
					piVar24 = (int *)((int)pTexFile + iVar6) + 0x10;
					*piVar7 = iVar20;		// write the number of objects in the group to the first byte of the allocated memory
					__aeabi_memcpy4(*(int *)(iVar19 + iVar3 * 8) + 4, piVar24, iVar20 * 0x38);
					// iVar20 = 7, 8, 122, 14 in tavern map
					// 7 + 8 + 122 + 14 = 151 -> Total number of objects in tavern map according to world explorer tool
					// probably objects are grouped 
					// iVar20 -> total number of objects in the group
					if (0 < iVar20) {
						logv_error("worldAllocateSegments: iVar20: %i\n", iVar20);

						local_650 = 0;
						piVar7 = piVar24; // piVar7 points to the first object in the group
						// Iterate over each object in the group
						do { // while (lockrect_struct.local_650 != iVar20);
							// ivar20 == 7, 8, 122, 14

							// piVar7 is increased by 0xE every iteration
							// piVar7 points to "width" of the object
							iVar8 = piVar7[2]; 	// 120384: offset to data?
							iVar9 = *(int *)((int)piVar7 + iVar8);
							logv_error("worldAllocateSegments: iVar8: %i\n", iVar8); // 120384
							logv_error("worldAllocateSegments: iVar9: %i\n", iVar9); // iVar9=400
							iVar6 = iVar9 - (int)piVar24;
							logv_error("worldAllocateSegments: iVar6: %i\n", iVar6); 
							logv_error("worldAllocateSegments: piVar24: %i\n", piVar24); 
							logv_error("worldAllocateSegments: piVar7: %i\n", piVar7); 
							//*LOC(0x003f8a60) = (int)piVar24 + (int)piVar7 + iVar6 + 0x400;
							*LOC(0x003f8a60) = (int)piVar7 + iVar9 + 0x400; // + 1424 or 0x590
							logv_error("worldAllocateSegments: *LOC(0x003f8a60): %i\n", *LOC(0x003f8a60)); // 120384 + 0x400 = 120784
							*LOC(0x003f8a64) = (int)piVar7 + iVar6 + 0xc04; // + 3476 or 0xD94
							logv_error("worldAllocateSegments: *LOC(0x003f8a64): %i\n", *LOC(0x003f8a64)); // 120384 + 0xc04 = 121388
							*LOC(0x003f8a68) = *LOC(0x003f8a64) + *(int *)((int)piVar7 + iVar6 + 0xc00) * 2;
							logv_error("worldAllocateSegments: *LOC(0x003f8a68): %i\n", *LOC(0x003f8a68)); // 121388 + 0x1E0 = 121568
							*LOC(0x003f8a6c) = *LOC(0x003f8a68) + 0x48;
							logv_error("worldAllocateSegments: *LOC(0x003f8a6c): %i\n", *LOC(0x003f8a6c)); // 121568 + 0x48 = 121616
													
							texWidth = lowestPowerof2NotLessThan((int)*(short *)piVar7);
							//texWidth = (int)*(short *)piVar7;
							iVar6 = texWidth;
							if (texWidth < 0x41) {
								iVar6 = 0x40;
							}

							texHeight = lowestPowerof2NotLessThan((int)*(short *)((int)piVar7 + 2));
							//texHeight = (int)*(short *)((int)piVar7 + 2);

							if (__newTexPtr != (void *)0x0) {
								free(__newTexPtr);
							}

							// log the width and height of the texture
							logv_error("--------: texWidth: %i, texHeight: %i\n", texWidth, texHeight);
							logv_error("--------: texWidthRaw: %i, texHeightRaw: %i\n", (int)*(short *)piVar7, (int)*(short *)((int)piVar7 + 2));
							// log the actual width and height of the texture

							piVar37 = (int *)((int)piVar7 + iVar8) + 1;
							totalPixels = iVar6 * texHeight;
							__newTexPtr = malloc(totalPixels + 0x19000); // 102400
							__aeabi_memclr(__newTexPtr, totalPixels);
							iVar41 = *LOC(0x003f8a6c);
							iVar34 = *LOC(0x003f8a68);
							iVar8 = *LOC(0x003f8a64);
							uVar4 = 0;
							iVar13 = *(int *)(LOC(0x003f8a6c + 4));
							puVar15 = (uint16_t *)((uint)lockrect_struct.huffman_table | 2);
							
							// Probably creating the Huffman table 
							do { 
								// This loops 256 times and seems to fill
								// the 512 byte huffman_table array with some values 
								// 2 bytes each time
								// iVar13 and iVar41 contain something from memory
								// uVar4 is the loop counter
							
								if (iVar13 < (int)(uVar4 >> 7)) {
									if ((int)(uVar4 >> 6) <= *(int *)(iVar41 + 8)) {
										uVar38 = uVar4 >> 6;
										iVar18 = 2;
										goto LAB_00131f0c;
									}
									if ((int)(uVar4 >> 5) <= *(int *)(iVar41 + 0xc)) {
										uVar38 = uVar4 >> 5;
										iVar18 = 3;
										goto LAB_00131f0c;
									}
									if ((int)(uVar4 >> 4) <= *(int *)(iVar41 + 0x10)) {
										uVar38 = uVar4 >> 4;
										iVar18 = 4;
										goto LAB_00131f0c;
									}
									if ((int)(uVar4 >> 3) <= *(int *)(iVar41 + 0x14)) {
										uVar38 = uVar4 >> 3;
										iVar18 = 5;
										goto LAB_00131f0c;
									}
									if ((int)(uVar4 >> 2) <= *(int *)(iVar41 + 0x18)) {
										uVar38 = uVar4 >> 2;
										iVar18 = 6;
										goto LAB_00131f0c;
									}
									if ((int)(uVar4 >> 1) <= *(int *)(iVar41 + 0x1c)) {
										uVar38 = uVar4 >> 1;
										iVar18 = 7;
										goto LAB_00131f0c;
									}
									iVar18 = 8;
									uVar38 = uVar4;
									if ((int)uVar4 <= *(int *)(iVar41 + 0x20))
										goto LAB_00131f0c;
									iVar18 = 0;
									puVar15[-1] = 0;
								}
								else {
									uVar38 = uVar4 >> 7;
									iVar18 = 1;
									LAB_00131f0c:
									puVar15[-1] = 
									*(uint16_t *)(iVar8 + (*(int *)(iVar34 + iVar18 * 4) + uVar38) * 2);
								}


								uVar4 = uVar4 + 1;
								*puVar15 = (short)iVar18;  // iVar18 is som
								iVar18 = *LOC(0x003f8a60);
								puVar15 = puVar15 + 2;
							} while (uVar4 != 0x100);  // 256


							// This is the texture data it seems
							// uses the huffman_table array 
							// called once for each object in the group
							cVar14 = *(char *)piVar37;
							if (cVar14 != -1) {
								logv_error("-----------worldAllocateSegments: cVar14: %i\n", cVar14);
								do { //  while (cVar14 != -1); try to find the end of the texture data ?
									iVar22 = (int)*(char *)((int)piVar37 + 1);
									iVar16 = (int)cVar14;
									pcVar2 = (char *)((int)piVar37 + 3);
									cVar14 = *(char *)((int)piVar37 + 2);
									piVar37 = piVar37 + 1;
									iVar13 = (int)*pcVar2;
									// log result of (iVar13 - iVar22)
									logv_error("   worldAllocateSegments: (iVar13 - iVar22): %i\n", (iVar13 - iVar22));
									// log result of (cVar14 - iVar16)
									logv_error("   worldAllocateSegments: (cVar14 - iVar16): %i\n", (cVar14 - iVar16));

									if (*pcVar2 < iVar22) {
										iVar13 = iVar22;
									}

									iVar28 = 0;
									do { // (iVar28 != (iVar13 - iVar22) + 1);  // Repeat for each line ?
										if (iVar16 <= cVar14) {
											iVar17 = 0;
											do { //  while (iVar17 != (cVar14 - iVar16) + 1); // Repeat for each pixel in the line ? 
												iVar30 = *piVar37;
												iVar35 = 0;
												uVar38 = 0;
												uVar4 = 0;
												iVar31 = iVar17 + iVar16;
												// This loops 256 times
												// iVar35 is the loop counter
												// uses huffman_table 
												do { // while (iVar35 != 0x100); // 256
													puVar15 = (uint16_t *)((int)piVar7 + ((int)uVar4 >> 4) * 2 + iVar30);
													// Reads bit by bit 
													uVar1 = CONCAT22(*puVar15, puVar15[1]) << (uVar4 & 0xf);
													uVar23 = uVar1 >> 0x18; // 24, RGB?
													psVar25 = lockrect_struct.huffman_table + uVar23 * 2;
													iVar32 = (int)lockrect_struct.huffman_table[uVar23 * 2 + 1];
													if (iVar32 == 0) {
														uVar1 = uVar1 >> 0x10;
														iVar33 = 0;
														do {
															iVar32 = iVar33 * -4;
															uVar23 = iVar33 + 7;
															iVar33 = iVar33 + -1;
														} while (*(int *)(iVar41 + 0x24 + iVar32) < (int)(uVar1 >> (uVar23 & 0xff)));
														iVar32 = 8 - iVar33;
														psVar25 = (short *)(iVar8 + (*(int *)(iVar34 + 0x20 + iVar33 * -4) + (uVar1 >> (uVar23 & 0xff))) * 2);
													}
													uVar23 = (uint)*psVar25;
													if (0xff < (int)uVar23) {
														if ((int)uVar23 < 0x105) {
															pbVar26 = (uint8_t *)((int)&lockrect_struct.local_598 + iVar35 + (char)(LOC(0x000b0128))[uVar23]);
														}
														else {
															pbVar26 = (uint8_t *)(uVar23 + iVar18 + uVar38 * 8 + -0x105);
														}
														uVar23 = (uint)*pbVar26;
													}
													uVar38 = uVar23;
													uVar4 = iVar32 + uVar4;
													*(char *)((int)&lockrect_struct.local_598 + iVar35) = (char)uVar38;
													iVar35 = iVar35 + 1;
												} while (iVar35 != 0x100); // 256
												iVar17 = iVar17 + 1;
												piVar37 = piVar37 + 1;
												iVar30 = iVar31 * 0x10 + (iVar28 + iVar22) * iVar6 * 0x10;
												// iVar6 is the Width of the texture
												// iVar30 is the offset in the texture
												*(uint64_t *)((int)__newTexPtr + iVar30) = CONCAT44(lockrect_struct.uStack_594,lockrect_struct.local_598);
												((uint64_t *)((int)__newTexPtr + iVar30))[1] = lockrect_struct.uStack_590;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6);
												*puVar29 = lockrect_struct.local_588;
												puVar29[1] = lockrect_struct.uStack_580;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 2);
												*puVar29 = lockrect_struct.local_578;
												puVar29[1] = lockrect_struct.uStack_570;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 3);
												*puVar29 = lockrect_struct.local_568;
												puVar29[1] = lockrect_struct.uStack_560;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 4);
												*puVar29 = lockrect_struct.local_558;
												puVar29[1] = lockrect_struct.uStack_550;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 5);
												*puVar29 = lockrect_struct.local_548;
												puVar29[1] = lockrect_struct.uStack_540;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 6);
												*puVar29 = lockrect_struct.local_538;
												puVar29[1] = lockrect_struct.uStack_530;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 7);
												*puVar29 = lockrect_struct.local_528;
												puVar29[1] = lockrect_struct.uStack_520;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 8);
												*puVar29 = lockrect_struct.local_518;
												puVar29[1] = lockrect_struct.uStack_510;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 9);
												*puVar29 = lockrect_struct.local_508;
												puVar29[1] = lockrect_struct.uStack_500;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 10);
												*puVar29 = lockrect_struct.local_4f8;
												puVar29[1] = lockrect_struct.uStack_4f0;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 0xb);
												*puVar29 = lockrect_struct.local_4e8;
												puVar29[1] = lockrect_struct.uStack_4e0;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 0xc);
												*puVar29 = lockrect_struct.local_4d8;
												puVar29[1] = lockrect_struct.uStack_4d0;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 0xd);
												*puVar29 = lockrect_struct.local_4c8;
												puVar29[1] = lockrect_struct.uStack_4c0;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 0xe);
												*puVar29 = lockrect_struct.local_4b8;
												puVar29[1] = lockrect_struct.uStack_4b0;
												puVar29 = (uint64_t *)((int)__newTexPtr + iVar30 + iVar6 * 0xf);
												*puVar29 = lockrect_struct.local_4a8;
												puVar29[1] = lockrect_struct.uStack_4a0;
											} while (iVar17 != (cVar14 - iVar16) + 1);
										}
										iVar28 = iVar28 + 1;
									} while (iVar28 != (iVar13 - iVar22) + 1);
									cVar14 = *(char *)piVar37;
								} while (cVar14 != -1);
							}


							lockLoadingMutex(true);
							iVar34 = 0;
							iVar8 = *(int *)(iVar19 + iVar3 * 8);

							pallette = D3DDevice_CreatePalette2(0);
							iVar6 = D3DPalette_Lock2(pallette,0);
							do { // 1024 : 256 * RGBA = Initialize the 256 palette RGBA colors
								pbVar26 = (uint8_t *)((int)piVar7 + iVar34 + iVar9);
								fVar40 = (float)(long long)(int)((uint)pbVar26[1] + (uint)*pbVar26 + (uint)pbVar26[2]) * 0.3333333;
								iVar41 = (int)(fVar40 + ((float)(unsigned long)(uint)pbVar26[2] - fVar40) * 1.3 + 0.5);
								iVar13 = (int)(fVar40 + ((float)(unsigned long)(uint)*pbVar26 - fVar40) * 1.3 + 0.5);
								iVar18 = (int)(fVar40 + ((float)(unsigned long)(uint)pbVar26[1] - fVar40) * 1.3 + 0.5);
								
								// uVar4 = UnsignedSaturate(iVar41,8);
								// UnsignedDoesSaturate(iVar41,8);
								// iVar41 = UnsignedSaturate(iVar13,8);
								// UnsignedDoesSaturate(iVar13,8);
								// iVar13 = UnsignedSaturate(iVar18,8);
								// UnsignedDoesSaturate(iVar18,8);

								uVar4  = UnsignedSaturate8(iVar41);
								iVar41 = UnsignedSaturate8(iVar13);
								iVar13 = UnsignedSaturate8(iVar18);

								*(uint *)(iVar6 + iVar34) = uVar4 | (uint)pbVar26[3] << 0x18 | iVar41 << 0x10 | iVar13 << 8;
								iVar34 = iVar34 + 4;
							} while (iVar34 != 0x400); // 1024 : 256 * RGBA
							iVar8 = iVar8 + local_650 * 0x38;
							// Store the palette in the world (?) object for later use
							*(uint32_t *)(iVar8 + 0x14) = pallette;

							//- 0x8B is  D3DRS_DXT1NOISEENABLE??
							//   see: https://github.com/vncloudsco/original-xbox-kernel-source/blob/e19dd436a9add10da24e832ee14a6bf857b8a3a4/public/sdk/inc/d3d8types.h#L532
							/*
								UINT                Width,
								UINT                Height,       
								UINT                Depth,          1 
								UINT                Levels,         0
								DWORD               Usage,          0x0 	is D3DUSAGE_RENDERTARGET
								D3DFORMAT           Format,         0x8B 	is D3DRS_DXT1NOISEENABLE??
								D3DRESOURCETYPE     D3DResource     3    	is D3DRTYPE_TEXTURE
							*/
							pallette = D3DDevice_CreateTexture2(texWidth, texHeight, 1, 0, 0, 0x8b, 3);
							//logv_error("worldAllocateSegments: D3DDevice_CreateTexture2 passed return value: %i\n", pallette);

							// print addressof  lockrect_struct.local_598 and lockrect_struct.uStack_594
							//logv_error("worldAllocateSegments: lockrect_struct.local_598: %p, lockrect_struct.uStack_594: %p\n", &lockrect_struct.local_598, &lockrect_struct.uStack_594);

							D3DTexture_LockRect(pallette, 0, (int*)&lockrect_struct.local_598, 0, 0);
							//print contents of lockrect_struct.local_598 and lockrect_struct.uStack_594
							logv_error("worldAllocateSegments: lockrect_struct.local_598: %i, lockrect_struct.uStack_594: %p\n", lockrect_struct.local_598, lockrect_struct.uStack_594);
							//int lockedRect[2];
							//D3DTexture_LockRect(pallette, 0, lockedRect, NULL, 0);
							// int pitch    = lockedRect[0];
							// int ptrValue = lockedRect[1];

							//log_error("worldAllocateSegments: D3DTexture_LockRect passed \n");
							//logv_error("Will call __aeabi_memcpy with totalPixels: %i, __newTexPtr: %p, lockrect_struct.uStack_594: %p\n", totalPixels, __newTexPtr, lockrect_struct.uStack_594);
							__aeabi_memcpy(lockrect_struct.uStack_594, __newTexPtr, totalPixels);
							//__aeabi_memcpy((void*)ptrValue, __newTexPtr, totalPixels);

							//log_error("worldAllocateSegments: __aeabi_memcpy passed \n");
							// // Pseudocode for a row-by-row copy:
							// uint8_t* dst = (uint8_t*) lockrect_struct.uStack_594;   // from LockRect
							// const uint8_t* src = (uint8_t*) __newTexPtr;  // your CPU buffer
							// int pitch = lockrect_struct.local_598;                 // from LockRect
							// int rows = actualHeight;               // or however you track it
							// int rowBytes = actualWidth;            // 1 byte per texel if palettized

							// for (int y = 0; y < rows; ++y) {
							// 	memcpy(dst, src, rowBytes);
							// 	dst += pitch;
							// 	src += rowBytes;
							// }

							D3DTexture_UnlockRect(pallette,0);
							*(uint32_t *)(iVar8 + 0x10) = pallette;
							releaseLoadingMutex();
							local_650 = local_650 + 1;
							piVar7 = piVar7 + 0xe;
						} while (local_650 != iVar20);
					}
				}
				iVar3 = iVar3 + 1;
			} while (iVar3 != (iVar36 - iVar27) + 1);
			free(pTexFile);
			if (__newTexPtr != (void *)0x0) {
				free(__newTexPtr);
			}
		}
	}
}


so_hook worldAllocateSegments_hook;

void so_patch(void) {

	log_error("Patching .so functions\n");

	
	machHostSeek = (void (*)(int, int, int))so_symbol(&so_mod, "_Z12machHostSeekiii");
	machHostRead = (void (*)(int, void *, int))so_symbol(&so_mod, "_Z12machHostReadiPvi");
	machHostClose = (void (*)(int))so_symbol(&so_mod, "_Z13machHostClosei");
	lockLoadingMutex = (void (*)(bool))so_symbol(&so_mod, "_Z16lockLoadingMutexb");
	releaseLoadingMutex = (void (*)(void))so_symbol(&so_mod, "_Z19releaseLoadingMutexv");
	D3DDevice_CreatePalette2 = (uint32_t (*)(int))so_symbol(&so_mod, "D3DDevice_CreatePalette2");
	D3DPalette_Lock2 = (int (*)(uint32_t, int))so_symbol(&so_mod, "D3DPalette_Lock2");
	D3DDevice_CreateTexture2 = (uint32_t (*)(int, int, int, int, int, int, int))so_symbol(&so_mod, "D3DDevice_CreateTexture2");
	//_ZNK14D3DBaseTexture7GetInfoER10_D3DFORMATRiS2_RmS3_
	D3DBaseTexture_GetInfo = (void (*)(void *, int, int *, int *, int *, int *))so_symbol(&so_mod, "_ZNK14D3DBaseTexture7GetInfoER10_D3DFORMATRiS2_RmS3_");

	// void D3DTexture_LockRect(D3DTexture *pThis, UINT Level, D3DLOCKED_RECT *pLockedRect, CONST RECT *pRect, DWORD Flags);
	D3DTexture_LockRect = (void (*)(void *, int, int *, void *, int))so_symbol(&so_mod, "D3DTexture_LockRect");
	if (D3DTexture_LockRect == NULL) {
		log_error("D3DTexture_LockRect not found\n");
	} else {
		logv_error("D3DTexture_LockRect found at %p\n", D3DTexture_LockRect);
	}
	// void D3DTexture_UnlockRect(D3DTexture *pThis, UINT Level)
	D3DTexture_UnlockRect = (void (*)(uint32_t*, uint32_t))so_symbol(&so_mod, "D3DTexture_UnlockRect");

	// _Z21worldAllocateSegmentsP12_worldHeader
	//orldAllocateSegments_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z21worldAllocateSegmentsP12_worldHeader"), (uintptr_t)&worldAllocateSegments);

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

	//_Z25lowestPowerof2NotLessThani
	uintptr_t lowestPowerof2NotLessThan_addr = (uintptr_t)so_symbol(&so_mod, "_Z25lowestPowerof2NotLessThani");
	if (lowestPowerof2NotLessThan_addr == NULL) {
		log_error("lowestPowerof2NotLessThan not found\n");
	} else {
		logv_error("lowestPowerof2NotLessThan found at %p\n", lowestPowerof2NotLessThan_addr);
		lowestPowerof2NotLessThan_hook = hook_addr(lowestPowerof2NotLessThan_addr, (uintptr_t)&lowestPowerof2NotLessThan);
	}



	// D3DTexture_LockRect
	uintptr_t d3d_texture_lock_rect_addr = (uintptr_t)so_symbol(&so_mod, "D3DTexture_LockRect");
	if (d3d_texture_lock_rect_addr == NULL) {
		log_error("D3DTexture_LockRect not found\n");
	} else {
		logv_error("D3DTexture_LockRect found at %p\n", d3d_texture_lock_rect_addr);
		d3d_texture_lock_rect_hook = hook_addr(d3d_texture_lock_rect_addr, (uintptr_t)&d3d_texture_lock_rect);
	}

	// _ZN3JBE7AudioPF12StreamThread10ThreadFuncEv
	uintptr_t audio_thread_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE7AudioPF12StreamThread10ThreadFuncEv");
	if (audio_thread_addr == NULL) {
		log_error("audio_thread not found\n");
	} else {
		logv_error("audio_thread found at %p\n", audio_thread_addr);
		//audio_thread_hook = hook_addr(audio_thread_addr, (uintptr_t)&audio_thread);
	}

	// XGSetTextureHeader
	// undefined4 XGSetTextureHeader(uint param_1,uint param_2,int param_3,uint param_4,uint param_5,undefined4 param_6,uint *param_7,uint param_8,uint param_9)
	uintptr_t xg_set_texture_header_addr = (uintptr_t)so_symbol(&so_mod, "XGSetTextureHeader");
	if (xg_set_texture_header_addr == NULL) {
		log_error("XGSetTextureHeader not found\n");
	} else {
		logv_error("XGSetTextureHeader found at %p\n", xg_set_texture_header_addr);
		xg_set_texture_header_hook = hook_addr(xg_set_texture_header_addr, (uintptr_t)&xg_set_texture_header);
	}

	// _ogg_malloc
	uintptr_t ogg_malloc_addr = (uintptr_t)so_symbol(&so_mod, "_ogg_malloc");
	if (ogg_malloc_addr == NULL) {
		log_error("ogg_malloc not found\n");
	} else {
		logv_error("ogg_malloc found at %p\n", ogg_malloc_addr);
		//ogg_alloc_hook = hook_addr(ogg_malloc_addr, (uintptr_t)&ogg_alloc);
	}

	//_Z14worldFreeWorldP12_worldHeader
	uintptr_t worldFreeWorld_addr = (uintptr_t)so_symbol(&so_mod, "_Z14worldFreeWorldP12_worldHeader");
	if (worldFreeWorld_addr == NULL) {
		log_error("worldFreeWorld not found\n");
	} else {
		logv_error("worldFreeWorld found at %p\n", worldFreeWorld_addr);
		//worldFreeWorld_hook = hook_addr(worldFreeWorld_addr, (uintptr_t)&worldFreeWorld);
	}

	// _Z17writeConfigDirectv
	uintptr_t writeConfigDirect_addr = (uintptr_t)so_symbol(&so_mod, "_Z17writeConfigDirectv");
	if (writeConfigDirect_addr == NULL) {
		log_error("writeConfigDirect not found\n");
	} else {
		logv_error("writeConfigDirect found at %p\n", writeConfigDirect_addr);
		writeConfigDirect_hook = hook_addr(writeConfigDirect_addr, (uintptr_t)&writeConfigDirect);
	}


	//D3DDevice_Swap
	uintptr_t swap_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_Swap");
	if (swap_addr == NULL) {
		log_error("D3DDevice_Swap not found\n");
	} else {
		logv_error("D3DDevice_Swap found at %p\n", swap_addr);
		//d3dDevice_swap_hook = hook_addr(swap_addr, (uintptr_t)&d3dDevice_swap);
	}

	// _ZN3JBE9D3DDevice11ReadCommandEv
	uintptr_t d3dDeviceReadCommand_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice11ReadCommandEv");
	if (d3dDeviceReadCommand_addr == NULL) {
		log_error("d3dDeviceReadCommand not found\n");
	} else {
		logv_error("d3dDeviceReadCommand found at %p\n", d3dDeviceReadCommand_addr);
		//d3dDeviceReadCommand_hook = hook_addr(d3dDeviceReadCommand_addr, (uintptr_t)&d3dDeviceReadCommand);
	}

	// _Z9lumpCleari
	uintptr_t lumpClear_addr = (uintptr_t)so_symbol(&so_mod, "_Z9lumpCleari");
	if (lumpClear_addr == NULL) {
		log_error("lumpClear not found\n");
	} else {
		logv_error("lumpClear found at %p\n", lumpClear_addr);
		//lumpClear_hook = hook_addr(lumpClear_addr, (uintptr_t)&lumpClear);
	}

	// _Z18lumpSetDiscardablePKc
	uintptr_t lump_set_discardable_addr = (uintptr_t)so_symbol(&so_mod, "_Z18lumpSetDiscardablePKc");
	if (lump_set_discardable_addr == NULL) {
		log_error("lump_set_discardable not found\n");
	} else {
		logv_error("lump_set_discardable found at %p\n", lump_set_discardable_addr);
		//lump_set_discardable_hook = hook_addr(lump_set_discardable_addr, (uintptr_t)&lump_set_discardable);
	}

	// _ZN3JBE5Input6RenderEv
	uintptr_t inputRender_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE5Input6RenderEv");
	if (inputRender_addr == NULL) {
		log_error("inputRender not found\n");
	} else {
		logv_error("inputRender found at %p\n", inputRender_addr);
		inputRender_hook = hook_addr(inputRender_addr, (uintptr_t)&inputRender);
	}

	// _ZN15VirtualControlsC2Ev
	uintptr_t virtualControlsCtor_addr = (uintptr_t)so_symbol(&so_mod, "_ZN15VirtualControlsC2Ev");
	if (virtualControlsCtor_addr == NULL) {
		log_error("virtualControlsCtor not found\n");
	} else {
		logv_error("virtualControlsCtor found at %p\n", virtualControlsCtor_addr);
		//virtualControlsCtor_hook = hook_addr(virtualControlsCtor_addr, (uintptr_t)&virtualControlsCtor);
	}

	// D3DDevice_SetGammaRamp
	uintptr_t d3d_set_gamma_ramp_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_SetGammaRamp");
	if (d3d_set_gamma_ramp_addr == NULL) {
		log_error("D3DDevice_SetGammaRamp not found\n");
	} else {
		logv_error("D3DDevice_SetGammaRamp found at %p\n", d3d_set_gamma_ramp_addr);
		//d3d_set_gamma_ramp_hook = hook_addr(d3d_set_gamma_ramp_addr, (uintptr_t)&d3d_set_gamma_ramp);
	}

	// _ZN15VirtualControls6RenderEv
	uintptr_t virtualControlsRender_addr = (uintptr_t)so_symbol(&so_mod, "_ZN15VirtualControls6RenderEv");
	if (virtualControlsRender_addr == NULL) {
		log_error("virtualControlsRender not found\n");
	} else {
		logv_error("virtualControlsRender found at %p\n", virtualControlsRender_addr);
		virtualControlsRender_hook = hook_addr(virtualControlsRender_addr, (uintptr_t)&virtualControlsRender);
	}

	// _ZN14CommonControls12UpdateCheatsEv
	uintptr_t updateCheats_addr = (uintptr_t)so_symbol(&so_mod, "_ZN14CommonControls12UpdateCheatsEv");
	if (updateCheats_addr == NULL) {
		log_error("updateCheats not found\n");
	} else {
		logv_error("updateCheats found at %p\n", updateCheats_addr);
		//updateCheats_hook = hook_addr(updateCheats_addr, (uintptr_t)&updateCheats);
	}

	// _Z31frontEndDoControllerScreenInputRiS_
	uintptr_t frontEndDoControllerScreenInput_addr = (uintptr_t)so_symbol(&so_mod, "_Z31frontEndDoControllerScreenInputRiS_");
	if (frontEndDoControllerScreenInput_addr == NULL) {
		log_error("frontEndDoControllerScreenInput not found\n");
	} else {
		logv_error("frontEndDoControllerScreenInput found at %p\n", frontEndDoControllerScreenInput_addr);
		frontEndDoControllerScreenInput_hook = hook_addr(frontEndDoControllerScreenInput_addr, (uintptr_t)&frontEndDoControllerScreenInput);
	}

	// _ZN14CommonControls16UsingTouchscreenEv
	uintptr_t usingTouchscreen_addr = (uintptr_t)so_symbol(&so_mod, "_ZN14CommonControls16UsingTouchscreenEv");
	if (usingTouchscreen_addr == NULL) {
		log_error("usingTouchscreen not found\n");
	} else {
		logv_error("usingTouchscreen found at %p\n", usingTouchscreen_addr);
		usingTouchscreen_hook = hook_addr(usingTouchscreen_addr, (uintptr_t)&usingTouchscreen);
	}

	// _ZN3JBE6System6UpdateEv
	uintptr_t systemUpdate_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE6System6UpdateEv");
	if (systemUpdate_addr == NULL) {
		log_error("systemUpdate not found\n");
	} else {
		logv_error("systemUpdate found at %p\n", systemUpdate_addr);
		//systemUpdate_hook = hook_addr(systemUpdate_addr, (uintptr_t)&systemUpdate);
	}

	// _ZN3JBE15TouchController6UpdateERKiji
	uintptr_t touchControllerUpdate_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE15TouchController6UpdateERKiji");
	if (touchControllerUpdate_addr == NULL) {
		log_error("touchControllerUpdate not found\n");
	} else {
		logv_error("touchControllerUpdate found at %p\n", touchControllerUpdate_addr);
		//touchControllerUpdate_hook = hook_addr(touchControllerUpdate_addr, (uintptr_t)&touchControllerUpdate);
	}

	//_ZN14CommonControls16RenderTouchIconsEP4Menu
	uintptr_t renderTouchIcons_addr = (uintptr_t)so_symbol(&so_mod, "_ZN14CommonControls16RenderTouchIconsEP4Menu");
	if (renderTouchIcons_addr == NULL) {
		log_error("renderTouchIcons not found\n");
	} else {
		logv_error("renderTouchIcons found at %p\n", renderTouchIcons_addr);
		renderTouchIcons_hook = hook_addr(renderTouchIcons_addr, (uintptr_t)&renderTouchIcons);
	}

	// D3DBaseTexture *D3DDevice_CreateTexture2(int param_1,int param_2,undefined4 param_3,int param_4,uint param_5,undefined4 param_6)
	uintptr_t d3d_create_texture2_addr = (uintptr_t)so_symbol(&so_mod, "D3DDevice_CreateTexture2");
	if (d3d_create_texture2_addr == NULL) {
		log_error("D3DDevice_CreateTexture2 not found\n");
	} else {
		logv_error("D3DDevice_CreateTexture2 found at %p\n", d3d_create_texture2_addr);
		d3d_create_texture2_hook = hook_addr(d3d_create_texture2_addr, (uintptr_t)&d3d_create_texture2);
	}

	// uint D3DResource_Release(uint *param_1)
	// uintptr_t d3d_resource_release_addr = (uintptr_t)so_symbol(&so_mod, "D3DResource_Release");
	// if (d3d_resource_release_addr == NULL) {
	// 	log_error("D3DResource_Release not found\n");
	// } else {
	// 	logv_error("D3DResource_Release found at %p\n", d3d_resource_release_addr);
	// 	d3d_resource_release_hook = hook_addr(d3d_resource_release_addr, (uintptr_t)&d3d_resource_release);
	// }

	// // _ZN3JBE9D3DDevice19DeleteResourceAsyncEm
	// uintptr_t d3d_delete_resource_async_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE9D3DDevice19DeleteResourceAsyncEm");
	// if (d3d_delete_resource_async_addr == NULL) {
	// 	log_error("D3DDevice_DeleteResourceAsync not found\n");
	// } else {
	// 	logv_error("D3DDevice_DeleteResourceAsync found at %p\n", d3d_delete_resource_async_addr);
	// 	d3d_delete_resource_async_hook = hook_addr(d3d_delete_resource_async_addr, (uintptr_t)&d3d_delete_resource_async);
	// }


	// _Z12machHostOpenPKcS0_
	uintptr_t machHostOpen_addr = (uintptr_t)so_symbol(&so_mod, "_Z12machHostOpenPKcS0_");
	if (machHostOpen_addr == NULL) {
		log_error("machHostOpen not found\n");
	} else {
		logv_error("machHostOpen found at %p\n", machHostOpen_addr);
		machHostOpen_hook = hook_addr(machHostOpen_addr, (uintptr_t)&machHostOpen);
	}


	

	// _ZN3JBE8SystemPF7HasNEONEv
	uintptr_t hasNEON_addr = (uintptr_t)so_symbol(&so_mod, "_ZN3JBE8SystemPF7HasNEONEv");
	if (hasNEON_addr == NULL) {
		log_error("hasNEON not found\n");
	} else {
		logv_error("hasNEON found at %p\n", hasNEON_addr);
		//hasNEON_hook = hook_addr(hasNEON_addr, (uintptr_t)&hasNEON);
	}

	//_Z13gameLoadWorldPc
	uintptr_t gameLoadWorld_addr = (uintptr_t)so_symbol(&so_mod, "_Z13gameLoadWorldPc");
	if (gameLoadWorld_addr == NULL) {
		log_error("gameLoadWorld not found\n");
	} else {
		logv_error("gameLoadWorld found at %p\n", gameLoadWorld_addr);
		//gameLoadWorld_hook = hook_addr(gameLoadWorld_addr, (uintptr_t)&gameLoadWorld);
	}

	// _Z28objectDrawDelayedDrawObjectsv
	uintptr_t objectDrawDelayedDrawObjects_addr = (uintptr_t)so_symbol(&so_mod, "_Z28objectDrawDelayedDrawObjectsv");
	if (objectDrawDelayedDrawObjects_addr == NULL) {
		log_error("objectDrawDelayedDrawObjects not found\n");
	} else {
		logv_error("objectDrawDelayedDrawObjects found at %p\n", objectDrawDelayedDrawObjects_addr);
		//objectDrawDelayedDrawObjects_hook = hook_addr(objectDrawDelayedDrawObjects_addr, (uintptr_t)&objectDrawDelayedDrawObjects);
	}

	// _Z9SND_Framev
	uintptr_t SND_Frame_addr = (uintptr_t)so_symbol(&so_mod, "_Z9SND_Framev");
	if (SND_Frame_addr == NULL) {
		log_error("SND_Frame not found\n");
	} else {
		logv_error("SND_Frame found at %p\n", SND_Frame_addr);
		SND_Frame_hook = hook_addr(SND_Frame_addr, (uintptr_t)&SND_Frame);
	}

	// _Z9animFramev
	uintptr_t animFrame_addr = (uintptr_t)so_symbol(&so_mod, "_Z9animFramev");
	if (animFrame_addr == NULL) {
		log_error("animFrame not found\n");
	} else {
		logv_error("animFrame found at %p\n", animFrame_addr);
		//animFrame_hook = hook_addr(animFrame_addr, (uintptr_t)&animFrame);
	}

	//_Z9cdProcessi
	uintptr_t cdProcess_addr = (uintptr_t)so_symbol(&so_mod, "_Z9cdProcessi");
	if (cdProcess_addr == NULL) {
		log_error("cdProcess not found\n");
	} else {
		logv_error("cdProcess found at %p\n", cdProcess_addr);
		//cdProcess_hook = hook_addr(cdProcess_addr, (uintptr_t)&cdProcess);
	}

	// _Z20texProcessDecompressv
	uintptr_t texProcessDecompress_addr = (uintptr_t)so_symbol(&so_mod, "_Z20texProcessDecompressv");
	if (texProcessDecompress_addr == NULL) {
		log_error("texProcessDecompress not found\n");
	} else {
		logv_error("texProcessDecompress found at %p\n", texProcessDecompress_addr);
		//texProcessDecompress_hook = hook_addr(texProcessDecompress_addr, (uintptr_t)&texProcessDecompress);
	}

	// _Z21worldPlotRouteProcessi
	uintptr_t worldPlotRouteProcess_addr = (uintptr_t)so_symbol(&so_mod, "_Z21worldPlotRouteProcessi");
	if (worldPlotRouteProcess_addr == NULL) {
		log_error("worldPlotRouteProcess not found\n");
	} else {
		logv_error("worldPlotRouteProcess found at %p\n", worldPlotRouteProcess_addr);
		//worldPlotRouteProcess_hook = hook_addr(worldPlotRouteProcess_addr, (uintptr_t)&worldPlotRouteProcess);
	}

	// 

	//av_log
	uintptr_t av_log_addr = (uintptr_t)so_symbol(&so_mod_libxmv, "av_log");
	if (av_log_addr == NULL) {
		log_error("av_log not found\n");
	} else {
		logv_error("av_log found at %p\n", av_log_addr);
		//av_log_hook = hook_addr(av_log_addr, (uintptr_t)&av_log);
	}

	// _Z8gameLoopv
	uintptr_t gameLoop_addr = (uintptr_t)so_symbol(&so_mod, "_Z8gameLoopv");
	if (gameLoop_addr == NULL) {
		log_error("gameLoop not found\n");
	} else {
		logv_error("gameLoop found at %p\n", gameLoop_addr);
		//gameLoop_hook = hook_addr(gameLoop_addr, (uintptr_t)&gameLoop);
	}

	// _Z14machFrameStartv
	uintptr_t machFrameStart_addr = (uintptr_t)so_symbol(&so_mod, "_Z14machFrameStartv");
	if (machFrameStart_addr == NULL) {
		log_error("machFrameStart not found\n");
	} else {
		logv_error("machFrameStart found at %p\n", machFrameStart_addr);
		//machFrameStart_hook = hook_addr(machFrameStart_addr, (uintptr_t)&machFrameStart);
	}

	// machFrameEnd
	uintptr_t machFrameEnd_addr = (uintptr_t)so_symbol(&so_mod, "_Z12machFrameEndi");
	if (machFrameEnd_addr == NULL) {
		log_error("machFrameEnd not found\n");
	} else {
		logv_error("machFrameEnd found at %p\n", machFrameEnd_addr);
		//machFrameEnd_hook = hook_addr(machFrameEnd_addr, (uintptr_t)&machFrameEnd);
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

 	// undefined4 XGBytesPerPixelFromFormat(undefined4 param_1)
	uintptr_t XGBytesPerPixelFromFormat_addr = (uintptr_t)so_symbol(&so_mod, "XGBytesPerPixelFromFormat");
	if (XGBytesPerPixelFromFormat_addr == NULL) {
		log_error("XGBytesPerPixelFromFormat not found\n");
	} else {
		logv_error("XGBytesPerPixelFromFormat found at %p\n", XGBytesPerPixelFromFormat_addr);
		//xg_bytes_per_pixel_from_format_hook = hook_addr(XGBytesPerPixelFromFormat_addr, (uintptr_t)&xg_bytes_per_pixel_from_format);
	}



	// Patch the initial heap allocation of the game to use 0x2800000 bytes instead of 0x4000000

	// uintptr_t addressToPatch = so_mod.text_base + 0x001827f4 - 0x00010000;
	// // Print the next 4 bytes to see if we're at the right place
	// logv_error("Bytes at %p: %x %x %x %x\n", addressToPatch, *(uint8_t *)addressToPatch, *(uint8_t *)(addressToPatch + 1), *(uint8_t *)(addressToPatch + 2), *(uint8_t *)(addressToPatch + 3));
	// // Patch the bytes with 05 04 A0 E3
	// kuKernelCpuUnrestrictedMemcpy((void *)addressToPatch, "\x0A\x05\xA0\xE3", 4);
	// // Print the new bytes to see if it was patched correctly
	// logv_error("Patched bytes at %p: %x %x %x %x\n", addressToPatch, *(uint8_t *)addressToPatch, *(uint8_t *)(addressToPatch + 1), *(uint8_t *)(addressToPatch + 2), *(uint8_t *)(addressToPatch + 3));

	// addressToPatch = so_mod.text_base + 0x001827fc - 0x00010000;
	// // Print the next 4 bytes to see if we're at the right place
	// logv_error("Bytes at %p: %x %x %x %x\n", addressToPatch, *(uint8_t *)addressToPatch, *(uint8_t *)(addressToPatch + 1), *(uint8_t *)(addressToPatch + 2), *(uint8_t *)(addressToPatch + 3));
	// // Patch the bytes with 05 14 A0 E3
	// kuKernelCpuUnrestrictedMemcpy((void *)addressToPatch, "\x0A\x15\xA0\xE3", 4);
	// // Print the new bytes to see if it was patched correctly
	// logv_error("Patched bytes at %p: %x %x %x %x\n", addressToPatch, *(uint8_t *)addressToPatch, *(uint8_t *)(addressToPatch + 1), *(uint8_t *)(addressToPatch + 2), *(uint8_t *)(addressToPatch + 3));
	


	uintptr_t addresses[] = {
		0x0009caf1,
		0x000a326a,
		0x000a1733,
		0x0009b060,
		0x000a0523,
		0x000a9bde,
		0x000a327c,
		0x0009e89e,
		0x0009dda3,
		0x000a4da4,
		0x000a7e89,
		0x0009cb02,
		0x0009dd92,
		0x000ac2b3,
		0x000ab86c,
		0x0009e8b5,
		0x000a328c,
		0x000a2a2c,
		0x000a1745,
		0x0009b044,
		0x0009b9ae,
		0x0009f9bb,
		//0x000a61ab, // "arrowA.tex",
		0x000ab861,  // "arrowB.tex"
		0x000a6a40,  // cancelButtonA.tex
		0x000ad677,   // cancelButtonB.tex
		//0x000a3fa2,	// legal2.tex	
		//0x00a3a91	//frontendmenulong.tex	"frontendmenulong.tex"	ds
	};

	char* stringToPatch = "arrowA.tex";
	for(int i = 0; i < sizeof(addresses) / sizeof(uintptr_t); i++) {
		uintptr_t addressToPatch = so_mod.text_base + addresses[i] - 0x00010000;
		
		// print original string
		logv_error("Original string at %p: %s\n", addressToPatch, (char *)addressToPatch);
		kuKernelCpuUnrestrictedMemcpy((void *)addressToPatch, stringToPatch, strlen(stringToPatch)+1);
		// print new string
		logv_error("Patched string at %p: %s\n", addressToPatch, (char *)addressToPatch);
	}
}

void patch_address_with_string(uintptr_t address, char *string) {
	// print original string
	logv_error("Original string at %p: %s\n", address, (char *)address);
	kuKernelCpuUnrestrictedMemcpy((void *)address, string, strlen(string));
	// print new string
	logv_error("Patched string at %p: %s\n", address, (char *)address);
}
