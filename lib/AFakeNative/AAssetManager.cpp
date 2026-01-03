#include "AAssetManager.h"
#include "AFakeNative_Utils.h"

#include <pthread.h>
#include <malloc.h>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <libc_bridge/libc_bridge.h>
#include <string>
//#include "reimpl/io.h"

extern "C" ssize_t read_delegate(int fd, void *buf, size_t count);
extern "C" off_t lseek_delegate(int fd, off_t offset, int whence);
extern "C" int open_soloader(char *_fname, int flags, ...);
extern "C" int close_soloader(int fd);

typedef struct assetManager {
    int dummy = 0; // TODO: mb we will need to store something here in future
    pthread_mutex_t mLock;
} assetManager;

typedef struct aAsset {
    char * filename;
    int f;
} asset;

static AAssetManager * g_AAssetManager = nullptr;

AAssetManager * AAssetManager_create() {
    return nullptr;
    if (g_AAssetManager) return g_AAssetManager;

    assetManager am;

    pthread_mutex_init(&am.mLock, nullptr);

    g_AAssetManager = (AAssetManager *) malloc(sizeof(assetManager));
    memcpy(g_AAssetManager, &am, sizeof(assetManager));

    return g_AAssetManager;
}

static std::string stripXmfExt(const char* filename)
{
    std::string name(filename);
    size_t dot = name.rfind('.');
    if (dot != std::string::npos) {
        // Case-insensitive compare against ".xmf"
        if (strcasecmp(name.c_str() + dot, ".xmf") == 0) {
            name.erase(dot);
        }
    }
    return name;
}

AAsset* AAssetManager_open(AAssetManager* mgr, const char* filename, int mode) {
    //ALOGE("AAssetManager_open called with filename: %s, mode: %d", filename, mode);
    // return nullptr;
    // // if filename starts with /res, return nullptr
    // if (strncmp(filename, "/res", 4) == 0) {
    //    // ALOGD("[AAssetManager] AAssetManager_open(%p, %s, %i): returning nullptr", mgr, filename, mode);
    //     return nullptr;
    // }
    // return nullptr;


   // std::string path = stripXmfExt(filename);
   std::string path = std::string(filename);

    //ALOGE("2 AAssetManager_open called with filename: %s, mode: %d\n", path.c_str(), mode);



    //ALOGD("[AAssetManager] AAssetManager_open(%p, %s, %i)", mgr, path.c_str(), mode);
    // // if filename ends with .xmf, return nullptr
    // if (strstr(path.c_str(), ".xmf") != nullptr) {
    //     ALOGD("Ignoring xmf file");
    //      return nullptr;
    // }

    // remove .xmf extension if it exists from the filename by getting rid of the last 4 characters
    // char filename_no_ext[256];
    // strncpy(filename_no_ext, filename, strlen(filename) - 4);
    // filename_no_ext[strlen(filename) - 4] = '\0';
    
    //print return address
    //ALOGD("AAssetManager_open called from %p", __builtin_return_address(0));
    std::string realp = std::string(DATA_PATH) + std::string("assets/") + path;

    

    //ALOGD("[AAssetManager] AAssetManager_open real path: %s", realp.c_str());

    auto * a = (aAsset *) malloc(sizeof(aAsset));
    memset(a, 0, sizeof(aAsset));  // Zero-initialize
   // a->fd = -1;  // Initialize to invalid fd
    a->filename = (char *) malloc(realp.length() + 1);
    strcpy(a->filename, realp.c_str());

    a->f = open_soloader(a->filename, O_RDONLY);
    //ALOGD("[AAssetManager] AAssetManager_open: fd=0x%X", a->f);
    if (!a->f || a->f == -1) {
        free(a->filename);
        free(a);
        a = nullptr;
    }

    // log return value
    //ALOGD("[AAssetManager] AAssetManager_open returns %p", a);

    //ALOGD("[AAssetManager] AAssetManager_open(%p, %s, %i): %p", mgr, realp.c_str(), mode, a);
    return (AAsset *) a;
}

int AAsset_openFileDescriptor(AAsset* asset, off_t* outStart, off_t* outLength) {
    //ALOGD("AAsset_openFileDescriptor(%p)", asset);
    if (!asset) {
        return -1;
    }
    auto * a = (aAsset *) asset;

    *outStart = 0;

    off_t current_pos = lseek_delegate(a->f, 0, SEEK_CUR);
    off_t file_size = lseek_delegate(a->f, 0, SEEK_END);
    lseek_delegate(a->f, current_pos, SEEK_SET);  // Restore position

    *outLength = file_size;
    //ALOGD("AAsset_openFileDescriptor(start=%d, len=%d) a->f=0x%x", *outStart, *outLength, a->f);

    return open_soloader(a->filename, O_RDONLY);
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
    //return;
    
    if (asset) {
        auto * a = (aAsset *) asset;
        free(a->filename);

        // Close FILE* (which closes its underlying dup'd fd)
        if (a->f) {
            int r = close_soloader(a->f);
           // ALOGD("AAsset_close(%p)=%d", asset, r);
        }

        free(a);
    }

   // ALOGD("DONE AAsset_close(%p)", asset);
}

int AAsset_read(AAsset* asset, void* buf, size_t count) {
   // ALOGD("AAsset_read(%p, %p, %i)", asset, buf, count);
   // return -1;

    if (!asset) {
        return -1;
    }

    auto * a = (aAsset *) asset;

    ssize_t ret = read_delegate(a->f, buf, count);
// #ifdef USE_SCELIBC_IO
//     size_t ret = sceLibcBridge_fread(buf, 1, count, a->f);
// #else
//     size_t ret = fread(buf, 1, count, a->f);
// #endif

    if (ret > 0) {
        return (int) ret;
    } else {
// #ifdef USE_SCELIBC_IO
//         if (ret == 0 || sceLibcBridge_feof(a->f)) {
// #else
        //if (ret == 0 || feof(a->f)) {
//#endif
        if (ret == 0) {
            return 0;
        } else {
            return -1;
        }
    }
}

off_t AAsset_seek(AAsset* asset, off_t offset, int whence) {
    //ALOGD("AAsset_seek(%p, %d, %i)", asset, offset, whence);
    return -1;
    if (!asset) {
        return (off_t) -1;
    }

    auto * a = (aAsset *) asset;

    auto ret = (off_t)lseek_delegate(a->f, offset, whence);
// #ifdef USE_SCELIBC_IO
//     auto ret = (off_t) sceLibcBridge_fseek(a->f, offset, whence);
// #else
//     auto ret = (off_t) fseek(a->f, offset, whence);
// #endif

    return ret;
}
