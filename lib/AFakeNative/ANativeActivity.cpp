#include <cstdlib>
#include "ANativeActivity.h"

#include <falso_jni/FalsoJNI.h>
#include "native_app_glue.h"
#include "AConfiguration.h"

ANativeActivity * ANativeActivity_create() {
    auto * ret = (ANativeActivity *) malloc(sizeof(ANativeActivity));
    ret->callbacks = (ANativeActivityCallbacks *) malloc(sizeof(ANativeActivityCallbacks));
    ret->env = &jni;
    ret->vm = &jvm;
    ret->clazz = (jclass) 0x42424242;
    ret->internalDataPath = DATA_PATH"assets/";
    ret->externalDataPath = DATA_PATH"assets/";
    ret->sdkVersion = 14;
    // Create a fake android_app instance
    android_app * app = (android_app *) malloc(sizeof(android_app));
    app->activity = (ANativeActivity *) ret;
    app->config = AConfiguration_new();
    app->savedState = nullptr;
    app->savedStateSize = 0;
    app->userData = nullptr;
    app->onAppCmd = nullptr;
    app->onInputEvent = nullptr;

    ret->instance = app;

    return ret;
}

void ANativeActivity_setWindowFlags(ANativeActivity* activity, uint32_t addFlags, uint32_t removeFlags) {
    // see Android's window.h for flags reference.
    // they are pretty much useless for us because we are always fullscreen, focusable, etc.
}

void ANativeActivity_finish(ANativeActivity* activity) {
    free(activity);
}
