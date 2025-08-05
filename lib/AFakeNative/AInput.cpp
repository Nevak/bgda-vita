#include "AInput.h"
#include "PseudoEpoll.h"
#include "AFakeNative_Utils.h"
#include "AFakeNative/utils/controls.h"
#include "AFakeNative/polling/pseudo_eventfd.h"

#include <vector>
#include <pthread.h>
#include <cstring>

static AInputQueue * g_AInputQueue = nullptr;

typedef struct inputQueue {
    int mDispatchFd;
    std::vector<ALooper*> mAppLoopers;
    //ALooper * mDispatchLooper;
    //sp<WeakMessageHandler> mHandler;
    //PooledInputEventFactory mPooledInputEventFactory;
    // Guards the pending and finished event vectors
    pthread_mutex_t mLock;
    std::vector<AInputEvent*> mPendingEvents;
    //std::vector<key_value_pair_t<AInputEvent*, bool> > mFinishedEvents;

} inputQueue;

AInputQueue * AInputQueue_create() {
    if (g_AInputQueue) return g_AInputQueue;

    auto * iq = reinterpret_cast<inputQueue *>(malloc(sizeof(inputQueue)));
    iq->mDispatchFd = pseudo_eventfd(0, PSEUDO_EFD_NONBLOCK | PSEUDO_EFD_SEMAPHORE);

    if (iq->mDispatchFd < 0) {
        ALOGE("eventfd creation for AInputQueue failed: %s\n", strerror(errno));
    } else {
        ALOGD("Created eventfd for AInputQueue: #%i", iq->mDispatchFd);
    }

    pthread_mutex_init(&iq->mLock, nullptr);
    g_AInputQueue = reinterpret_cast<AInputQueue *>(iq);

    controls_init(g_AInputQueue);

    return g_AInputQueue;
}

void AInputQueue_attachLooper(AInputQueue* queue, ALooper* looper,
                              int ident, ALooper_callbackFunc callback, void* data) {
    if (!queue || !looper) return;
    auto * q = reinterpret_cast<inputQueue *>(queue);

    pthread_mutex_lock(&q->mLock);

    ALOGD("AInputQueue_attachLooper called: %p, %p, %i, %p, %p", queue, looper, ident, callback, data);
    ALOGD("Total Loopers [%d]", q->mAppLoopers.size());

    for (size_t i = 0; i < q->mAppLoopers.size(); i++) {
        //ALOGD("Looper %p", q->mAppLoopers[i]);
        if (looper == q->mAppLoopers[i]) {
            ALOGD("Looper already attached %d", i);
            pthread_mutex_unlock(&q->mLock);
            return;
        }
    }

    q->mAppLoopers.push_back(looper);
#if DEBUG_POLL_AND_WAKE
    ALOGD("AInputQueue (%p) : attaching looper (%p) to our FD %i", q, looper, q->mDispatchFd);
#endif
    ALooper_addFd(looper, q->mDispatchFd, ident, ALOOPER_EVENT_INPUT, callback, data);
    pthread_mutex_unlock(&q->mLock);
}

void AInputQueue_detachLooper(AInputQueue* queue) {
    if (!queue) return;
    auto * q = reinterpret_cast<inputQueue *>(queue);

    pthread_mutex_lock(&q->mLock);
    for (size_t i = 0; i < q->mAppLoopers.size(); i++) {
        ALooper_removeFd(q->mAppLoopers[i], q->mDispatchFd);
    }
    q->mAppLoopers.clear();
    pthread_mutex_unlock(&q->mLock);
}

__attribute__((__no_instrument_function__, __no_profile_instrument_function__))
int32_t AInputQueue_getEvent(AInputQueue* queue, AInputEvent** outEvent) {
    if (!queue) {
        ALOGE("AInputQueue_getEvent: bad queue");
        return -1;
    }

    if (!outEvent) {
        ALOGE("AInputQueue_getEvent: bad outEvent");
        return -1;
    }

    auto * q = reinterpret_cast<inputQueue *>(queue);

    pthread_mutex_lock(&q->mLock);
    *outEvent = NULL;
    if (!q->mPendingEvents.empty()) {
        *outEvent = q->mPendingEvents[0];
        q->mPendingEvents.erase(q->mPendingEvents.begin());
    }

    if (q->mPendingEvents.empty()) {
        uint64_t byteread;
        ssize_t nRead;
        do {
            nRead = pseudo_read(q->mDispatchFd, &byteread, sizeof(byteread));
            if (nRead < 0 && errno != EAGAIN) {
                ALOGW("Failed to read from native dispatch pipe: %s", strerror(errno));
            }
        } while (nRead == 8); // reduce eventfd semaphore to 0
    }

    int ret = *outEvent != NULL ? 0 : -EAGAIN;
    pthread_mutex_unlock(&q->mLock);
    return ret;
}

int32_t AInputQueue_preDispatchEvent(AInputQueue* queue, AInputEvent* event) {
    // Never pre-dispatch
    return false;
}

