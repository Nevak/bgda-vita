#include "PseudoEpoll.h"
#include "AFakeNative_Utils.h"
#include <pthread.h>
#include <cstdlib>
#include <cstring>
#include <map>
#include <sys/unistd.h>
#include <cstdio>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/clib.h>

#include "polling/pseudo_eventfd.h"
#include "polling/pseudo_pipe.h"

#define EPOLL_FD_MARGIN 128
#define EPOLL_FD_MAX 64

typedef struct epollElement {
    int fd;
    pseudo_epoll_event e;
} epollElement;

typedef struct _epoll_fd_internal {
    int fd = -1; // >=0 indicates that it's in use
    std::map<int, epollElement> * interest = nullptr;
} _epoll_fd_internal;

static _epoll_fd_internal epoll_fd_pool[EPOLL_FD_MAX];
static SceKernelLwMutexWork * _epoll_lock = nullptr;


void _check_init_lock() {
    if (_epoll_lock == nullptr) {
        _epoll_lock = (SceKernelLwMutexWork *) malloc(sizeof(SceKernelLwMutexWork));
        sceKernelCreateLwMutex(_epoll_lock, "epoll_lock", 0, 0, NULL);

        for (int i = 0; i < EPOLL_FD_MAX; ++i) {
            epoll_fd_pool[i].fd = -1;
        }
    }
}

void _lock() {
    _check_init_lock();
    sceKernelLockLwMutex(_epoll_lock, 1, NULL);
}

void _unlock() {
    if (_epoll_lock) sceKernelUnlockLwMutex(_epoll_lock, 1);
}

int pseudo_epoll_create(int size) {
    if (size <= 0) {
        errno = EINVAL;
        return -1;
    }

    return pseudo_epoll_create1(0);
}

int pseudo_epoll_create1(int flags) {
    // flags can be ditched since the only flag is O_CLOEXEC and we never exec()?
    if (flags != 0 && flags != PSEUDO_EPOLL_CLOEXEC) {
        errno = EINVAL;
        return -1;
    }

    _lock();

    _epoll_fd_internal * fd = nullptr;
    for (int i = 0; i < EPOLL_FD_MAX; ++i) {
        if (epoll_fd_pool[i].fd == -1) {
            epoll_fd_pool[i].fd = i + EPOLL_FD_MARGIN;
            epoll_fd_pool[i].interest = new std::map<int, epollElement>;
            fd = &epoll_fd_pool[i];
            break;
        }
    }

    if (!fd) {
        _unlock();
        errno = EMFILE;
        return -1;
    }

    _unlock();
    return fd->fd;
}

#ifdef DEBUG_EPOLL
const char * __op_to_str(int op) {
    switch (op) {
        case PSEUDO_EPOLL_CTL_ADD:
            return "PSEUDO_EPOLL_CTL_ADD";
        case PSEUDO_EPOLL_CTL_DEL:
            return "PSEUDO_EPOLL_CTL_DEL";
        case PSEUDO_EPOLL_CTL_MOD:
            return "PSEUDO_EPOLL_CTL_MOD";
    }
    return "PSEUDO_EPOLL_CTL_UNKNOWN";
}
#endif

