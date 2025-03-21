#include "AAssetManager.h"
#include "AFakeNative_Utils.h"

#include <pthread.h>
#include <malloc.h>
#include <cstring>
#include <cstdio>
#include <libc_bridge/libc_bridge.h>
#include <string>

typedef struct assetManager {
    int dummy = 0; // TODO: mb we will need to store something here in future
    pthread_mutex_t mLock;
} assetManager;

typedef struct aAsset {
    char * filename;
    FILE* f;
} asset;

static AAssetManager * g_AAssetManager = nullptr;

AAssetManager * AAssetManager_create() {
    if (g_AAssetManager) return g_AAssetManager;

    assetManager am;

    pthread_mutex_init(&am.mLock, nullptr);

    g_AAssetManager = (AAssetManager *) malloc(sizeof(assetManager));
    memcpy(g_AAssetManager, &am, sizeof(assetManager));

    return g_AAssetManager;
}

AAsset* AAssetManager_open(AAssetManager* mgr, const char* filename, int mode) {


    // if filename ends with .xmf, return nullptr
    if (strstr(filename, ".xmf") != nullptr) {
        return nullptr;
    }

    ALOGD("[AAssetManager] AAssetManager_open(%p, %s, %i)", mgr, filename, mode);
    // remove .xmf extension if it exists from the filename by getting rid of the last 4 characters
    // char filename_no_ext[256];
    // strncpy(filename_no_ext, filename, strlen(filename) - 4);
    // filename_no_ext[strlen(filename) - 4] = '\0';
    
    //print return address
    ALOGD("AAssetManager_open called from %p", __builtin_return_address(0));
    std::string realp = std::string(DATA_PATH) + std::string("assets/") + std::string(filename);

    

    ALOGD("[AAssetManager] AAssetManager_open real path: %s", realp.c_str());

    auto * a = (aAsset *) malloc(sizeof(aAsset));
    a->filename = (char *) malloc(realp.length() + 1);
    strcpy(a->filename, realp.c_str());

#ifdef USE_SCELIBC_IO
    a->f = sceLibcBridge_fopen((const char *)a->filename, "r");
#else
    a->f = fopen((cost char *)a->filename, "r");
#endif

    if (!a->f) {
        free(a->filename);
        free(a);
        a = nullptr;
    }

    // log return value
    ALOGD("[AAssetManager] AAssetManager_open returns %p", a);

    //ALOGD("[AAssetManager] AAssetManager_open(%p, %s, %i): %p", mgr, realp.c_str(), mode, a);
    return (AAsset *) a;
}

int AAsset_openFileDescriptor(AAsset* asset, off_t* outStart, off_t* outLength) {
    if (!asset) {
        return -1;
    }

    auto * a = (aAsset *) asset;

#ifdef USE_SCELIBC_IO
    //log_error("UNIMPLEMENTED!!!");
    auto ret = NULL;
    //auto ret = (off_t) sceLibcBridge_fseek(a->f, offset, whence);
#else
    auto ret = fileno(a->f);
    *outStart = 0;

    fseek(a->f, 0L, SEEK_END);
    *outLength = ftell(a->f);
    fseek(a->f, 0L, SEEK_SET);
#endif

    return ret;
}

// AAssetDir* AAssetManager_openDir() {
//     std::string realp = std::string(DATA_PATH) + std::string("assets/");

//     auto * a = (aAssetDir *) malloc(sizeof(aAssetDir));
//     a->filename = (char *) malloc(realp.length() + 1);
//     strcpy(a->filename, realp.c_str());

// #ifdef USE_SCELIBC_IO
//     a->f = sceLibcBridge_opendir((const char *)a->filename);
// #else
//     a->f = opendir((cost char *)a->filename);
// #endif
    
//         if (!a->f) {
//             free(a->filename);
//             free(a);
//             a = nullptr;
//         }
    
//         ALOGD("[AAssetManager] AAssetManager_openDir(%s): %p", realp.c_str(), a);
//         return (AAssetDir *) a;
// }

void AAsset_close(AAsset* asset) {
    //ALOGD("AAsset_close(%p)", asset);

    if (asset) {
        auto * a = (aAsset *) asset;
        free(a->filename);
        fclose(a->f);
        free(a);
    }
}

int AAsset_read(AAsset* asset, void* buf, size_t count) {
    //ALOGD("AAsset_read(%p, %p, %i)", asset, buf, count);

    if (!asset) {
        return -1;
    }

    auto * a = (aAsset *) asset;

#ifdef USE_SCELIBC_IO
    size_t ret = sceLibcBridge_fread(buf, 1, count, a->f);
#else
    size_t ret = fread(buf, 1, count, a->f);
#endif

    if (ret > 0) {
        return (int) ret;
    } else {
#ifdef USE_SCELIBC_IO
        if (ret == 0 || sceLibcBridge_feof(a->f)) {
#else
        if (ret == 0 || feof(a->f)) {
#endif
            return 0;
        } else {
            return -1;
        }
    }
}

off_t AAsset_seek(AAsset* asset, off_t offset, int whence) {
    //ALOGD("AAsset_seek(%p, %d, %i)", asset, offset, whence);

    if (!asset) {
        return (off_t) -1;
    }

    auto * a = (aAsset *) asset;

#ifdef USE_SCELIBC_IO
    auto ret = (off_t) sceLibcBridge_fseek(a->f, offset, whence);
#else
    auto ret = (off_t) fseek(a->f, offset, whence);
#endif

    return ret;
}
