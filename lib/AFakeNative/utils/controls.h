/*
 * reimpl/controls.h
 *
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef AFAKENATIVE_CONTROLS_H
#define AFAKENATIVE_CONTROLS_H

#include <psp2/touch.h>
#include <psp2/ctrl.h>
#include <psp2/motion.h>

#include <math.h>
#include "AFakeNative/AInput.h"

#define GRAVITY_CONSTANT 9.807f

#define kIdRawPointerCancel 0xe
#define kIdRawPointerDown 0x6000e
#define kIdRawPointerMove 0x4000e
#define kIdRawPointerUp 0x8000e
#define kIdUndefined 0

#define kModuleTypeIdTouchScreen 1000
#define kModuleTypeIdTouchPad 1100

enum {
    ACTION_DOWN = 1,
    ACTION_UP   = 2,
    ACTION_MOVE = 3,
};

typedef struct {
    uint32_t sce_button;
    int32_t android_button;
} ButtonMapping;

#ifdef __cplusplus
extern "C" {
#endif

void controls_init(AInputQueue * queue);
void * controls_poll(void * arg);
void pollTouch();
void pollPad();
void pollAccel();
void runSilentStartHelper();
int detectControllers();

#ifdef __cplusplus
}
#endif

#endif // AFAKENATIVE