int pseudo_epoll_ctl(int epfd, int op, int fd, struct pseudo_epoll_event *event) {
    if (epfd < EPOLL_FD_MARGIN || epfd > EPOLL_FD_MARGIN + EPOLL_FD_MAX || fd < 0) {
#ifdef DEBUG_EPOLL
        ALOGD("pseudo_epoll_ctl(epfd:%i, op:%s, fd:%i): EBADF: epfd or fd is not a valid file descriptor.", epfd, __op_to_str(op), fd);
#endif
        errno = EBADF;
        return -1;
    }

    _epoll_fd_internal * epoll = nullptr;
    for (int i = 0; i < EPOLL_FD_MAX; ++i) {
        if (epoll_fd_pool[i].fd == epfd) {
            epoll = &epoll_fd_pool[i];
            break;
        }
    }

    if (!epoll || fd == epfd) {
#ifdef DEBUG_EPOLL
        ALOGD("pseudo_epoll_ctl(epfd:%i, op:%s, fd:%i): EINVAL: epfd is not an epoll file descriptor, or fd is the same as epfd.", epfd, __op_to_str(op), fd);
#endif
        _unlock();
        errno = EINVAL;
        return -1;
    }

    if (op == PSEUDO_EPOLL_CTL_ADD && epoll->interest->contains(fd)) { 
#ifdef DEBUG_EPOLL
        ALOGD("pseudo_epoll_ctl(epfd:%i, op:%s, fd:%i): EEXIST: op was EPOLL_CTL_ADD, and the supplied file descriptor fd is already registered with this epoll instance.", epfd, __op_to_str(op), fd);
#endif
        _unlock();
        errno = EEXIST;
        return -1;
    }

    // EINVAL: An invalid event type was specified along with EPOLLEXCLUSIVE in events.
    // ????

    if (!event && op != PSEUDO_EPOLL_CTL_DEL) {
#ifdef DEBUG_EPOLL
        ALOGD("pseudo_epoll_ctl(epfd:%i, op:%s, fd:%i): EINVAL: [extra]: `event` can not be null if `op` isn't EPOLL_CTL_DEL", epfd, __op_to_str(op), fd);
#endif
        _unlock();
        errno = EINVAL;
        return -1;
    }

    if ((op == PSEUDO_EPOLL_CTL_MOD || op == PSEUDO_EPOLL_CTL_DEL) && !epoll->interest->contains(fd)) {
#ifdef DEBUG_EPOLL
        ALOGD("pseudo_epoll_ctl(epfd:%i, op:%s, fd:%i): ENOENT: op was EPOLL_CTL_MOD or EPOLL_CTL_DEL, and fd is not registered with this epoll instance.", epfd, __op_to_str(op), fd);
#endif

        _unlock();
        errno = ENOENT;
        return -1;
    }

    if (op == PSEUDO_EPOLL_CTL_MOD && epoll->interest->at(epfd).e.events & PSEUDO_EPOLLEXCLUSIVE) {
#ifdef DEBUG_EPOLL
        ALOGD("pseudo_epoll_ctl(epfd:%i, op:%s, fd:%i): EINVAL: op was EPOLL_CTL_MOD and the EPOLLEXCLUSIVE flag has previously been applied to this epfd, fd pair.", epfd, __op_to_str(op), fd);
#endif

        _unlock();
        errno = EINVAL;
        return -1;
    }

    if (op == PSEUDO_EPOLL_CTL_MOD && event->events & PSEUDO_EPOLLEXCLUSIVE) {
#ifdef DEBUG_EPOLL
        ALOGD("pseudo_epoll_ctl(epfd:%i, op:%s, fd:%i): EINVAL: op was EPOLL_CTL_MOD and events included EPOLLEXCLUSIVE.", epfd, __op_to_str(op), fd);
#endif

        _unlock();
        errno = EINVAL;
        return -1;
    }

    if (fd >= EPOLL_FD_MARGIN && fd < EPOLL_FD_MARGIN + EPOLL_FD_MAX) {
#ifdef DEBUG_EPOLL
        ALOGD("pseudo_epoll_ctl(epfd:%i, op:%s, fd:%i): ELOOP: fd refers to an epoll instance and this EPOLL_CTL_ADD operation would result in a circular loop of epoll instances monitoring one another or a nesting depth of epoll instances greater than 5.", epfd, __op_to_str(op), fd);
#endif

        // fd refers to an epoll instance. while not exactly per spec, but let's easen up our life a bit by
        // not supporting this case
        _unlock();
        errno = ELOOP;
        return -1;
    }

    if (op == PSEUDO_EPOLL_CTL_ADD || op == PSEUDO_EPOLL_CTL_MOD) {
#ifdef DEBUG_EPOLL
        ALOGD("pseudo_epoll_ctl(epfd:%i, op:%s, fd:%i): adding/modding fd %i. IN stat:%i, OUT stat:%i", epfd, __op_to_str(op), fd, fd, event->events & PSEUDO_EPOLLIN, event->events & PSEUDO_EPOLLOUT);
#endif

        epollElement ele;
        ele.e = *event;
        ele.fd = fd;
        epoll->interest->emplace(fd, ele);
        _unlock();
        return 0;
    }

    epoll->interest->erase(fd);
    _unlock();
    return 0;
}

