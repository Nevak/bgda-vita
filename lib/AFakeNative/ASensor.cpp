// #include <cstdio>
// #include <pthread.h>
// #include <cstring>
// #include <map>
// #include <vector>
// #include <algorithm>

// #include "ASensor.h"
// #include "AFakeNative_Utils.h"
// #include "AFakeNative/utils/sensors.h"
// #include "PseudoEpoll.h"
// #include "AFakeNative/polling/pseudo_eventfd.h"

// static ASensorManager * g_ASensorManager = nullptr;
// static ASensorEventQueue * g_ASensorEventQueue = nullptr;

// typedef struct aSensor {
//     int handle = -1;
//     int type = ASENSOR_TYPE_INVALID;

//     const char * name = "Unknown Sensor";
//     const char * vendor = "Sony";

//     float resolution = 0.001f;
//     int minDelay = 1000; // useconds
//     int fifoMaxEventCount = 100;
//     int fifoReservedEventCound = 1;
//     int reportingMode = AREPORTING_MODE_CONTINUOUS;
//     bool isWakeUpSensor = false;
// } aSensor;

// typedef struct sensorManager {
//     pthread_mutex_t mLock;
//     std::map<int, aSensor*> * sensors;
// } sensorManager;

// typedef struct sensorEventQueue {
//     int mDispatchFd;
//     std::vector<ALooper*> * mAppLoopers;
//     std::vector<ASensor*> * mSensors;
//     pthread_mutex_t mLock;
//     std::vector<ASensorEvent> * mPendingEvents;
// } sensorEventQueue;

// const char * asensor_type_str(int t) {
//     switch (t) {
//         case ASENSOR_TYPE_ACCELEROMETER:
//             return "ASENSOR_TYPE_ACCELEROMETER";
//         case ASENSOR_TYPE_ACCELEROMETER_UNCALIBRATED:
//             return "ASENSOR_TYPE_ACCELEROMETER_UNCALIBRATED";
//         case ASENSOR_TYPE_AMBIENT_TEMPERATURE:
//             return "ASENSOR_TYPE_AMBIENT_TEMPERATURE";
//         case ASENSOR_TYPE_GAME_ROTATION_VECTOR:
//             return "ASENSOR_TYPE_GAME_ROTATION_VECTOR";
//         case ASENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR:
//             return "ASENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR";
//         case ASENSOR_TYPE_GRAVITY:
//             return "ASENSOR_TYPE_GRAVITY";
//         case ASENSOR_TYPE_GYROSCOPE:
//             return "ASENSOR_TYPE_GYROSCOPE";
//         case ASENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
//             return "ASENSOR_TYPE_GYROSCOPE_UNCALIBRATED";
//         case ASENSOR_TYPE_HEART_BEAT:
//             return "ASENSOR_TYPE_HEART_BEAT";
//         case ASENSOR_TYPE_HEART_RATE:
//             return "ASENSOR_TYPE_HEART_RATE";
//         case ASENSOR_TYPE_LIGHT:
//             return "ASENSOR_TYPE_LIGHT";
//         case ASENSOR_TYPE_LINEAR_ACCELERATION:
//             return "ASENSOR_TYPE_LINEAR_ACCELERATION";
//         case ASENSOR_TYPE_LOW_LATENCY_OFFBODY_DETECT:
//             return "ASENSOR_TYPE_LOW_LATENCY_OFFBODY_DETECT";
//         case ASENSOR_TYPE_MAGNETIC_FIELD:
//             return "ASENSOR_TYPE_MAGNETIC_FIELD";
//         case ASENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
//             return "ASENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED";
//         case ASENSOR_TYPE_MOTION_DETECT:
//             return "ASENSOR_TYPE_MOTION_DETECT";
//         case ASENSOR_TYPE_POSE_6DOF:
//             return "ASENSOR_TYPE_POSE_6DOF";
//         case ASENSOR_TYPE_PRESSURE:
//             return "ASENSOR_TYPE_PRESSURE";
//         case ASENSOR_TYPE_PROXIMITY:
//             return "ASENSOR_TYPE_PROXIMITY";
//         case ASENSOR_TYPE_RELATIVE_HUMIDITY:
//             return "ASENSOR_TYPE_RELATIVE_HUMIDITY";
//         case ASENSOR_TYPE_ROTATION_VECTOR:
//             return "ASENSOR_TYPE_ROTATION_VECTOR";
//         case ASENSOR_TYPE_SIGNIFICANT_MOTION:
//             return "ASENSOR_TYPE_SIGNIFICANT_MOTION";
//         case ASENSOR_TYPE_STATIONARY_DETECT:
//             return "ASENSOR_TYPE_STATIONARY_DETECT";
//         case ASENSOR_TYPE_STEP_COUNTER:
//             return "ASENSOR_TYPE_STEP_COUNTER";
//         case ASENSOR_TYPE_STEP_DETECTOR:
//             return "ASENSOR_TYPE_STEP_DETECTOR";
//         case ASENSOR_TYPE_INVALID:
//         default:
//             return "ASENSOR_TYPE_INVALID";
//     }
// }

