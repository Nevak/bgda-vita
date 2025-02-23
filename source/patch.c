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

uint32_t *	I_HeapKaraLoop;

int S_CheckUsefulStage(int stage) {
	// Report as unlocked any not unlockable stage
	switch (stage) {
	case 8:
	case 14: // Chaos
	case 16: // Takematsu Castle (Winter)
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
		return 1;
	default:
		return SO_CONTINUE(int, stage_hook, stage);
	}
}

so_hook _machInit_hook;
so_hook _padInit_hook;
so_hook _cdInit_hook;
so_hook _emathInit_hook;
so_hook _queryPerformanceFrequency_hook;
so_hook _createFile_hook;
so_hook _direct3DCreate8_hook;
so_hook _openFile_hook;
so_hook _isES31_hook;

void exit_process() {
	sceKernelExitProcess(0);
}

int TakamatsuSummer() {
	*I_HeapKaraLoop = 1;
	return SO_CONTINUE(int, takamatsu_hook);
}

int TakamatsuWinter() {
	*I_HeapKaraLoop = 1;
	return SO_CONTINUE(int, takamatsu2_hook);
}

int lumpLoad(char *param_1) {
	printf("lumpLoad(%s)\n", param_1);
	return SO_CONTINUE(int, lumpLoad_hook, param_1);
}

void memPrintFree() {
	printf("memPrintFree()\n");
	return SO_CONTINUE(void *, memPrintFree_hook);
}

void listInit() {
	printf("listInit()\n");
	return SO_CONTINUE(void *, listInit_hook);
}

void machInit() {
	printf("machInit()\n");
	return SO_CONTINUE(void *, _machInit_hook);
}

int queryPerformanceFrequency() {
	printf("queryPerformanceFrequency()\n");
	return SO_CONTINUE(int, _queryPerformanceFrequency_hook);
}

void direct3DCreate8() {
	printf("direct3DCreate8()\n");
	return SO_CONTINUE(void *, _direct3DCreate8_hook);
}

// void createFile(char* param_1, int param_2) {
// 	printf("createFile()\n");
// 	return SO_CONTINUE(void *, _createFile_hook, param_1, param_2);
// }

void padInit() {
	printf("padInit()\n");
	return SO_CONTINUE(void *, _padInit_hook);
}

void cdInit() {
	printf("cdInit()\n");
	return SO_CONTINUE(void *, _cdInit_hook);
}

void emathInit() {
	printf("emathInit()\n");
	return SO_CONTINUE(void *, _emathInit_hook);
}

void openFile(void * thisptr, char * param_1, int param_2) {
	printf("openFile()\n");
	return SO_CONTINUE(void *, _openFile_hook, thisptr, param_1, param_2);
}

bool isES31() {
	printf("isES31()\n");
	return SO_CONTINUE(bool, _isES31_hook);
}


//MC_Init
so_hook MC_Init_hook;
int MC_Init(int param_1) {
	logv_error("MC_Init(%i)\n", param_1);
	int returnval = SO_CONTINUE(int, MC_Init_hook, param_1);
	logv_error("MC_Init returned %i\n", returnval);
	return returnval;
}

//coreAddTask
so_hook coreAddTask_hook;
int coreAddTask(void *param_1, int param_2, char *param_3) {
	logv_error("coreAddTask(%p, %i, %s)\n", param_1, param_2, param_3);
	int returnval = SO_CONTINUE(int, coreAddTask_hook, param_1, param_2, param_3);
	//logv_error("coreAddTask returned %i\n", returnval);
	return returnval;
}

//XInitCloud
so_hook XInitCloud_hook;
int XInitCloud(int param_1, void *param_2, int param_3, void *param_4, int param_5) {
	logv_error("XInitCloud(%i, %p, %i, %p, %i)\n", param_1, param_2, param_3, param_4, param_5);
	int returnval = SO_CONTINUE(int, XInitCloud_hook, param_1, param_2, param_3, param_4, param_5);
	logv_error("XInitCloud returned %i\n", returnval);
	return returnval;
}

// ShaderManager_LoadProgram
so_hook ShaderManager_LoadProgram_hook;
/* JBE::ShaderManager::LoadProgram(JBE::ShaderProgram&, JBE::ShaderManager::VertexDef const&, int,
   JBE::ShaderManager::PixelDef const&, unsigned int, int
   (*)(JBE::Container<JBE::Util::AlignedPtr<char const> >::Iterator&)) */
