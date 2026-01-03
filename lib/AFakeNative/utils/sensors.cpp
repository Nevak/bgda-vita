// #include "sensors.h"
// #include "AFakeNative/AFakeNative_Utils.h"

// #include <psp2/motion.h>
// #include <pthread.h>
// #include <psp2/kernel/threadmgr.h>

// ASensorEventQueue * sensorEventQueue;

// void sensors_init(ASensorEventQueue * queue) {
//     // Enable sensors
//     sceMotionStartSampling();

//     sensorEventQueue = queue;

//     pthread_t t;
//     pthread_attr_t attr;
//     pthread_attr_init(&attr);
//     pthread_attr_setstacksize(&attr, 32*1024);
//     pthread_create(&t, &attr, sensors_thread, nullptr);
//     pthread_detach(t);
// }

// void * sensors_thread(void * arg) {
//     while (true) {
//         sensors_poll();
//         sceKernelDelayThread(15000);
//     }
// }

// void sensors_poll() {
//     ASensor* sensors[ASENSOR_COUNT_MAX];
//     ASensorEventQueue_getEnabledSensors(sensorEventQueue, sensors);

//     SceMotionSensorState sensor;
//     sceMotionGetSensorState(&sensor, 1);

//     for (auto * s : sensors) {
//         if (!s) break;

//         switch (ASensor_getType(s)) {
//         case ASENSOR_TYPE_ACCELEROMETER: {
//             ASensorVector v;
//             v.y = sensor.accelerometer.x * ASENSOR_STANDARD_GRAVITY * -1;
//             v.z = sensor.accelerometer.y * ASENSOR_STANDARD_GRAVITY * -1;
//             v.x = sensor.accelerometer.z * ASENSOR_STANDARD_GRAVITY * -1;

//             ASensorEvent e;
//             e.version = sizeof(ASensorEvent);
//             e.type = ASENSOR_TYPE_ACCELEROMETER;
//             e.sensor = ASensor_getHandle(s);
//             e.acceleration = v;

//             ASensorEventQueue_enqueueEvent(sensorEventQueue, &e);
//             break;
//         }
//         default: {
//             //ALOGE("sensors: not implemented sensor %s", ASensor_getStringType(s));
//             break;
//         }
//         }
//     }
// }
