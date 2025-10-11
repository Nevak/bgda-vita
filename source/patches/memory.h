#ifndef MEMORY_H
#define MEMORY_H

#include <so_util/so_util.h>

so_hook memInit_hook;
// This is to reduce the internal memory pool that is allocated by the game
// It was hardcoded to 0x4000000 (64MB) but it seems to work fine with less (32MB). Found by trial and error.
// Maybe it can be reduced even more.
void memInit(void *param_1, int param_2) {
	logv_error("memInit(%p, %d)\n", param_1, param_2);
	// free the original memory
	free(param_1);

	// allocate new memory. original is 0x4000000 which is like 64MB
	int newSize = 0x2000000;
	void *newMem = malloc(newSize);

	SO_CONTINUE(void *, memInit_hook, newMem, newSize);
	//log_error("memInit finished\n");
}

so_hook MEMAllocFromExpHeapEx_hook;
void *MEMAllocFromExpHeapEx(void *heap, int size, int flags) {
	void *res = SO_CONTINUE(void *, MEMAllocFromExpHeapEx_hook, heap, size, flags);
	if (res == NULL) {
		logv_error("MEMAllocFromExpHeapEx returned NULL for heap %p, size %d, flags %d\n", heap, size, flags);
	}
	return res;
}

// so_hook memAlloc_hook;
// void *memAlloc(int size, const char *name) {
// 	//logv_error("memAlloc(size: %d, name: %s)\n", size, name);
// 	void *res = SO_CONTINUE(void *, memAlloc_hook, size, name);
// 	if (res == NULL) {
// 		logv_error("memAlloc returned NULL for size %d, name %s\n", size, name);
// 	}
// 	//logv_error("memAlloc returned: %p\n", res);
// 	return res;
// }

#endif