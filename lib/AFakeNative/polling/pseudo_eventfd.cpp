#include <cstdint>
#include <psp2/kernel/threadmgr.h>
#include <malloc.h>
#include <cerrno>
#include <cstdio>
#include <sys/unistd.h>
#include <atomic>
#include "AFakeNative/AFakeNative_Utils.h"
#include "AFakeNative/PseudoEpoll.h"

#include "pseudo_eventfd.h"

#define EVENTFD_MARGIN 256
#define EVENTFD_MAX 64

typedef struct eventfd_internal {
    int fd = -1; // >=0 indicates that it's in use
    uint64_t value{};
    int flags{};
    SceKernelLwMutexWork * mutex{};
} eventfd_internal;

static eventfd_internal eventfd_pool[EVENTFD_MAX];
static SceKernelLwMutexWork eventfd_pool_mutex;
static std::atomic<int> eventfd_pool_mutex_inited(0);

// Thread-safe lazy initialization
static inline void eventfd_mutex_init_once(void) {
    int expected = 0;
    if (eventfd_pool_mutex_inited.compare_exchange_strong(expected, 1)) {
        // We won the race, initialize the mutex
        int ret = sceKernelCreateLwMutex(&eventfd_pool_mutex, "eventfd_pool_mutex", 0, 0, NULL);
        if (ret < 0) {
            printf("Error: failed to create eventfd_pool mutex: 0x%x\n", ret);
            eventfd_pool_mutex_inited.store(0);
            return;
        }

        // Initialize the pool
        for (int i = 0; i < EVENTFD_MAX; ++i) {
            eventfd_pool[i].fd = -1;
            eventfd_pool[i].value = 0;
            eventfd_pool[i].flags = 0;
            eventfd_pool[i].mutex = (SceKernelLwMutexWork *) malloc(sizeof(SceKernelLwMutexWork));
        }

        eventfd_pool_mutex_inited.store(2); // Mark as fully initialized
    } else {
        // Another thread is initializing, wait for it to complete
        while (eventfd_pool_mutex_inited.load() == 1) {
            sceKernelDelayThread(100); // Wait 0.1ms
        }
    }
}

int pseudo_eventfd(unsigned int initval, int flags) {
    eventfd_mutex_init_once();

    if (eventfd_pool_mutex_inited.load() != 2) {
        return -1; // Initialization failed
    }

    sceKernelLockLwMutex(&eventfd_pool_mutex, 1, NULL);

    eventfd_internal * fd = nullptr;
    for (int i = 0; i < EVENTFD_MAX; ++i) {
        if (eventfd_pool[i].fd == -1) {
            eventfd_pool[i].fd = i + EVENTFD_MARGIN;
            fd = &eventfd_pool[i];
            sceKernelCreateLwMutex(eventfd_pool[i].mutex, "eventfd_mutex", 0, 0, NULL);
            break;
        }
    }

    if (!fd) {
        sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);
        errno = EMFILE;
        return -1;
    }

    fd->value = initval;
    fd->flags = flags;

    sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);
#ifdef DEBUG_POLL_AND_WAKE
    ALOGD("Created eventfd #%i from addr %p", fd->fd, __builtin_return_address(0));
#endif
    return fd->fd;
}

bool is_eventfd(int fd) {
    eventfd_mutex_init_once();

    if (eventfd_pool_mutex_inited.load() != 2) {
        return false; // Can't determine if not initialized
    }

    eventfd_internal * p = nullptr;

    sceKernelLockLwMutex(&eventfd_pool_mutex, 1, NULL);

    for (int i = 0; i < EVENTFD_MAX; ++i) {
        if (eventfd_pool[i].fd == fd) {
            p = &eventfd_pool[i];
            break;
        }
    }

    sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);

    return p != nullptr;
}

