// /*
//  * based on frameworks/native/include/android/sensor.h,
//  * Copyright (C) 2010 The Android Open Source Project
//  *
//  * Licensed under the Apache License, Version 2.0.
//  */

// #ifndef ANDROID_SENSOR_H
// #define ANDROID_SENSOR_H

// #include <stdint.h>
// #include <stdlib.h>
// #include <stdbool.h>
// #include <sys/types.h>

// #include "ALooper.h"

// #ifdef __cplusplus
// extern "C" {
// #endif

// // [Non-standard]: Max count of sensors supported by a manager/queue
// #define ASENSOR_COUNT_MAX 48

// /**
//  * Sensor types.
//  *
//  * See
//  * [android.hardware.SensorEvent#values](https://developer.android.com/reference/android/hardware/SensorEvent.html#values)
//  * for detailed explanations of the data returned for each of these types.
//  */
// enum {
//     /**
//      * Invalid sensor type. Returned by {@link ASensor_getType} as error value.
//      */
//     ASENSOR_TYPE_INVALID = -1,
//     /**
//      * {@link ASENSOR_TYPE_ACCELEROMETER}
//      * reporting-mode: continuous
//      *
//      *  All values are in SI units (m/s^2) and measure the acceleration of the
//      *  device minus the force of gravity.
//      */
//     ASENSOR_TYPE_ACCELEROMETER       = 1,
//     /**
//      * {@link ASENSOR_TYPE_MAGNETIC_FIELD}
//      * reporting-mode: continuous
//      *
//      *  All values are in micro-Tesla (uT) and measure the geomagnetic
//      *  field in the X, Y and Z axis.
//      */
//     ASENSOR_TYPE_MAGNETIC_FIELD      = 2,
//     /**
//      * {@link ASENSOR_TYPE_GYROSCOPE}
//      * reporting-mode: continuous
//      *
//      *  All values are in radians/second and measure the rate of rotation
//      *  around the X, Y and Z axis.
//      */
//     ASENSOR_TYPE_GYROSCOPE           = 4,
//     /**
//      * {@link ASENSOR_TYPE_LIGHT}
//      * reporting-mode: on-change
//      *
//      * The light sensor value is returned in SI lux units.
//      */
//     ASENSOR_TYPE_LIGHT               = 5,
//     /**
//      * {@link ASENSOR_TYPE_PRESSURE}
//      *
//      * The pressure sensor value is returned in hPa (millibar).
//      */
//     ASENSOR_TYPE_PRESSURE            = 6,
//     /**
//      * {@link ASENSOR_TYPE_PROXIMITY}
//      * reporting-mode: on-change
//      *
//      * The proximity sensor which turns the screen off and back on during calls is the
//      * wake-up proximity sensor. Implement wake-up proximity sensor before implementing
//      * a non wake-up proximity sensor. For the wake-up proximity sensor set the flag
//      * SENSOR_FLAG_WAKE_UP.
//      * The value corresponds to the distance to the nearest object in centimeters.
//      */
//     ASENSOR_TYPE_PROXIMITY           = 8,
//     /**
//      * {@link ASENSOR_TYPE_GRAVITY}
//      *
//      * All values are in SI units (m/s^2) and measure the direction and
//      * magnitude of gravity. When the device is at rest, the output of
//      * the gravity sensor should be identical to that of the accelerometer.
//      */
//     ASENSOR_TYPE_GRAVITY             = 9,
//     /**
//      * {@link ASENSOR_TYPE_LINEAR_ACCELERATION}
//      * reporting-mode: continuous
//      *
//      *  All values are in SI units (m/s^2) and measure the acceleration of the
//      *  device not including the force of gravity.
//      */
//     ASENSOR_TYPE_LINEAR_ACCELERATION = 10,
//     /**
//      * {@link ASENSOR_TYPE_ROTATION_VECTOR}
//      */
//     ASENSOR_TYPE_ROTATION_VECTOR     = 11,
//     /**
//      * {@link ASENSOR_TYPE_RELATIVE_HUMIDITY}
//      *
//      * The relative humidity sensor value is returned in percent.
//      */
//     ASENSOR_TYPE_RELATIVE_HUMIDITY   = 12,
//     /**
//      * {@link ASENSOR_TYPE_AMBIENT_TEMPERATURE}
//      *
//      * The ambient temperature sensor value is returned in Celcius.
//      */
//     ASENSOR_TYPE_AMBIENT_TEMPERATURE = 13,
//     /**
//      * {@link ASENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED}
//      */
//     ASENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED = 14,
//     /**
//      * {@link ASENSOR_TYPE_GAME_ROTATION_VECTOR}
//      */
//     ASENSOR_TYPE_GAME_ROTATION_VECTOR = 15,
//     /**
//      * {@link ASENSOR_TYPE_GYROSCOPE_UNCALIBRATED}
//      */
//     ASENSOR_TYPE_GYROSCOPE_UNCALIBRATED = 16,
//     /**
//      * {@link ASENSOR_TYPE_SIGNIFICANT_MOTION}
//      */
//     ASENSOR_TYPE_SIGNIFICANT_MOTION = 17,
//     /**
//      * {@link ASENSOR_TYPE_STEP_DETECTOR}
//      */
//     ASENSOR_TYPE_STEP_DETECTOR = 18,
//     /**
//      * {@link ASENSOR_TYPE_STEP_COUNTER}
//      */
//     ASENSOR_TYPE_STEP_COUNTER = 19,
//     /**
//      * {@link ASENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR}
//      */
//     ASENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR = 20,
//     /**
//      * {@link ASENSOR_TYPE_HEART_RATE}
//      */
//     ASENSOR_TYPE_HEART_RATE = 21,
//     /**
//      * {@link ASENSOR_TYPE_POSE_6DOF}
//      */
//     ASENSOR_TYPE_POSE_6DOF = 28,
//     /**
//      * {@link ASENSOR_TYPE_STATIONARY_DETECT}
//      */
//     ASENSOR_TYPE_STATIONARY_DETECT = 29,
//     /**
//      * {@link ASENSOR_TYPE_MOTION_DETECT}
//      */
//     ASENSOR_TYPE_MOTION_DETECT = 30,
//     /**
//      * {@link ASENSOR_TYPE_HEART_BEAT}
//      */
//     ASENSOR_TYPE_HEART_BEAT = 31,
//     /**
//      * {@link ASENSOR_TYPE_LOW_LATENCY_OFFBODY_DETECT}
//      */
//     ASENSOR_TYPE_LOW_LATENCY_OFFBODY_DETECT = 34,
//     /**
//      * {@link ASENSOR_TYPE_ACCELEROMETER_UNCALIBRATED}
//      */
//     ASENSOR_TYPE_ACCELEROMETER_UNCALIBRATED = 35,
// };