// ASensorManager* ASensorManager_getInstance() {
//     if (g_ASensorManager) return g_ASensorManager;

//     sensorManager sm;

//     pthread_mutex_init(&sm.mLock, nullptr);
//     sm.sensors = new std::map<int, aSensor*>;

//     int handle = 0;

//     /**
//     * ASENSOR_TYPE_ACCELEROMETER
//     * reporting-mode: continuous
//     *
//     *  All values are in SI units (m/s^2) and measure the acceleration of the
//     *  device minus the force of gravity.
//     */
//     auto * sensor_accel = new aSensor;
//     sensor_accel->handle = handle++;
//     sensor_accel->type = ASENSOR_TYPE_ACCELEROMETER;
//     sensor_accel->name = "PSVita Built-in Accelerometer";
//     sm.sensors->insert(std::pair<int, aSensor *>(sensor_accel->type, sensor_accel));

//     /**
//      * ASENSOR_TYPE_MAGNETIC_FIELD
//      * reporting-mode: continuous
//      *
//      *  All values are in micro-Tesla (uT) and measure the geomagnetic
//      *  field in the X, Y and Z axis.
//      */
//     auto * sensor_magneto = new aSensor;
//     sensor_magneto->handle = handle++;
//     sensor_magneto->type = ASENSOR_TYPE_MAGNETIC_FIELD;
//     sensor_magneto->name = "PSVita Built-in Magnetometer";
//     sm.sensors->insert(std::pair<int, aSensor *>(sensor_magneto->type, sensor_magneto));

//     /**
//      * ASENSOR_TYPE_GYROSCOPE
//      * reporting-mode: continuous
//      *
//      *  All values are in radians/second and measure the rate of rotation
//      *  around the X, Y and Z axis.
//      */
//     auto * sensor_gyro = new aSensor;
//     sensor_gyro->handle = handle++;
//     sensor_gyro->type = ASENSOR_TYPE_GYROSCOPE;
//     sensor_gyro->name = "PSVita Built-in Gyroscope";
//     sm.sensors->insert(std::pair<int, aSensor *>(sensor_gyro->type, sensor_gyro));

//     /**
//      * ASENSOR_TYPE_GRAVITY
//      *
//      * All values are in SI units (m/s^2) and measure the direction and
//      * magnitude of gravity. When the device is at rest, the output of
//      * the gravity sensor should be identical to that of the accelerometer.
//      */
//     auto * sensor_gravity = new aSensor;
//     sensor_gravity->handle = handle++;
//     sensor_gravity->type = ASENSOR_TYPE_GRAVITY;
//     sensor_gravity->name = "PSVita Built-in Gravity Sensor";
//     sm.sensors->insert(std::pair<int, aSensor *>(sensor_gravity->type, sensor_gravity));

//     /**
//      * ASENSOR_TYPE_LINEAR_ACCELERATION
//      * reporting-mode: continuous
//      *
//      *  All values are in SI units (m/s^2) and measure the acceleration of the
//      *  device not including the force of gravity.
//      */
//     auto * sensor_accel_linear = new aSensor;
//     sensor_accel_linear->handle = handle++;
//     sensor_accel_linear->type = ASENSOR_TYPE_LINEAR_ACCELERATION;
//     sensor_accel_linear->name = "PSVita Built-in Linear Acceleration Sensor";
//     sm.sensors->insert(std::pair<int, aSensor *>(sensor_accel_linear->type, sensor_accel_linear));

