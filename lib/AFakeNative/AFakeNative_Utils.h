#ifndef AFAKENATIVE_UTILS_H
#define AFAKENATIVE_UTILS_H

#include <cstdint>

/* Used to retry syscalls that can return EINTR. */
#define TEMP_FAILURE_RETRY(exp) ({         \
    __typeof__(exp) _rc;                   \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })

#define DEBUG_EPOLL 1
#define DEBUG_PIPEFD 1
#define DEBUG_CALLBACKS 1
#define DEBUG_POLL_AND_WAKE 1

uint64_t AFN_timeMillis();

void LOG_ALWAYS_FATAL_IF(bool cond, const char * fmt, ...);
void LOG_ALWAYS_FATAL(const char * fmt, ...);
void ALOGE(const char * fmt, ...);
void ALOGW(const char * fmt, ...);
void ALOGD(const char * fmt, ...);

#endif // AFAKENATIVE_UTILS_H