int pseudo_epoll_wait(int epfd, struct pseudo_epoll_event *events, int maxevents, int timeout) {
#ifdef DEBUG_EPOLL
    ALOGD("pseudo_epoll_wait: epfd: %i; events: 0x%x; maxevents: %i, timeout: %i", epfd, events, maxevents, timeout);
#endif

    // fd out of our defined bounds
    if (epfd < EPOLL_FD_MARGIN || epfd > EPOLL_FD_MARGIN + EPOLL_FD_MAX) {
#ifdef DEBUG_EPOLL
        ALOGD("pseudo_epoll_wait: epoll fd out of bounds");
#endif

        errno = EBADF;
        return -1;
    }

    if (maxevents <= 0) {
#ifdef DEBUG_EPOLL
        ALOGD("pseudo_epoll_wait: maxevents <= 0");
#endif

        errno = EINVAL;
        return -1;
    }

    _lock();

    _epoll_fd_internal * fd = nullptr;
    for (int i = 0; i < EPOLL_FD_MAX; ++i) {
        if (epoll_fd_pool[i].fd == epfd) {
            fd = &epoll_fd_pool[i];
            break;
        }
    }

    if (!fd) {
#ifdef DEBUG_EPOLL
        ALOGD("pseudo_epoll_wait: epoll fd not found in pool");
#endif

        _unlock();
        errno = EINVAL;
        return -1;
    }

    uint64_t time_started = AFN_timeMillis();
    int eventsReported = 0;

    for (;;) {
        for (auto & e : *fd->interest) {
            bool is_readable, is_writeable;

            if (is_eventfd(e.first)) {
                pseudo_eventfd_status(e.first, &is_readable, &is_writeable);
            } else if (is_pipe(e.first)) {
                pseudo_pipe_status(e.first, &is_readable, &is_writeable);
            } else {
#ifdef DEBUG_EPOLL
                ALOGD("pseudo_epoll_wait: unknown fd type for fd %i", e.first);
#endif
                continue;
            }

            if ((e.second.e.events & PSEUDO_EPOLLIN && is_readable) || (e.second.e.events & PSEUDO_EPOLLOUT && is_writeable)) {
                if (eventsReported >= maxevents) {
                    break;
                }

                memcpy(&events[eventsReported], &e.second.e, sizeof(pseudo_epoll_event));
                events[eventsReported].events = 0;
                if (e.second.e.events & PSEUDO_EPOLLIN && is_readable) events[eventsReported].events |= PSEUDO_EPOLLIN;
                if (e.second.e.events & PSEUDO_EPOLLOUT && is_writeable) events[eventsReported].events |= PSEUDO_EPOLLOUT;

#ifdef DEBUG_EPOLL
                int __x = (e.second.e.events & PSEUDO_EPOLLIN && is_readable);
                int __y = (e.second.e.events & PSEUDO_EPOLLOUT && is_writeable);
                if (__x && __y) {
                    ALOGD("pseudo_epoll_wait: reporting events IN+OUT for fd %i", e.first);
                } else if (__x) {
                    ALOGD("pseudo_epoll_wait: reporting event IN for fd %i", e.first);
                } else if (__y) {
                    ALOGD("pseudo_epoll_wait: reporting event OUT for fd %i", e.first);
                }
#endif
                eventsReported++;
            }
        }

        if (timeout == 0) goto done;
        if (eventsReported >= maxevents) goto done;
        if (timeout == -1 && eventsReported > 0) goto done;

        if (timeout != -1) {
            if (AFN_timeMillis() - time_started > timeout) goto done;
        }

        _unlock();
        usleep(10000); // give a chance for other threads to add new FDs to pool
        _lock();
    }

done:
    _unlock();
    return eventsReported;
}

// Declare read_delegate to be used from external
extern "C" ssize_t read_delegate(int fd, void *buf, size_t count);


ssize_t pseudo_read(int fd, void *buf, size_t count) {
    if (is_eventfd(fd)) {
        return pseudo_eventfd_read(fd, buf, count);
    } else if (is_pipe(fd)) {
        return pseudo_pipe_read(fd, buf, count);
    } else {
        // not eventfd or pipe, fallback to normal read
        return read_delegate(fd, buf, count);
    }
}

ssize_t pseudo_write(int fd, const void *buf, size_t count) {
    if (is_eventfd(fd)) {
        return pseudo_eventfd_write(fd, buf, count);
    } else if (is_pipe(fd)) {
        return pseudo_pipe_write(fd, buf, count);
    } else {
        // not eventfd or pipe, fallback to normal write
        return write(fd, buf, count);
    }
}
