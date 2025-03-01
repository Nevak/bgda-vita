/*
 * main.c
 *
 * ARMv7 Shared Libraries loader. Soulcalibur Edition
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021-2023 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/init.h"
#include "utils/glutil.h"
#include "utils/settings.h"
#include "utils/logger.h"

#include <psp2/kernel/threadmgr.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>

#include <AFakeNative/AFakeNative.h>
#include <vitasdk.h>
#include <stdio.h>
#include <psp2/gxm.h>

int _newlib_heap_size_user = 256 * 1024 * 1024;

#ifdef USE_SCELIBC_IO
int sceLibcHeapSize = 24 * 1024 * 1024;
#endif

so_module so_mod;
so_module so_mod_libcpufeatues;
so_module so_mod_jbejni;
so_module so_mod_libcpp;
so_module so_mod_libxmv;

int main() {
	SceAppUtilInitParam appUtilParam;
	SceAppUtilBootParam appUtilBootParam;
	memset(&appUtilParam, 0, sizeof(SceAppUtilInitParam));
	memset(&appUtilBootParam, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&appUtilParam, &appUtilBootParam);
	
	soloader_init_all();


    int (*JNI_OnLoad)(JavaVM* jvm) = (void*)so_symbol(&so_mod_jbejni, "JNI_OnLoad");

	int (*ANativeActivity_onCreate)(ANativeActivity *activity, void *savedState,
		size_t savedStateSize) = (void *) so_symbol(&so_mod_jbejni, "ANativeActivity_onCreate");

	ANativeActivity *activity = ANativeActivity_create();

	log_info("Created NativeActivity object");

	ANativeActivity_onCreate(activity, NULL, 0);
	log_info("ANativeActivity_onCreate() passed");

	activity->callbacks->onStart(activity);
	log_info("onStart() passed");

	AInputQueue *aInputQueue = AInputQueue_create();
	activity->callbacks->onInputQueueCreated(activity, aInputQueue);
	log_info("onInputQueueCreated() passed");

	JNI_OnLoad(&jvm);

	ANativeWindow *aNativeWindow = ANativeWindow_create();
	activity->callbacks->onNativeWindowCreated(activity, aNativeWindow);
	log_info("onNativeWindowCreated() passed");

	activity->callbacks->onWindowFocusChanged(activity, 1);
	log_info("onWindowFocusChanged() passed");

	log_info("Main  thread shutting down");


	//JBE_android_main_sub(NULL);
	

	// /* JBEMain(int, char const**) */
	// int (* JBEMain)(int, char const**) = (void *) so_symbol(&so_mod, "JBEMain");
	
	// /*void JBE_System_Printf(char *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)*/
	// void (* JBE_System_Printf)(char *, int, int, int) = (void *) so_symbol(&so_mod, "JBE_System_Printf");

	// //JBE_System_Printf("Hello from main.c\n", 0, 0, 0);


	//JBEMain(0, NULL);
	

	// int (*ANativeActivity_onCreate)(ANativeActivity *activity, void *savedState, size_t savedStateSize) = (void *) so_symbol(&so_mod, "ANativeActivity_onCreate");
	// ANativeActivity_onCreate(activity, NULL, 0);
	
	// log_info("ANativeActivity_onCreate() passed");
	// void JBE_android_main_sub(android_app *param_1)
	// void (* JBE_android_main_sub)(void *) = (void *) so_symbol(&so_mod, "JBE_android_main_sub");
	// if (JBE_android_main_sub == NULL)
	// {
	// 	log_error("JBE_android_main_sub is NULL");
	// }
	// else
	// {
	//   	log_info("JBE_android_main_sub is not NULL");
	//   	JBE_android_main_sub(activity);
	// }

	// log_info("JBE_android_main_sub() passed");



	// log_info("JBEMain333() passed");

	// activity->callbacks->onStart(activity);
	// log_info("onStart() passed");

	// AInputQueue *aInputQueue = AInputQueue_create();
	// activity->callbacks->onInputQueueCreated(activity, aInputQueue);
	// log_info("onInputQueueCreated() passed");

	//  ANativeWindow *aNativeWindow = ANativeWindow_create();
	//  activity->callbacks->onNativeWindowCreated(activity, aNativeWindow);
	//  log_info("onNativeWindowCreated() passed");

	// activity->callbacks->onWindowFocusChanged(activity, 1);
	// log_info("onWindowFocusChanged() passed");

	//_ZN3JBE8SystemPF13SetAndroidAppEP11android_app
	/* JBE::SystemPF::SetAndroidApp(android_app*) */





	// void (* JBE_SystemPF_SetAndroidApp)(void *) = (void *) so_symbol(&so_mod, "_ZN3JBE8SystemPF13SetAndroidAppEP11android_app");
	// if (JBE_SystemPF_SetAndroidApp == NULL)
	// {
	// 	log_error("JBE_SystemPF_SetAndroidApp is NULL");
	// }
	// else
	// {
	// 	log_info("JBE_SystemPF_SetAndroidApp is not NULL");
	// 	JBE_SystemPF_SetAndroidApp(activity->instance);
	// }

	// log_info("JBE_SystemPF_SetAndroidApp() passed");


	// int (* JBEStartup)(void) = (void *) so_symbol(&so_mod, "_Z10JBEStartupv");
	// if (JBEStartup == NULL)
	// {
	// 	log_error("JBEStartup is NULL");
	// }
	// else
	// {
	// 	log_info("JBEStartup is not NULL");
	// 	JBEStartup();
	// }

	// log_info("JBEStartup() passed");


	// // void JBEMain(int param_1,char **param_2)
	// void (* JBEMain)(int, char **) = (void *) so_symbol(&so_mod, "_Z7JBEMainiPPKc");
	// if (JBEMain == NULL)
	// {
	// 	log_error("JBEMain is NULL");
	// }
	// else
	// {
	// 	log_info("JBEMain is not NULL");
	// 	char *argv[] = { "soulcalibur", NULL };
	// 	JBEMain(1, argv);
	// }


	//_Z11wrappedMainiPPc
	// void (* wrappedMain)(int, char **) = (void *) so_symbol(&so_mod, "_Z11wrappedMainiPPc");
	// if (wrappedMain == NULL)
	// {
	// 	log_error("wrappedMain is NULL");
	// }
	// else
	// {
	// 	log_info("wrappedMain is not NULL");
	// 	char *argv[] = { "soulcalibur", NULL };
	// 	wrappedMain(1, argv);
	// }

	// log_info("wrappedMain() passed");

	// log_info("Main thread shutting down");
/*
	uint8_t * DAT_0033a95c = (uint8_t *)(so_mod.text_base + 0x0033a95c);
	uint8_t * DAT_0033a96c = (uint8_t *)(so_mod.text_base + 0x0033a96c);
	uint8_t * DAT_0033a94c = (uint8_t *)(so_mod.text_base + 0x0033a94c);

	while (true) {
		sceClibPrintf("flags: DAT_0033a95c:%i(exp 1); DAT_0033a96c:%i(exp 0); DAT_0033a94c:%i(exp 1)\n", *DAT_0033a95c, *DAT_0033a96c, *DAT_0033a94c);
		sceKernelDelayThread(500000);
	}*/

	sceKernelExitDeleteThread(0);
}