//     g_ASensorManager = (ASensorManager *) malloc(sizeof(sensorManager));
//     memcpy(g_ASensorManager, &sm, sizeof(sensorManager));

//     return g_ASensorManager;
// }

// ASensor const* ASensorManager_getDefaultSensor(ASensorManager* manager, int type) {
//     if (!manager) return nullptr;
//     auto * mgr = (sensorManager *) manager;

//     if (!mgr->sensors->contains(type)) {
//         ALOGE("ASensorManager does not support sensor %s (%i)", asensor_type_str(type), type);
//         return nullptr;
//     }

//     return (ASensor const *) mgr->sensors->at(type);
// }

// ASensorEventQueue* ASensorManager_createEventQueue(ASensorManager* manager,
//                                                    ALooper* looper, int ident, ALooper_callbackFunc callback, void* data) {
//     if (!looper) return nullptr;

//     if (!g_ASensorEventQueue) {
//         sensorEventQueue seq;
//         seq.mDispatchFd = pseudo_eventfd(0, PSEUDO_EFD_NONBLOCK | PSEUDO_EFD_SEMAPHORE);
//         seq.mAppLoopers = new std::vector<ALooper *>;
//         seq.mPendingEvents = new std::vector<ASensorEvent>;
//         seq.mSensors = new std::vector<ASensor *>;

//         if (seq.mDispatchFd < 0) {
//             ALOGE("eventfd creation for ASensorEventQueue failed: %s\n", strerror(errno));
//         }

//         pthread_mutex_init(&seq.mLock, nullptr);

//         g_ASensorEventQueue = (ASensorEventQueue *) malloc(sizeof(sensorEventQueue));
//         memcpy(g_ASensorEventQueue, &seq, sizeof(sensorEventQueue));

//         sensors_init(g_ASensorEventQueue);
//     }

//     auto * q = (sensorEventQueue *) g_ASensorEventQueue;

//     pthread_mutex_lock(&q->mLock);

//     for (auto * l : * q->mAppLoopers) {
//         if (looper == l) {
//             pthread_mutex_unlock(&q->mLock);
//             return g_ASensorEventQueue;
//         }
//     }

//     q->mAppLoopers->push_back(looper);
//     ALooper_addFd(looper, q->mDispatchFd, ident, ALOOPER_EVENT_INPUT, callback, data);
//     pthread_mutex_unlock(&q->mLock);

//     return g_ASensorEventQueue;
// }

// int ASensorEventQueue_enableSensor(ASensorEventQueue* queue, ASensor const* sensor) {
//     if (!queue) return -1;

//     auto * q = (sensorEventQueue *) g_ASensorEventQueue;

//     pthread_mutex_lock(&q->mLock);

//     for (auto * s : * q->mSensors) {
//         if (sensor == s) {
//             pthread_mutex_unlock(&q->mLock);
//             return 0;
//         }
//     }

//     q->mSensors->push_back((ASensor *) sensor);
//     pthread_mutex_unlock(&q->mLock);

//     return 0;
// }

// int ASensorEventQueue_disableSensor(ASensorEventQueue* queue, ASensor const* sensor) {
//     if (!queue) return -1;
//     auto * q = (sensorEventQueue *) g_ASensorEventQueue;

//     pthread_mutex_lock(&q->mLock);

//     auto position = std::find(q->mSensors->begin(), q->mSensors->end(), sensor);
//     if (position != q->mSensors->end())
//         q->mSensors->erase(position);

//     pthread_mutex_unlock(&q->mLock);

//     return 0;
// }

// ssize_t ASensorEventQueue_getEvents(ASensorEventQueue* queue, ASensorEvent* events, size_t count) {
//     if (!queue || !events || count <= 0) return -1;
//     auto * q = (sensorEventQueue *) g_ASensorEventQueue;

//     pthread_mutex_lock(&q->mLock);

//     size_t copy = std::min(count, q->mPendingEvents->size());
//     for (size_t i = 0; i < copy; ++i) {
//         events[i] = q->mPendingEvents->at(i);
//     }
//     q->mPendingEvents->erase(q->mPendingEvents->begin(), q->mPendingEvents->begin() + copy);

