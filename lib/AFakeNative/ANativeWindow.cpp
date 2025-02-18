#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "ANativeWindow.h"
#include "utils/logger.h"

typedef struct nativeWindow {
    int dummy;
} nativeWindow;

ANativeWindow * ANativeWindow_create() {
    nativeWindow win;

    auto ret = (ANativeWindow *) malloc(sizeof(nativeWindow));
    memcpy(ret, &win, sizeof(nativeWindow));

    return ret;
}

int32_t ANativeWindow_getWidth(ANativeWindow* window) {
    return 960;
}

int32_t ANativeWindow_getHeight(ANativeWindow* window) {
    return 544;
}

int32_t ANativeWindow_getFormat(ANativeWindow* window) {
    return 1; // AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM
}

int32_t ANativeWindow_setBuffersGeometry(ANativeWindow* window,
                                         int32_t width, int32_t height, int32_t format) {
    logv_error("unimpl: ANativeWindow_setBuffersGeometry %d %d %d", width, height, format);
    return 0;
}