// /*
//  * Sensor Reporting Modes.
//  */
// enum {
//     AREPORTING_MODE_CONTINUOUS = 0,
//     AREPORTING_MODE_ON_CHANGE = 1,
//     AREPORTING_MODE_ONE_SHOT = 2,
//     AREPORTING_MODE_SPECIAL_TRIGGER = 3
// };
// /*
//  * A few useful constants
//  */
// /* Earth's gravity in m/s^2 */
// #define ASENSOR_STANDARD_GRAVITY            (9.80665f)
// /* Maximum magnetic field on Earth's surface in uT */
// #define ASENSOR_MAGNETIC_FIELD_EARTH_MAX    (60.0f)
// /* Minimum magnetic field on Earth's surface in uT*/
// #define ASENSOR_MAGNETIC_FIELD_EARTH_MIN    (30.0f)

// /**
//  * A sensor event.
//  */

// /* NOTE: changes to these structs have to be backward compatible */
// typedef struct ASensorVector {
//     union {
//         float v[3];
//         struct {
//             float x;
//             float y;
//             float z;
//         };
//         struct {
//             float azimuth;
//             float pitch;
//             float roll;
//         };
//     };
//     int8_t status;
//     uint8_t reserved[3];
// } ASensorVector;

// typedef struct AMetaDataEvent {
//     int32_t what;
//     int32_t sensor;
// } AMetaDataEvent;

// typedef struct AUncalibratedEvent {
//     union {
//         float uncalib[3];
//         struct {
//             float x_uncalib;
//             float y_uncalib;
//             float z_uncalib;
//         };
//     };
//     union {
//         float bias[3];
//         struct {
//             float x_bias;
//             float y_bias;
//             float z_bias;
//         };
//     };
// } AUncalibratedEvent;

// typedef struct AHeartRateEvent {
//     float bpm;
//     int8_t status;
// } AHeartRateEvent;

// typedef struct ADynamicSensorEvent {
//     int32_t  connected;
//     int32_t  handle;
// } ADynamicSensorEvent;