//     if (q->mPendingEvents->empty()) {
//         uint64_t byteread;
//         ssize_t nRead;
//         do {
//             nRead = pseudo_read(q->mDispatchFd, &byteread, sizeof(byteread));
//             if (nRead < 0 && errno != EAGAIN) {
//                 ALOGW("Failed to read from native dispatch pipe: %s", strerror(errno));
//             }
//         } while (nRead == 8); // reduce eventfd semaphore to 0
//     }

//     pthread_mutex_unlock(&q->mLock);

//     return copy;
// }

// void ASensorEventQueue_enqueueEvent(ASensorEventQueue * queue, ASensorEvent * event) {
//     if (!queue) return;
//     auto * q = (sensorEventQueue *) queue;

//     pthread_mutex_lock(&q->mLock);
//     q->mPendingEvents->push_back(*event);
//     if (q->mPendingEvents->size() == 1) {
//         uint64_t payload = 1;
//         int res = TEMP_FAILURE_RETRY(pseudo_write(q->mDispatchFd, &payload, sizeof(payload)));
//         if (res < 0 && errno != EAGAIN) {
//             ALOGW("Failed writing to dispatch fd: %s", strerror(errno));
//         }
//     }
//     pthread_mutex_unlock(&q->mLock);
// }

// void ASensorEventQueue_getEnabledSensors(ASensorEventQueue * queue, ASensor * sensors[ASENSOR_COUNT_MAX]) {
//     if (!queue) return;
//     auto * q = (sensorEventQueue *) queue;

//     pthread_mutex_lock(&q->mLock);

//     int count = std::min((int)q->mSensors->size(), ASENSOR_COUNT_MAX);
//     for (int i = 0; i < count; ++i) {
//         sensors[i] = q->mSensors->at(i);
//     }

//     if (count < ASENSOR_COUNT_MAX) {
//         for (int i = count; i < ASENSOR_COUNT_MAX; ++i) {
//             sensors[i] = nullptr;
//         }
//     }

//     pthread_mutex_unlock(&q->mLock);
// }

// int ASensorEventQueue_setEventRate(ASensorEventQueue* queue, ASensor const* sensor, int32_t usec) {
//     // Ignore
//     return 0;
// }

// const char* ASensor_getName(ASensor const* sensor) {
//     if (!sensor) return "";
//     auto * s = (aSensor *) sensor;
//     return s->name;
// }

// const char* ASensor_getVendor(ASensor const* sensor) {
//     if (!sensor) return "";
//     auto * s = (aSensor *) sensor;
//     return s->vendor;
// }

// int ASensor_getType(ASensor const* sensor) {
//     if (!sensor) return ASENSOR_TYPE_INVALID;
//     auto * s = (aSensor *) sensor;
//     return s->type;
// }

// float ASensor_getResolution(ASensor const* sensor) {
//     if (!sensor) return 0;
//     auto * s = (aSensor *) sensor;
//     return s->resolution;
// }

// int ASensor_getMinDelay(ASensor const* sensor) {
//     if (!sensor) return 0;
//     auto * s = (aSensor *) sensor;
//     return s->minDelay;
// }

// int ASensor_getFifoMaxEventCount(ASensor const* sensor) {
//     if (!sensor) return 1;
//     auto * s = (aSensor *) sensor;
//     return s->fifoMaxEventCount;
// }

// int ASensor_getFifoReservedEventCount(ASensor const* sensor) {
//     if (!sensor) return 1;
//     auto * s = (aSensor *) sensor;
//     return s->fifoReservedEventCound;
// }

// const char* ASensor_getStringType(ASensor const* sensor) {
//     if (!sensor) return asensor_type_str(ASENSOR_TYPE_INVALID);
//     auto * s = (aSensor *) sensor;
//     return asensor_type_str(s->type);
// }

// int ASensor_getReportingMode(ASensor const* sensor) {
//     if (!sensor) return AREPORTING_MODE_CONTINUOUS;
//     auto * s = (aSensor *) sensor;
//     return s->reportingMode;
// }

// bool ASensor_isWakeUpSensor(ASensor const* sensor) {
//     if (!sensor) return false;
//     auto * s = (aSensor *) sensor;
//     return s->isWakeUpSensor;
// }

// int ASensor_getHandle(ASensor const* sensor) {
//     if (!sensor) return -1;
//     auto * s = (aSensor *) sensor;
//     return s->handle;
// }