ssize_t pseudo_eventfd_read(int fd, void *buf, size_t count) {
    if (eventfd_pool_mutex_inited.load() != 2) {
        return -1;
    }
    sceKernelLockLwMutex(&eventfd_pool_mutex, 1, NULL);

    eventfd_internal * efd = nullptr;

    for (int i = 0; i < EVENTFD_MAX; ++i) {
        if (eventfd_pool[i].fd == fd) {
            efd = &eventfd_pool[i];
            break;
        }
    }

    if (!efd) {
        sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);
        errno = EINVAL;
        return -1;
    }

    if (count < 8 || !buf) {
        sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);
        errno = EINVAL;
        return -1;
    }

    sceKernelLockLwMutex(efd->mutex, 1, NULL);

    if (efd->value == 0) {
        if (efd->flags & PSEUDO_EFD_NONBLOCK) {
            sceKernelUnlockLwMutex(efd->mutex, 1);
            sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);
            errno = EAGAIN;
            return -1;
        } else {
            for (;;) {
                sceKernelUnlockLwMutex(efd->mutex, 1);
                sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);
                usleep(10000);
                sceKernelLockLwMutex(&eventfd_pool_mutex, 1, NULL);
                sceKernelLockLwMutex(efd->mutex, 1, NULL);

                if (efd->value != 0) {
                    break;
                }
            }
        }
    }

    if (efd->flags & PSEUDO_EFD_SEMAPHORE && efd->value != 0) {
        *(uint64_t *)buf = (uint64_t) 1;
        efd->value--;
        sceKernelUnlockLwMutex(efd->mutex, 1);
        sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);
        return 8;
    }

    // Non-semaphore, non-zero value
    *(uint64_t *)buf = efd->value;
    efd->value = 0;
    sceKernelUnlockLwMutex(efd->mutex, 1);
    sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);
    return 8;
}

ssize_t pseudo_eventfd_write(int fd, const void *buf, size_t count) {
    if (eventfd_pool_mutex_inited.load() != 2) {
        return -1;
    }
    sceKernelLockLwMutex(&eventfd_pool_mutex, 1, NULL);

    uint64_t val;
    eventfd_internal * efd = nullptr;

    for (int i = 0; i < EVENTFD_MAX; ++i) {
        if (eventfd_pool[i].fd == fd) {
            efd = &eventfd_pool[i];
            break;
        }
    }

    if (!efd) {
        sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);
        errno = EINVAL;
        return -1;
    }

    if (count < 8 || !buf) {
        sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);
        errno = EINVAL;
        return -1;
    }

    sceKernelLockLwMutex(efd->mutex, 1, NULL);

    val = *(uint64_t *) buf;
    if (0xfffffffffffffffe - efd->value < val) {
        if (efd->flags & PSEUDO_EFD_NONBLOCK) {
            sceKernelUnlockLwMutex(efd->mutex, 1);
            sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);
            errno = EAGAIN;
            return -1;
        } else {
            for (;;) {
                sceKernelUnlockLwMutex(efd->mutex, 1);
                sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);
                usleep(10000);
                sceKernelLockLwMutex(&eventfd_pool_mutex, 1, NULL);
                sceKernelLockLwMutex(efd->mutex, 1, NULL);

                if (0xfffffffffffffffe - efd->value >= val) {
                    break;
                }
            }
        }
    }

    efd->value += val;
    sceKernelUnlockLwMutex(efd->mutex, 1);
    sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);
    return 8;
}

void pseudo_eventfd_status(int fd, bool * is_readable, bool * is_writeable) {
    if (eventfd_pool_mutex_inited.load() != 2) {
        return;
    }

    sceKernelLockLwMutex(&eventfd_pool_mutex, 1, nullptr);

    for (int u = 0; u < EVENTFD_MAX; ++u) {
        if (eventfd_pool[u].fd == fd) {
            sceKernelLockLwMutex(eventfd_pool[u].mutex, 1, nullptr);
            *is_readable = eventfd_pool[u].value > 0;
            *is_writeable = eventfd_pool[u].value < 0xfffffffffffffffe;
            sceKernelUnlockLwMutex(eventfd_pool[u].mutex, 1);
            break;
        }
    }

    sceKernelUnlockLwMutex(&eventfd_pool_mutex, 1);
}
