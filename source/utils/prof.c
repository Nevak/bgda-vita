#include "prof.h"
#include <string.h>
#include <psp2/kernel/clib.h>

/* GLOBAL POOL (uninitialised BSS) */
ProfilerState g_profPool[MAX_PROF_THREADS];
static void print_branch(ProfilerState *ps, int parent, int indent);


static ProfilerState *cur(void)
{
    const SceUID tid = sceKernelGetThreadId();
    //sceClibPrintf("Profiler: cur() called by thread 0x%08X\n", tid);
    /* 1. fast path – already registered */
    
    for (int i = 0; i < MAX_PROF_THREADS; ++i)
        if (g_profPool[i].threadId == tid)
            return &g_profPool[i];

    /* 2. slow path – claim a free slot */
    for (int i = 0; i < MAX_PROF_THREADS; ++i) {
        if (g_profPool[i].threadId == 0) {
            ProfilerState *ps = &g_profPool[i];
            ps->threadId    = tid;
            ps->stackTop    = -1;
            ps->sampleCount = 0;
            return ps;
        }
    }

    /* 3. out of slots – fallback to slot 0 (better than crashing) */
    return &g_profPool[0];
}



void Profiler_BeginSample(const char* name) {
    ProfilerState *ps = cur();

    /* parent is −1 when stack empty */
    int parent = (ps->stackTop >= 0) ? ps->stack[ps->stackTop] : -1;

    /* find existing sample in the same branch */
    int idx = -1;
    int sampleCount = ps->sampleCount;
    Sample* samples = ps->samples;
    for (int i = 0; i < sampleCount; ++i) {
        Sample *s = &samples[i];
        if (s->parentIndex == parent && strcmp(s->name, name) == 0) {
            idx = i;
            break;
        }
    }

    /* create new entry if necessary */
    if (idx == -1 && ps->sampleCount < MAX_SAMPLES) {
        idx = ps->sampleCount++;
        Sample *s = &ps->samples[idx];
        s->name       = name;
        s->totalTime  = 0;
        s->callCount  = 0;
        s->parentIndex  = parent;
    }

    Sample *s = &ps->samples[idx];
    s->start = clock();
    s->callCount++;

    /* push onto call-stack */
    if (++ps->stackTop < MAX_STACK_DEPTH)
        ps->stack[ps->stackTop] = idx;
    else
        --ps->stackTop;                /* ignore over-depth calls */
}

void Profiler_EndSample()
{
    ProfilerState *ps = cur();

    if (ps->stackTop < 0) return;      /* underflow safeguard */

    int idx = ps->stack[ps->stackTop--];
    clock_t end = clock();
    ps->samples[idx].totalTime += (uint64_t)(end - ps->samples[idx].start);
}

/* ─────────────────────────────── */
/* utilities: print and reset      */
static void print_branch(ProfilerState *ps, int parent, int indent)
{
    for (int i = 0; i < ps->sampleCount; ++i) {
        Sample *s = &ps->samples[i];
        if (s->parentIndex != parent) continue;

        for (int j = 0; j < indent; ++j) sceClibPrintf("  ");
        sceClibPrintf("%s: took %.2f ms in %d calls\n",
                      s->name,
                      (double)s->totalTime * 1000.0 / CLOCKS_PER_SEC,
                      s->callCount);

        print_branch(ps, i, indent + 1);
    }
}

int frame_count = 0;
void Profiler_PrintAll(void)
{
    sceClibPrintf("========== [%d] Profiler Report ==========\n", frame_count++);
    for (int i = 0; i < MAX_PROF_THREADS; ++i) {
        ProfilerState *ps = &g_profPool[i];
        if (ps->threadId == 0) continue;   /* unused slot */

        sceClibPrintf("\nThread 0x%08X\n", ps->threadId);
        print_branch(ps, -1, 0);
    }
    sceClibPrintf("==============================\n");
}

void Profiler_ResetAll(void)
{
    for (int i = 0; i < MAX_PROF_THREADS; ++i) {
        ProfilerState *ps = &g_profPool[i];
        ps->sampleCount = 0;
        ps->stackTop    = -1;
    }
}
