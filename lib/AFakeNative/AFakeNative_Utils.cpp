#include "AFakeNative_Utils.h"

#include <sys/time.h>
#include <psp2/kernel/clib.h>

__attribute__((__no_instrument_function__, __no_profile_instrument_function__))
uint64_t AFN_timeMillis() {
    struct timeval te{};
    gettimeofday(&te, nullptr);
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    return milliseconds;
}

void LOG_ALWAYS_FATAL_IF(bool cond, const char * fmt, ...) {
    if (cond) {
        static char text[2048];
        static char fmt_2[2048];
        sceClibSnprintf(fmt_2, 2047, "FATAL: %s\n", fmt);

        va_list list;
        va_start(list, fmt);
        sceClibVsnprintf(text, 2048, fmt_2, list);
        va_end(list);

        sceClibPrintf(text);
        sceClibAbort();
    }
}

void LOG_ALWAYS_FATAL(const char * fmt, ...) {
    static char text[2048];
    static char fmt_2[2048];
    sceClibSnprintf(fmt_2, 2047, "FATAL: %s\n", fmt);

    va_list list;
    va_start(list, fmt);
    sceClibVsnprintf(text, 2048, fmt_2, list);
    va_end(list);

    sceClibPrintf(text);
    sceClibAbort();
}

void ALOGE(const char * fmt, ...) {
    static char text[2048];
    static char fmt_2[2048];
    sceClibSnprintf(fmt_2, 2047, "ALOGE: %s\n", fmt);

    va_list list;
    va_start(list, fmt);
    sceClibVsnprintf(text, 2048, fmt_2, list);
    va_end(list);

    sceClibPrintf(text);
}

void ALOGW(const char * fmt, ...) {
    static char text[2048];
    static char fmt_2[2048];
    sceClibSnprintf(fmt_2, 2047, "ALOGW: %s\n", fmt);

    va_list list;
    va_start(list, fmt);
    sceClibVsnprintf(text, 2048, fmt_2, list);
    va_end(list);

    sceClibPrintf(text);
}

void ALOGD(const char * fmt, ...) {
    static char text[2048];
    static char fmt_2[2048];
    sceClibSnprintf(fmt_2, 2047, "ALOGD: %s\n", fmt);

    va_list list;
    va_start(list, fmt);
    sceClibVsnprintf(text, 2048, fmt_2, list);
    va_end(list);

    sceClibPrintf(text);
}