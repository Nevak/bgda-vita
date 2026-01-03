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

	logv_error("MEMAllocFromExpHeapEx returned: %p  heap %p, size %d, flags %d\n", res, heap, size, flags);
	return res;
}

so_hook memAlloc_hook;
void *memAlloc(int size, const char *name) {
	// get caller address
	uintptr_t caller = (uintptr_t)__builtin_return_address(0);
	// measure time taken by memAlloc
	uint64_t timeNow = sceKernelGetProcessTimeWide();
	//logv_error("memAlloc(size: %d, name: %s)\n", size, name);
	void *res = SO_CONTINUE(void *, memAlloc_hook, size, name);
	if (res == NULL) {
		logv_error("memAlloc returned NULL for size %d, name %s\n", size, name);
	}

	uint64_t timeEnd = sceKernelGetProcessTimeWide();
	float elapsedMs = (timeEnd - timeNow) / 1000.0f;

	logv_error("[%p] memAlloc returned: %p  name %s, size %d, took %.2f ms\n", (void*)caller, res, name, size, elapsedMs);
	return res;
}

so_hook memFree_hook;
void memFree(void *ptr) {
	logv_error("memFree(%p)\n", ptr);
	SO_CONTINUE(void *, memFree_hook, ptr);
}

so_hook jbe_android_main_sub_hook;
void jbe_android_main_sub(void * app) {
	logv_error("jbe_android_main_sub called from %p", __builtin_return_address(0));
	SO_CONTINUE(void *, jbe_android_main_sub_hook, app);
}

// _ZN3JBE3Mem4Heap5AllocEjNS0_8LocationEiPKcz
// This is a variadic C++ member function
so_hook JBE_Mem_Heap_Alloc_hook;

// Logging helper called from assembly
__attribute__((used))
static void JBE_Mem_Heap_Alloc_logentry(void* thisptr, uint32_t size, uint32_t location,
                                         int param_2, void* caller) {
	logv_error("JBE::Mem::Heap::Alloc(this=%p, size=%u, location=%u, param_2=%d) called from %p",
		thisptr, size, location, param_2, caller);

	// Check if the value of thisptr + 0x10 is NULL
	void** vtable_ptr = (void**)((uintptr_t)thisptr + 0x10);
	if (vtable_ptr != NULL) {
		void* vtable = *vtable_ptr;
		if (vtable == NULL) {
			log_error("  Warning: vtable pointer at thisptr+0x10 is NULL!");
		}
	} else {
		log_error("  Warning: thisptr+0x10 is NULL!");
	}

	logv_debug("  Heap vtable pointer: %p", vtable_ptr ? *vtable_ptr : NULL);
}

// Trampoline function with inline assembly
// This preserves all arguments and stack state while calling our log function
__attribute__((naked)) __attribute__((noinline))
void *JBE_Mem_Heap_Alloc(void* thisptr, uint32_t size, uint32_t location, int param_2, const char* name, ...) {
	__asm__ (
		// Save all registers we'll use
		"push {r0-r3, r4, lr}\n"

		// Call log function: JBE_Mem_Heap_Alloc_logentry(r0=thisptr, r1=size, r2=location, r3=param_2, stack=lr)
		// r0-r3 are already set with first 4 args
		"mov r4, lr\n"                    // Save LR (caller) to r4
		"push {r4}\n"                     // Push as 5th argument
		"bl JBE_Mem_Heap_Alloc_logentry\n"
		"add sp, sp, #4\n"                // Clean up stack arg

		// Restore original arguments
		"pop {r0-r3, r4, lr}\n"

		// Get address of original function from hook struct
		// We need to use PC-relative addressing to avoid literal pool issues
		"push {r0-r3}\n"                  // Save args again
		"adr r0, 1f\n"                    // Get address of pointer below
		"ldr r0, [r0]\n"                  // Load &JBE_Mem_Heap_Alloc_hook
		"ldr r0, [r0, #12]\n"             // Load hook.orig (trampoline at offset 12)
		"mov r4, r0\n"                    // Save to r4
		"pop {r0-r3}\n"                   // Restore args

		// Tail call to original function (preserves all stack args)
		"bx r4\n"

		// Data section with pointer (PC-relative accessible)
		".align 2\n"
		"1: .word JBE_Mem_Heap_Alloc_hook\n"
	);
}
void patch_memory() {
	//memInit_hook = hook_addr(LOC(0x0013d578), (uintptr_t)&memInit);
	uintptr_t memInit_addr = (uintptr_t)so_symbol(&so_mod, "_Z7memInitPvi");
	if (memInit_addr == 0) {
		log_error("memInit not found\n");
	} else {
		logv_debug("memInit found at %p\n", memInit_addr);
		memInit_hook = hook_addr(memInit_addr, (uintptr_t)&memInit);
	}

	//memAlloc_hook = hook_addr(LOC(0x000f9b0c), (uintptr_t)&memAlloc);
	//MEMAllocFromExpHeapEx_hook = hook_addr(LOC(0x0023ea88), (uintptr_t)&MEMAllocFromExpHeapEx);
	//memFree_hook = hook_addr(LOC(0x000f99e8), (uintptr_t)&memFree);

	// uintptr_t jbe_andoid_main_addr = (uintptr_t) so_symbol(&so_mod, "JBE_android_main_sub");
	// if (jbe_andoid_main_addr == 0) {
	// 	log_error("JBE_android_main_sub not found\n");
	// } else {
	// 	logv_debug("JBE_android_main_sub found at %p\n", jbe_andoid_main_addr);
	// 	jbe_android_main_sub_hook = hook_addr(jbe_andoid_main_addr, (uintptr_t)&jbe_android_main_sub);
	// }

	// Hook JBE::Mem::Heap::Alloc - uses assembly trampoline for variadic function
	// _ZN3JBE3Mem4Heap5AllocEjNS0_8LocationEiPKcz
	// uintptr_t JBE_Mem_Heap_Alloc_addr = (uintptr_t) so_symbol(&so_mod, "_ZN3JBE3Mem4Heap5AllocEjNS0_8LocationEiPKcz");
	// if (JBE_Mem_Heap_Alloc_addr == 0) {
	// 	log_error("JBE::Mem::Heap::Alloc not found\n");
	// } else {
	// 	logv_debug("JBE::Mem::Heap::Alloc found at %p\n", JBE_Mem_Heap_Alloc_addr);
	// 	JBE_Mem_Heap_Alloc_hook = hook_addr(JBE_Mem_Heap_Alloc_addr, (uintptr_t)&JBE_Mem_Heap_Alloc);
	// }
}

#endif