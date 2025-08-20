#ifndef PROF_H
#define PROF_H

#include <stdint.h>
#include <time.h>
#include <psp2/kernel/threadmgr.h>   /* sceKernelGetThreadId */
#include <vitasdk.h>

/* tune these for your title */
#define MAX_SAMPLES       128
#define MAX_STACK_DEPTH    32
#define MAX_PROF_THREADS    16   /* raise if you have many worker threads */

/* ─────────────────────────────────────────────────────────────── */
typedef struct Sample {
    const char  *name;
    clock_t      start;
    uint64_t     totalTime;
    int          callCount;
    int          parentIndex;
} Sample;

typedef struct ProfilerState {
    SceUID  threadId;                    /*0 → slot unused              */
    int     stackTop;
    int     sampleCount;
    int     stack[MAX_STACK_DEPTH];
    Sample  samples[MAX_SAMPLES];
} ProfilerState;

/* one static pool shared by all threads (zero-filled by BSS) */
extern ProfilerState g_profPool[MAX_PROF_THREADS];

/* public API */
void Profiler_BeginSample(const char *name);
void Profiler_EndSample(void);

void Profiler_PrintAll(void);   /* dump every thread’s data */
void Profiler_ResetAll(void);   /* clear counters           */

#define PROF_ATTACH(FN, MANGLED)                                            \
    do {                                                                    \
        uintptr_t _a = (uintptr_t)so_symbol(&so_mod, MANGLED);              \
        if (_a)                                                             \
            FN##_hk = hook_addr(_a, (uintptr_t)&FN##_wrap);                 \
        else                                                               \
            log_error(#FN " not found\n");                                  \
    } while (0)


/* ────────────────────────────────────────────────────────────── */
/*  1. Hooks for functions that return **void**                   */
#define PROF_HOOK_VOID(FN, MANGLED, PARAMS, ...)                          \
    static so_hook FN##_hk;                                               \
                                                                          \
    static void FN##_wrap PARAMS                                          \
    {                                                                     \
        SO_CONTINUE(void *, FN##_hk, ##__VA_ARGS__);                      \
    }                                                                         \
    // static void __attribute__((constructor(101))) FN##_install(void)       \
    // {                                                                      \
    //     uintptr_t addr = (uintptr_t)so_symbol(&so_mod, MANGLED);           \
    //     if (addr)                                                          \
    //         FN##_hk = hook_addr(addr, (uintptr_t)&FN##_wrap);              \
    //     else                                                               \
    //         log_error(#FN " not found\n");                                 \
    // }
/* ────────────────────────────────────────────────────────────── */
/*  2. Hooks for functions that return **something**              */
#define PROF_HOOK_RET(RET, FN, MANGLED, PARAMS, ...)                       \
    static so_hook FN##_hk;                                                \
                                                                           \
    static RET FN##_wrap PARAMS                                            \
    {                                                                      \
        Profiler_BeginSample(#FN);                                         \
        RET _r = SO_CONTINUE(RET, FN##_hk, ##__VA_ARGS__);                 \
        Profiler_EndSample();                                              \
        return _r;                                                         \
    }                                                                      \
    static void __attribute__((constructor(101))) FN##_install(void)       \
    {                                                                      \
        uintptr_t a = (uintptr_t)so_symbol(&so_mod, MANGLED);              \
        if (a)                                                             \
            FN##_hk = hook_addr(a, (uintptr_t)&FN##_wrap);                 \
        else                                                               \
            log_error(#FN " not found\n");                                 \
    }


#endif