void AInputQueue_finishEvent(AInputQueue* queue, AInputEvent* event, int handled) {
    if (event) free(event);
}

void AInputQueue_enqueueEvent(AInputQueue* queue, AInputEvent* event) {
    if (!queue || !event) return;
    auto * q = reinterpret_cast<inputQueue *>(queue);

    pthread_mutex_lock(&q->mLock);
    q->mPendingEvents.push_back(event);
    if (q->mPendingEvents.size() == 1) {
        uint64_t payload = 1;
        int res = TEMP_FAILURE_RETRY(pseudo_write(q->mDispatchFd, &payload, sizeof(payload)));
        if (res < 0 && errno != EAGAIN) {
            ALOGW("Failed writing to dispatch fd: %s", strerror(errno));
        }
    }
    pthread_mutex_unlock(&q->mLock);
}

/**
 * ========================
 */

AInputEvent *AInputEvent_create(const inputEvent *e) {
    auto * ret = reinterpret_cast<AInputEvent *>(malloc(sizeof(inputEvent)));
    memcpy(ret, e, sizeof(inputEvent));
    return ret;
}

int32_t AInputEvent_getType(const AInputEvent* event) {
    if (!event) return -1;
    auto * e = reinterpret_cast<const inputEvent *>(event);
    return e->type;
}

int32_t AInputEvent_getSource(const AInputEvent* event) {
    if (!event) return AINPUT_SOURCE_UNKNOWN;
    auto * e = reinterpret_cast<const inputEvent *>(event);
    return e->source;
}

int32_t AKeyEvent_getAction(const AInputEvent* key_event) {
    if (!key_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(key_event);
    return e->action;
}

int32_t AKeyEvent_getKeyCode(const AInputEvent* key_event) {
    if (!key_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(key_event);
    return e->keycode;
}

int32_t AKeyEvent_getRepeatCount(const AInputEvent* key_event) {
    if (!key_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(key_event);
    return e->repeatcount;
}

int32_t AKeyEvent_getScanCode(const AInputEvent* key_event) {
    if (!key_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(key_event);
    return e->scancode;
}

int32_t AMotionEvent_getAction(const AInputEvent* motion_event) {
    if (!motion_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(motion_event);
    return e->motion_action;
}

size_t AMotionEvent_getPointerCount(const AInputEvent* motion_event) {
    if (!motion_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(motion_event);
    return e->motion_ptrcount;
}

int32_t AMotionEvent_getPointerId(const AInputEvent* motion_event, size_t pointer_index) {
    if (!motion_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(motion_event);
    if (pointer_index >= 10) pointer_index = 9;
    return e->motion_ptridx[pointer_index];
}

float AMotionEvent_getX(const AInputEvent* motion_event, size_t pointer_index) {
    if (!motion_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(motion_event);
    if (pointer_index >= 10) pointer_index = 9;
    return e->motion_x[pointer_index];
}

float AMotionEvent_getY(const AInputEvent* motion_event, size_t pointer_index) {
    if (!motion_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(motion_event);
    if (pointer_index >= 10) pointer_index = 9;
    return e->motion_y[pointer_index];
}

float AMotionEvent_getAxisValue(const AInputEvent* motion_event,
int32_t axis, size_t pointer_index) {
    if (!motion_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(motion_event);
    switch (axis) {
        case AMOTION_EVENT_AXIS_X:
            return e->motion_x[pointer_index];
        case AMOTION_EVENT_AXIS_Y:
            return e->motion_y[pointer_index];
        case AMOTION_EVENT_AXIS_Z:
            return e->motion_z[pointer_index];
        case AMOTION_EVENT_AXIS_RZ:
            return e->motion_rz[pointer_index];
        case AMOTION_EVENT_AXIS_HAT_X:
            return e->motion_hat_x[pointer_index];
        case AMOTION_EVENT_AXIS_HAT_Y:
            return e->motion_hat_y[pointer_index];
        case AMOTION_EVENT_AXIS_LTRIGGER:
            return e->motion_lt[pointer_index];
        case AMOTION_EVENT_AXIS_RTRIGGER:
            return e->motion_rt[pointer_index];
        case AMOTION_EVENT_AXIS_RX:
        case AMOTION_EVENT_AXIS_RY:
        case AMOTION_EVENT_AXIS_GAS:
        case AMOTION_EVENT_AXIS_BRAKE:
            // These are often requested for joysticks but not applicable to the Vita
            return 0;
        default:
            ALOGE("AMotionEvent_getAxisValue: unexpected axis %i", axis);
            return 0;
    }
}

float AMotionEvent_getHistoricalAxisValue(const AInputEvent* motion_event,
                                          int32_t axis, size_t pointer_index, size_t history_index) {
    // FIXME: Actual history impl here
    return AMotionEvent_getAxisValue(motion_event, axis, pointer_index);
}
