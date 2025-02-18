#include "AStorageManager.h"
#include <stdlib.h>

struct AStorageManager {
    int dummy;
};

AStorageManager* AStorageManager_new() {
        AStorageManager* mgr = (AStorageManager*)malloc(sizeof(AStorageManager));
        return mgr;
}

void AStorageManager_delete(AStorageManager* mgr) {
        free(mgr);
}

const char* AStorageManager_getMountedObbPath(AStorageManager* mgr, const char* filename) {
        return "mnt/sdcard/fakefakefake";
}