// typedef struct AAdditionalInfoEvent {
//     /**
//      * Event type, such as ASENSOR_ADDITIONAL_INFO_BEGIN, ASENSOR_ADDITIONAL_INFO_END and others.
//      * Refer to {@link ASENSOR_TYPE_ADDITIONAL_INFO} for the expected reporting behavior.
//      */
//     int32_t type;
//     int32_t serial;
//     union {
//         int32_t data_int32[14];
//         float   data_float[14];
//     };
// } AAdditionalInfoEvent;

// typedef struct AHeadTrackerEvent {
//     /**
//      * The fields rx, ry, rz are an Euler vector (rotation vector, i.e. a vector
//      * whose direction indicates the axis of rotation and magnitude indicates
//      * the angle to rotate around that axis) representing the transform from
//      * the (arbitrary, possibly slowly drifting) reference frame to the
//      * head frame. Expressed in radians. Magnitude of the vector must be
//      * in the range [0, pi], while the value of individual axes are
//      * in the range [-pi, pi].
//      */
//     float rx;
//     float ry;
//     float rz;

//     /**
//      * The fields vx, vy, vz are an Euler vector (rotation vector) representing
//      * the angular velocity of the head (relative to itself), in radians per
//      * second. The direction of this vector indicates the axis of rotation, and
//      * the magnitude indicates the rate of rotation.
//      */
//     float vx;
//     float vy;
//     float vz;

//     /**
//      * This value changes each time the reference frame is suddenly and
//      * significantly changed, for example if an orientation filter algorithm
//      * used for determining the orientation has had its state reset.
//      */
//     int32_t discontinuity_count;
// } AHeadTrackerEvent;

// typedef struct ALimitedAxesImuEvent {
//     union {
//         float calib[3];
//         struct {
//             float x;
//             float y;
//             float z;
//         };
//     };
//     union {
//         float supported[3];
//         struct {
//             float x_supported;
//             float y_supported;
//             float z_supported;
//         };
//     };
// } ALimitedAxesImuEvent;

// typedef struct ALimitedAxesImuUncalibratedEvent {
//     union {
//         float uncalib[3];
//         struct {
//             float x_uncalib;
//             float y_uncalib;
//             float z_uncalib;
//         };
//     };
//     union {
//         float bias[3];
//         struct {
//             float x_bias;
//             float y_bias;
//             float z_bias;
//         };
//     };
//     union {
//         float supported[3];
//         struct {
//             float x_supported;
//             float y_supported;
//             float z_supported;
//         };
//     };
// } ALimitedAxesImuUncalibratedEvent;

// typedef struct AHeadingEvent {
//     /**
//      * The direction in which the device is pointing relative to true north in
//      * degrees. The value must be between 0.0 (inclusive) and 360.0 (exclusive),
//      * with 0 indicating north, 90 east, 180 south, and 270 west.
//      */
//     float heading;
//     /**
//      * Accuracy is defined at 68% confidence. In the case where the underlying
//      * distribution is assumed Gaussian normal, this would be considered one
//      * standard deviation. For example, if the heading returns 60 degrees, and
//      * accuracy returns 10 degrees, then there is a 68 percent probability of
//      * the true heading being between 50 degrees and 70 degrees.
//      */
//     float accuracy;
// } AHeadingEvent;

// // LINT.IfChange
// /**
//  * Information that describes a sensor event, refer to
//  * <a href="/reference/android/hardware/SensorEvent">SensorEvent</a> for additional
//  * documentation.
//  *
//  * NOTE: changes to this struct has to be backward compatible and reflected in
//  * sensors_event_t
//  */
// typedef struct ASensorEvent {
//     int32_t version; /* sizeof(struct ASensorEvent) */
//     int32_t sensor;  /** The sensor that generates this event */
//     int32_t type;    /** Sensor type for the event, such as {@link ASENSOR_TYPE_ACCELEROMETER} */
//     int32_t reserved0; /** do not use */
//     /**
//      * The time in nanoseconds at which the event happened, and its behavior
//      * is identical to <a href="/reference/android/hardware/SensorEvent#timestamp">
//      * SensorEvent::timestamp</a> in Java API.
//      */
//     int64_t timestamp;
//     union {
//         union {
//             float           data[16];
//             ASensorVector   vector;
//             ASensorVector   acceleration;
//             ASensorVector   gyro;
//             ASensorVector   magnetic;
//             float           temperature;
//             float           distance;
//             float           light;
//             float           pressure;
//             float           relative_humidity;
//             AUncalibratedEvent uncalibrated_acceleration;
//             AUncalibratedEvent uncalibrated_gyro;
//             AUncalibratedEvent uncalibrated_magnetic;
//             AMetaDataEvent meta_data;
//             AHeartRateEvent heart_rate;
//             ADynamicSensorEvent dynamic_sensor_meta;
//             AAdditionalInfoEvent additional_info;
//             AHeadTrackerEvent head_tracker;
//             ALimitedAxesImuEvent limited_axes_imu;
//             ALimitedAxesImuUncalibratedEvent limited_axes_imu_uncalibrated;
//             AHeadingEvent heading;
//         };
//         union {
//             uint64_t        data[8];
//             uint64_t        step_counter;
//         } u64;
//     };