void ShaderManager_LoadProgram(void *thisptr, void *param_1, int param_2, void *param_3, unsigned int param_4, int (*param_5)(void *)) {
	logv_error("ShaderManager_LoadProgram(%p, %p, %i, %p, %u, %p)\n", thisptr, param_1, param_2, param_3, param_4, param_5);
	SO_CONTINUE(int, ShaderManager_LoadProgram_hook, thisptr, param_1, param_2, param_3, param_4, param_5);
	log_error("ShaderManager_LoadProgram returned \n");
}

// gameLoop
so_hook gameLoop_hook;
void gameLoop() {
	log_error("gameLoop()\n");
	SO_CONTINUE(void *, gameLoop_hook);
	log_error("gameLoop returned\n");
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



void so_patch(void) {

	log_error("Patching .so functions\n");

	// _Z7MC_Initi MC_Init
	MC_Init_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z7MC_Initi"), (uintptr_t)&MC_Init);

	// _Z11coreAddTaskPFvvEiPKc coreAddTask
	coreAddTask_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z11coreAddTaskPFvvEiPKc"), (uintptr_t)&coreAddTask);

	// _Z10XInitCloudiPFvPKvjiEPFvRPvRjEi XInitCloud
	uintptr_t XInitCloud_addr = (uintptr_t)so_symbol(&so_mod, "_Z10XInitCloudiPFvPKvjiEPFvRPvRjEi");
	if (XInitCloud_addr == NULL) {
		log_error("XInitCloud not found\n");
	} else {
		logv_error("XInitCloud found at %p\n", XInitCloud_addr);
		XInitCloud_hook = hook_addr(XInitCloud_addr, (uintptr_t)&XInitCloud);
	}
	
	// gameLoop
	uintptr_t gameLoop_addr = so_mod.text_base + 0xb296c;
	if (gameLoop_addr == NULL) {
	log_error("gameLoop not found\n");
	} else {
		logv_error("gameLoop found at %p\n", gameLoop_addr);
		gameLoop_hook = hook_addr(gameLoop_addr, (uintptr_t)&gameLoop);
	}

	// // void JBE_android_main_sub(android_app *param_1)
	// uintptr_t jbe_andoid_main_addr = (uintptr_t) so_symbol(&so_mod, "JBE_android_main_sub");
	// if (jbe_andoid_main_addr == NULL)
	// {
	// 	log_error("JBE_android_main_sub not found\n");
	// }
	// else
	// {
	// 	logv_error("JBE_android_main_sub found at %p\n", jbe_andoid_main_addr);
	// 	jbe_android_main_sub_hook = hook_addr(jbe_andoid_main_addr, (uintptr_t)&JBE_android_main_sub);
	// }


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
	// Print the original string to see if we're at the right place
	logv_error("Original sampler string: %s\n", (char *)sampler);
	// Patch the string
	kuKernelCpuUnrestrictedMemcpy((void *)sampler, "sam", 4);
	// Print the new string to see if it was patched correctly
	logv_error("Patched sampler string: %s\n", (char *)sampler);



	// _ZN3JBE13ShaderManager11LoadProgramERNS_13ShaderProgramERKNS0_9VertexDefEiRKNS0_8PixelDefEjPFiRNS_9ContainerINS_4Util10AlignedPtrIKcEEE8IteratorEE
	//ShaderManager_LoadProgram_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN3JBE13ShaderManager11LoadProgramERNS_13ShaderProgramERKNS0_9VertexDefEiRKNS0_8PixelDefEjPFiRNS_9ContainerINS_4Util10AlignedPtrIKcEEE8IteratorEE"), (uintptr_t)&ShaderManager_LoadProgram);

	//lumpLoad_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z8lumpLoadPKc"), (uintptr_t)&lumpLoad);
	// memPrintFree_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z12memPrintFreev"), (uintptr_t)&memPrintFree);
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

	// //_ZN3EXT6IsES31Ev
	// //undefined4 EXT::IsES31(void)
	// _isES31_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN3EXT6IsES31Ev"), (uintptr_t)&isES31);

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
	
	// // Unlock stages that aren't unlockable on Android port
	// stage_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z18S_CheckUsefulStagei"), (uintptr_t)&S_CheckUsefulStage);
	
	// // Prevent game from crashing when attempting to exit it
	// hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN11SoundOpenSL8shutdownEv"), (uintptr_t)&exit_process);
}