//     uint32_t flags;
//     int32_t reserved1[3];
// } ASensorEvent;

// struct ASensorManager;
// /**
//  * {@link ASensorManager} is an opaque type to manage sensors and
//  * events queues.
//  *
//  * {@link ASensorManager} is a singleton that can be obtained using
//  * ASensorManager_getInstance().
//  *
//  * This file provides a set of functions that uses {@link
//  * ASensorManager} to access and list hardware sensors, and
//  * create and destroy event queues:
//  * - ASensorManager_getSensorList()
//  * - ASensorManager_getDefaultSensor()
//  * - ASensorManager_getDefaultSensorEx()
//  * - ASensorManager_createEventQueue()
//  * - ASensorManager_destroyEventQueue()
//  */
// typedef struct ASensorManager ASensorManager;


// struct ASensorEventQueue;
// /**
//  * {@link ASensorEventQueue} is an opaque type that provides access to
//  * {@link ASensorEvent} from hardware sensors.
//  *
//  * A new {@link ASensorEventQueue} can be obtained using ASensorManager_createEventQueue().
//  *
//  * This file provides a set of functions to enable and disable
//  * sensors, check and get events, and set event rates on a {@link
//  * ASensorEventQueue}.
//  * - ASensorEventQueue_enableSensor()
//  * - ASensorEventQueue_disableSensor()
//  * - ASensorEventQueue_hasEvents()
//  * - ASensorEventQueue_getEvents()
//  * - ASensorEventQueue_setEventRate()
//  * - ASensorEventQueue_requestAdditionalInfoEvents()
//  */
// typedef struct ASensorEventQueue ASensorEventQueue;

// struct ASensor;
// /**
//  * {@link ASensor} is an opaque type that provides information about
//  * an hardware sensors.
//  *
//  * A {@link ASensor} pointer can be obtained using
//  * ASensorManager_getDefaultSensor(),
//  * ASensorManager_getDefaultSensorEx() or from a {@link ASensorList}.
//  *
//  * This file provides a set of functions to access properties of a
//  * {@link ASensor}:
//  * - ASensor_getName()
//  * - ASensor_getVendor()
//  * - ASensor_getType()
//  * - ASensor_getResolution()
//  * - ASensor_getMinDelay()
//  * - ASensor_getFifoMaxEventCount()
//  * - ASensor_getFifoReservedEventCount()
//  * - ASensor_getStringType()
//  * - ASensor_getReportingMode()
//  * - ASensor_isWakeUpSensor()
//  * - ASensor_getHandle()
//  */
// typedef struct ASensor ASensor;

// /**
//  * Enable the selected sensor at default sampling rate.
//  *
//  * Start event reports of a sensor to specified sensor event queue at a default rate.
//  *
//  * \param queue {@link ASensorEventQueue} for sensor event to be report to.
//  * \param sensor {@link ASensor} to be enabled.
//  *
//  * \return 0 on success or a negative error code on failure.
//  */
// int ASensorEventQueue_enableSensor(ASensorEventQueue* queue, ASensor const* sensor);

// /**
//  * Disable the selected sensor.
//  *
//  * Stop event reports from the sensor to specified sensor event queue.
//  *
//  * \param queue {@link ASensorEventQueue} to be changed
//  * \param sensor {@link ASensor} to be disabled
//  * \return 0 on success or a negative error code on failure.
//  */
// int ASensorEventQueue_disableSensor(ASensorEventQueue* queue, ASensor const* sensor);


// /**
//  * Retrieve pending events in sensor event queue
//  *
//  * Retrieve next available events from the queue to a specified event array.
//  *
//  * \param queue {@link ASensorEventQueue} to get events from
//  * \param events pointer to an array of {@link ASensorEvent}.
//  * \param count max number of event that can be filled into array event.
//  * \return number of events returned on success; negative error code when
//  *         no events are pending or an error has occurred.
//  *
//  * Examples:
//  *
//  *     ASensorEvent event;
//  *     ssize_t numEvent = ASensorEventQueue_getEvents(queue, &event, 1);
//  *
//  *     ASensorEvent eventBuffer[8];
//  *     ssize_t numEvent = ASensorEventQueue_getEvents(queue, eventBuffer, 8);
//  *
//  */
// ssize_t ASensorEventQueue_getEvents(ASensorEventQueue* queue, ASensorEvent* events, size_t count);

// /**
//  * [Non-standard]: Enqueue sensor event
//  */
// void ASensorEventQueue_enqueueEvent(ASensorEventQueue * queue, ASensorEvent * event);

// /**
//  * [Non-standard]: Get enabled sensors for the queue
//  *
//  * Writes array of enabled sensors to `sensors` arg.
//  * `sensors` must be able to contain no less than ASENSOR_COUNT_MAX integers
//  */
// void ASensorEventQueue_getEnabledSensors(ASensorEventQueue * queue, ASensor * sensors[ASENSOR_COUNT_MAX]);

// /**
//  * Sets the delivery rate of events in microseconds for the given sensor.
//  *
//  * This function has to be called after {@link ASensorEventQueue_enableSensor}.
//  * Note that this is a hint only, generally event will arrive at a higher
//  * rate. It is an error to set a rate inferior to the value returned by
//  * ASensor_getMinDelay().
//  *
//  * \param queue {@link ASensorEventQueue} to which sensor event is delivered.
//  * \param sensor {@link ASensor} of which sampling rate to be updated.
//  * \param usec sensor sampling period (1/sampling rate) in microseconds
//  * \return 0 on sucess or a negative error code on failure.
//  */
// int ASensorEventQueue_setEventRate(ASensorEventQueue* queue, ASensor const* sensor, int32_t usec);

// /**
//  * Creates a new sensor event queue and associate it with a looper.
//  *
//  * "ident" is a identifier for the events that will be returned when
//  * calling ALooper_pollOnce(). The identifier must be >= 0, or
//  * ALOOPER_POLL_CALLBACK if providing a non-NULL callback.
//  */
// ASensorEventQueue* ASensorManager_createEventQueue(ASensorManager* manager,
//                                                    ALooper* looper, int ident, ALooper_callbackFunc callback, void* data);

// /**
//  * Returns the default sensor for the given type, or NULL if no sensor
//  * of that type exists.
//  */
// ASensor const* ASensorManager_getDefaultSensor(ASensorManager* manager, int type);

// /**
//  * Get a reference to the sensor manager. ASensorManager is a singleton
//  * per package as different packages may have access to different sensors.
//  *
//  * Deprecated: Use ASensorManager_getInstanceForPackage(const char*) instead.
//  *
//  * Example:
//  *
//  *     ASensorManager* sensorManager = ASensorManager_getInstance();
//  *
//  */
// ASensorManager* ASensorManager_getInstance();

// /*****************************************************************************/
// /*
//  * Returns this sensor's name (non localized)
//  */
// const char* ASensor_getName(ASensor const* sensor);
// /*
//  * Returns this sensor's vendor's name (non localized)
//  */
// const char* ASensor_getVendor(ASensor const* sensor);
// /*
//  * Return this sensor's type
//  */
// int ASensor_getType(ASensor const* sensor);
// /*
//  * Returns this sensors's resolution
//  */
// float ASensor_getResolution(ASensor const* sensor);
// /*
//  * Returns the minimum delay allowed between events in microseconds.
//  * A value of zero means that this sensor doesn't report events at a
//  * constant rate, but rather only when a new data is available.
//  */
// int ASensor_getMinDelay(ASensor const* sensor);
// /*
//  * Returns the maximum size of batches for this sensor. Batches will often be
//  * smaller, as the hardware fifo might be used for other sensors.
//  */
// int ASensor_getFifoMaxEventCount(ASensor const* sensor);
// /*
//  * Returns the hardware batch fifo size reserved to this sensor.
//  */
// int ASensor_getFifoReservedEventCount(ASensor const* sensor);
// /*
//  * Returns this sensor's string type.
//  */
// const char* ASensor_getStringType(ASensor const* sensor);
// /*
//  * Returns the reporting mode for this sensor. One of AREPORTING_MODE_* constants.
//  */
// int ASensor_getReportingMode(ASensor const* sensor);
// /*
//  * Returns true if this is a wake up sensor, false otherwise.
//  */
// bool ASensor_isWakeUpSensor(ASensor const* sensor);
// /*
//  * [Non-standard] Returns this sensor's handle.
//  */
// int ASensor_getHandle(ASensor const* sensor);


// #ifdef __cplusplus
// };
// #endif

// #endif // ANDROID_SENSOR_H
