#ifndef _ANDROID_INPUT_H
#define _ANDROID_INPUT_H

#include <sys/cdefs.h>
#include <stdint.h>

#include "ALooper.h"
#include "keycodes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* Input event types.
*/
enum {
    /** Indicates that the input event is a key event. */
    AINPUT_EVENT_TYPE_KEY = 1,

    /** Indicates that the input event is a motion event. */
    AINPUT_EVENT_TYPE_MOTION = 2,

    /** Focus event */
    AINPUT_EVENT_TYPE_FOCUS = 3,

    /** Capture event */
    AINPUT_EVENT_TYPE_CAPTURE = 4,

    /** Drag event */
    AINPUT_EVENT_TYPE_DRAG = 5,

    /** TouchMode event */
    AINPUT_EVENT_TYPE_TOUCH_MODE = 6,
};

/**
 * Key event actions.
 */
enum {
    /** The key has been pressed down. */
    AKEY_EVENT_ACTION_DOWN = 0,

    /** The key has been released. */
    AKEY_EVENT_ACTION_UP = 1,

    /**
     * Multiple duplicate key events have occurred in a row, or a
     * complex string is being delivered.  The repeat_count property
     * of the key event contains the number of times the given key
     * code should be executed.
     */
    AKEY_EVENT_ACTION_MULTIPLE = 2
};


/**
 * Bit shift for the action bits holding the pointer index as
 * defined by AMOTION_EVENT_ACTION_POINTER_INDEX_MASK.
 */
#define AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT 8

/** Motion event actions */
enum {
    /** Bit mask of the parts of the action code that are the action itself. */
    AMOTION_EVENT_ACTION_MASK = 0xff,

    /**
     * Bits in the action code that represent a pointer index, used with
     * AMOTION_EVENT_ACTION_POINTER_DOWN and AMOTION_EVENT_ACTION_POINTER_UP.  Shifting
     * down by AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT provides the actual pointer
     * index where the data for the pointer going up or down can be found.
     */
    AMOTION_EVENT_ACTION_POINTER_INDEX_MASK = 0xff00,

    /** A pressed gesture has started, the motion contains the initial starting location. */
    AMOTION_EVENT_ACTION_DOWN = 0,

    /**
     * A pressed gesture has finished, the motion contains the final release location
     * as well as any intermediate points since the last down or move event.
     */
    AMOTION_EVENT_ACTION_UP = 1,

    /**
     * A change has happened during a press gesture (between AMOTION_EVENT_ACTION_DOWN and
     * AMOTION_EVENT_ACTION_UP).  The motion contains the most recent point, as well as
     * any intermediate points since the last down or move event.
     */
    AMOTION_EVENT_ACTION_MOVE = 2,

    /**
     * The current gesture has been aborted.
     * You will not receive any more points in it.  You should treat this as
     * an up event, but not perform any action that you normally would.
     */
    AMOTION_EVENT_ACTION_CANCEL = 3,

    /**
     * A movement has happened outside of the normal bounds of the UI element.
     * This does not provide a full gesture, but only the initial location of the movement/touch.
     */
    AMOTION_EVENT_ACTION_OUTSIDE = 4,

    /**
     * A non-primary pointer has gone down.
     * The bits in AMOTION_EVENT_ACTION_POINTER_INDEX_MASK indicate which pointer changed.
     */
    AMOTION_EVENT_ACTION_POINTER_DOWN = 5,

    /**
     * A non-primary pointer has gone up.
     * The bits in AMOTION_EVENT_ACTION_POINTER_INDEX_MASK indicate which pointer changed.
     */
    AMOTION_EVENT_ACTION_POINTER_UP = 6,

    /**
     * A change happened but the pointer is not down (unlike AMOTION_EVENT_ACTION_MOVE).
     * The motion contains the most recent point, as well as any intermediate points since
     * the last hover move event.
     */
    AMOTION_EVENT_ACTION_HOVER_MOVE = 7,

    /**
     * The motion event contains relative vertical and/or horizontal scroll offsets.
     * Use getAxisValue to retrieve the information from AMOTION_EVENT_AXIS_VSCROLL
     * and AMOTION_EVENT_AXIS_HSCROLL.
     * The pointer may or may not be down when this event is dispatched.
     * This action is always delivered to the winder under the pointer, which
     * may not be the window currently touched.
     */
    AMOTION_EVENT_ACTION_SCROLL = 8,

    /** The pointer is not down but has entered the boundaries of a window or view. */
    AMOTION_EVENT_ACTION_HOVER_ENTER = 9,

    /** The pointer is not down but has exited the boundaries of a window or view. */
    AMOTION_EVENT_ACTION_HOVER_EXIT = 10,

    /* One or more buttons have been pressed. */
    AMOTION_EVENT_ACTION_BUTTON_PRESS = 11,

    /* One or more buttons have been released. */
    AMOTION_EVENT_ACTION_BUTTON_RELEASE = 12,
};


/**
 * Constants that identify each individual axis of a motion event.
 * @anchor AMOTION_EVENT_AXIS
 */
enum {
    /**
     * Axis constant: X axis of a motion event.
     *
     * - For a touch screen, reports the absolute X screen position of the center of
     * the touch contact area.  The units are display pixels.
     * - For a touch pad, reports the absolute X surface position of the center of the touch
     * contact area. The units are device-dependent.
     * - For a mouse, reports the absolute X screen position of the mouse pointer.
     * The units are display pixels.
     * - For a trackball, reports the relative horizontal displacement of the trackball.
     * The value is normalized to a range from -1.0 (left) to 1.0 (right).
     * - For a joystick, reports the absolute X position of the joystick.
     * The value is normalized to a range from -1.0 (left) to 1.0 (right).
     */
    AMOTION_EVENT_AXIS_X = 0,
    /**
     * Axis constant: Y axis of a motion event.
     *
     * - For a touch screen, reports the absolute Y screen position of the center of
     * the touch contact area.  The units are display pixels.
     * - For a touch pad, reports the absolute Y surface position of the center of the touch
     * contact area. The units are device-dependent.
     * - For a mouse, reports the absolute Y screen position of the mouse pointer.
     * The units are display pixels.
     * - For a trackball, reports the relative vertical displacement of the trackball.
     * The value is normalized to a range from -1.0 (up) to 1.0 (down).
     * - For a joystick, reports the absolute Y position of the joystick.
     * The value is normalized to a range from -1.0 (up or far) to 1.0 (down or near).
     */
    AMOTION_EVENT_AXIS_Y = 1,
    /**
     * Axis constant: Pressure axis of a motion event.
     *
     * - For a touch screen or touch pad, reports the approximate pressure applied to the surface
     * by a finger or other tool.  The value is normalized to a range from
     * 0 (no pressure at all) to 1 (normal pressure), although values higher than 1
     * may be generated depending on the calibration of the input device.
     * - For a trackball, the value is set to 1 if the trackball button is pressed
     * or 0 otherwise.
     * - For a mouse, the value is set to 1 if the primary mouse button is pressed
     * or 0 otherwise.
     */
    AMOTION_EVENT_AXIS_PRESSURE = 2,
    /**
     * Axis constant: Size axis of a motion event.
     *
     * - For a touch screen or touch pad, reports the approximate size of the contact area in
     * relation to the maximum detectable size for the device.  The value is normalized
     * to a range from 0 (smallest detectable size) to 1 (largest detectable size),
     * although it is not a linear scale. This value is of limited use.
     * To obtain calibrated size information, see
     * {@link AMOTION_EVENT_AXIS_TOUCH_MAJOR} or {@link AMOTION_EVENT_AXIS_TOOL_MAJOR}.
     */
    AMOTION_EVENT_AXIS_SIZE = 3,
    /**
     * Axis constant: TouchMajor axis of a motion event.
     *
     * - For a touch screen, reports the length of the major axis of an ellipse that
     * represents the touch area at the point of contact.
     * The units are display pixels.
     * - For a touch pad, reports the length of the major axis of an ellipse that
     * represents the touch area at the point of contact.
     * The units are device-dependent.
     */
    AMOTION_EVENT_AXIS_TOUCH_MAJOR = 4,
    /**
     * Axis constant: TouchMinor axis of a motion event.
     *
     * - For a touch screen, reports the length of the minor axis of an ellipse that
     * represents the touch area at the point of contact.
     * The units are display pixels.
     * - For a touch pad, reports the length of the minor axis of an ellipse that
     * represents the touch area at the point of contact.
     * The units are device-dependent.
     *
     * When the touch is circular, the major and minor axis lengths will be equal to one another.
     */
    AMOTION_EVENT_AXIS_TOUCH_MINOR = 5,
    /**
     * Axis constant: ToolMajor axis of a motion event.
     *
     * - For a touch screen, reports the length of the major axis of an ellipse that
     * represents the size of the approaching finger or tool used to make contact.
     * - For a touch pad, reports the length of the major axis of an ellipse that
     * represents the size of the approaching finger or tool used to make contact.
     * The units are device-dependent.
     *
     * When the touch is circular, the major and minor axis lengths will be equal to one another.
     *
     * The tool size may be larger than the touch size since the tool may not be fully
     * in contact with the touch sensor.
     */
    AMOTION_EVENT_AXIS_TOOL_MAJOR = 6,
    /**
     * Axis constant: ToolMinor axis of a motion event.
     *
     * - For a touch screen, reports the length of the minor axis of an ellipse that
     * represents the size of the approaching finger or tool used to make contact.
     * - For a touch pad, reports the length of the minor axis of an ellipse that
     * represents the size of the approaching finger or tool used to make contact.
     * The units are device-dependent.
     *
     * When the touch is circular, the major and minor axis lengths will be equal to one another.
     *
     * The tool size may be larger than the touch size since the tool may not be fully
     * in contact with the touch sensor.
     */
    AMOTION_EVENT_AXIS_TOOL_MINOR = 7,
    /**
     * Axis constant: Orientation axis of a motion event.
     *
     * - For a touch screen or touch pad, reports the orientation of the finger
     * or tool in radians relative to the vertical plane of the device.
     * An angle of 0 radians indicates that the major axis of contact is oriented
     * upwards, is perfectly circular or is of unknown orientation.  A positive angle
     * indicates that the major axis of contact is oriented to the right.  A negative angle
     * indicates that the major axis of contact is oriented to the left.
     * The full range is from -PI/2 radians (finger pointing fully left) to PI/2 radians
     * (finger pointing fully right).
     * - For a stylus, the orientation indicates the direction in which the stylus
     * is pointing in relation to the vertical axis of the current orientation of the screen.
     * The range is from -PI radians to PI radians, where 0 is pointing up,
     * -PI/2 radians is pointing left, -PI or PI radians is pointing down, and PI/2 radians
     * is pointing right.  See also {@link AMOTION_EVENT_AXIS_TILT}.
     */
    AMOTION_EVENT_AXIS_ORIENTATION = 8,
    /**
     * Axis constant: Vertical Scroll axis of a motion event.
     *
     * - For a mouse, reports the relative movement of the vertical scroll wheel.
     * The value is normalized to a range from -1.0 (down) to 1.0 (up).
     *
     * This axis should be used to scroll views vertically.
     */
    AMOTION_EVENT_AXIS_VSCROLL = 9,
    /**
     * Axis constant: Horizontal Scroll axis of a motion event.
     *
     * - For a mouse, reports the relative movement of the horizontal scroll wheel.
     * The value is normalized to a range from -1.0 (left) to 1.0 (right).
     *
     * This axis should be used to scroll views horizontally.
     */
    AMOTION_EVENT_AXIS_HSCROLL = 10,
    /**
     * Axis constant: Z axis of a motion event.
     *
     * - For a joystick, reports the absolute Z position of the joystick.
     * The value is normalized to a range from -1.0 (high) to 1.0 (low).
     * <em>On game pads with two analog joysticks, this axis is often reinterpreted
     * to report the absolute X position of the second joystick instead.</em>
     */
    AMOTION_EVENT_AXIS_Z = 11,
    /**
     * Axis constant: X Rotation axis of a motion event.
     *
     * - For a joystick, reports the absolute rotation angle about the X axis.
     * The value is normalized to a range from -1.0 (counter-clockwise) to 1.0 (clockwise).
     */
    AMOTION_EVENT_AXIS_RX = 12,
    /**
     * Axis constant: Y Rotation axis of a motion event.
     *
     * - For a joystick, reports the absolute rotation angle about the Y axis.
     * The value is normalized to a range from -1.0 (counter-clockwise) to 1.0 (clockwise).
     */
    AMOTION_EVENT_AXIS_RY = 13,
    /**
     * Axis constant: Z Rotation axis of a motion event.
     *
     * - For a joystick, reports the absolute rotation angle about the Z axis.
     * The value is normalized to a range from -1.0 (counter-clockwise) to 1.0 (clockwise).
     * On game pads with two analog joysticks, this axis is often reinterpreted
     * to report the absolute Y position of the second joystick instead.
     */
    AMOTION_EVENT_AXIS_RZ = 14,
    /**
     * Axis constant: Hat X axis of a motion event.
     *
     * - For a joystick, reports the absolute X position of the directional hat control.
     * The value is normalized to a range from -1.0 (left) to 1.0 (right).
     */
    AMOTION_EVENT_AXIS_HAT_X = 15,
    /**
     * Axis constant: Hat Y axis of a motion event.
     *
     * - For a joystick, reports the absolute Y position of the directional hat control.
     * The value is normalized to a range from -1.0 (up) to 1.0 (down).
     */
    AMOTION_EVENT_AXIS_HAT_Y = 16,
    /**
     * Axis constant: Left Trigger axis of a motion event.
     *
     * - For a joystick, reports the absolute position of the left trigger control.
     * The value is normalized to a range from 0.0 (released) to 1.0 (fully pressed).
     */
    AMOTION_EVENT_AXIS_LTRIGGER = 17,
    /**
     * Axis constant: Right Trigger axis of a motion event.
     *
     * - For a joystick, reports the absolute position of the right trigger control.
     * The value is normalized to a range from 0.0 (released) to 1.0 (fully pressed).
     */
    AMOTION_EVENT_AXIS_RTRIGGER = 18,
    /**
     * Axis constant: Throttle axis of a motion event.
     *
     * - For a joystick, reports the absolute position of the throttle control.
     * The value is normalized to a range from 0.0 (fully open) to 1.0 (fully closed).
     */
    AMOTION_EVENT_AXIS_THROTTLE = 19,
    /**
     * Axis constant: Rudder axis of a motion event.
     *
     * - For a joystick, reports the absolute position of the rudder control.
     * The value is normalized to a range from -1.0 (turn left) to 1.0 (turn right).
     */
    AMOTION_EVENT_AXIS_RUDDER = 20,
    /**
     * Axis constant: Wheel axis of a motion event.
     *
     * - For a joystick, reports the absolute position of the steering wheel control.
     * The value is normalized to a range from -1.0 (turn left) to 1.0 (turn right).
     */
    AMOTION_EVENT_AXIS_WHEEL = 21,
    /**
     * Axis constant: Gas axis of a motion event.
     *
     * - For a joystick, reports the absolute position of the gas (accelerator) control.
     * The value is normalized to a range from 0.0 (no acceleration)
     * to 1.0 (maximum acceleration).
     */
    AMOTION_EVENT_AXIS_GAS = 22,
    /**
     * Axis constant: Brake axis of a motion event.
     *
     * - For a joystick, reports the absolute position of the brake control.
     * The value is normalized to a range from 0.0 (no braking) to 1.0 (maximum braking).
     */
    AMOTION_EVENT_AXIS_BRAKE = 23,
    /**
     * Axis constant: Distance axis of a motion event.
     *
     * - For a stylus, reports the distance of the stylus from the screen.
     * A value of 0.0 indicates direct contact and larger values indicate increasing
     * distance from the surface.
     */
    AMOTION_EVENT_AXIS_DISTANCE = 24,
    /**
     * Axis constant: Tilt axis of a motion event.
     *
     * - For a stylus, reports the tilt angle of the stylus in radians where
     * 0 radians indicates that the stylus is being held perpendicular to the
     * surface, and PI/2 radians indicates that the stylus is being held flat
     * against the surface.
     */
    AMOTION_EVENT_AXIS_TILT = 25,
    /**
     * Axis constant:  Generic scroll axis of a motion event.
     *
     * - This is used for scroll axis motion events that can't be classified as strictly
     *   vertical or horizontal. The movement of a rotating scroller is an example of this.
     */
    AMOTION_EVENT_AXIS_SCROLL = 26,
    /**
     * Axis constant: The movement of x position of a motion event.
     *
     * - For a mouse, reports a difference of x position between the previous position.
     * This is useful when pointer is captured, in that case the mouse pointer doesn't
     * change the location but this axis reports the difference which allows the app
     * to see how the mouse is moved.
     */
    AMOTION_EVENT_AXIS_RELATIVE_X = 27,
    /**
     * Axis constant: The movement of y position of a motion event.
     *
     * Same as {@link AMOTION_EVENT_AXIS_RELATIVE_X}, but for y position.
     */
    AMOTION_EVENT_AXIS_RELATIVE_Y = 28,
    /**
     * Axis constant: Generic 1 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_1 = 32,
    /**
     * Axis constant: Generic 2 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_2 = 33,
    /**
     * Axis constant: Generic 3 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_3 = 34,
    /**
     * Axis constant: Generic 4 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_4 = 35,
    /**
     * Axis constant: Generic 5 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_5 = 36,
    /**
     * Axis constant: Generic 6 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_6 = 37,
    /**
     * Axis constant: Generic 7 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_7 = 38,
    /**
     * Axis constant: Generic 8 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_8 = 39,
    /**
     * Axis constant: Generic 9 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_9 = 40,
    /**
     * Axis constant: Generic 10 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_10 = 41,
    /**
     * Axis constant: Generic 11 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_11 = 42,
    /**
     * Axis constant: Generic 12 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_12 = 43,
    /**
     * Axis constant: Generic 13 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_13 = 44,
    /**
     * Axis constant: Generic 14 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_14 = 45,
    /**
     * Axis constant: Generic 15 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_15 = 46,
    /**
     * Axis constant: Generic 16 axis of a motion event.
     * The interpretation of a generic axis is device-specific.
     */
    AMOTION_EVENT_AXIS_GENERIC_16 = 47,

    // NOTE: If you add a new axis here you must also add it to several other files.
    //       Refer to frameworks/base/core/java/android/view/MotionEvent.java for the full list.
};


/**
 * Input source masks.
 *
 * Refer to the documentation on android.view.InputDevice for more details about input sources
 * and their correct interpretation.
 */
enum {
    /** mask */
    AINPUT_SOURCE_CLASS_MASK = 0x000000ff,

    /** none */
    AINPUT_SOURCE_CLASS_NONE = 0x00000000,
    /** button */
    AINPUT_SOURCE_CLASS_BUTTON = 0x00000001,
    /** pointer */
    AINPUT_SOURCE_CLASS_POINTER = 0x00000002,
    /** navigation */
    AINPUT_SOURCE_CLASS_NAVIGATION = 0x00000004,
    /** position */
    AINPUT_SOURCE_CLASS_POSITION = 0x00000008,
    /** joystick */
    AINPUT_SOURCE_CLASS_JOYSTICK = 0x00000010,
};


/**
 * Input sources.
 */
enum {
    /** unknown */
    AINPUT_SOURCE_UNKNOWN = 0x00000000,

    /** keyboard */
    AINPUT_SOURCE_KEYBOARD = 0x00000100 | AINPUT_SOURCE_CLASS_BUTTON,
    /** dpad */
    AINPUT_SOURCE_DPAD = 0x00000200 | AINPUT_SOURCE_CLASS_BUTTON,
    /** gamepad */
    AINPUT_SOURCE_GAMEPAD = 0x00000400 | AINPUT_SOURCE_CLASS_BUTTON,
    /** touchscreen */
    AINPUT_SOURCE_TOUCHSCREEN = 0x00001000 | AINPUT_SOURCE_CLASS_POINTER,
    /** mouse */
    AINPUT_SOURCE_MOUSE = 0x00002000 | AINPUT_SOURCE_CLASS_POINTER,
    /** stylus */
    AINPUT_SOURCE_STYLUS = 0x00004000 | AINPUT_SOURCE_CLASS_POINTER,
    /** bluetooth stylus */
    AINPUT_SOURCE_BLUETOOTH_STYLUS = 0x00008000 | AINPUT_SOURCE_STYLUS,
    /** trackball */
    AINPUT_SOURCE_TRACKBALL = 0x00010000 | AINPUT_SOURCE_CLASS_NAVIGATION,
    /** mouse relative */
    AINPUT_SOURCE_MOUSE_RELATIVE = 0x00020000 | AINPUT_SOURCE_CLASS_NAVIGATION,
    /** touchpad */
    AINPUT_SOURCE_TOUCHPAD = 0x00100000 | AINPUT_SOURCE_CLASS_POSITION,
    /** navigation */
    AINPUT_SOURCE_TOUCH_NAVIGATION = 0x00200000 | AINPUT_SOURCE_CLASS_NONE,
    /** joystick */
    AINPUT_SOURCE_JOYSTICK = 0x01000000 | AINPUT_SOURCE_CLASS_JOYSTICK,
    /** HDMI */
    AINPUT_SOURCE_HDMI = 0x02000000 | AINPUT_SOURCE_CLASS_BUTTON,
    /** sensor */
    AINPUT_SOURCE_SENSOR = 0x04000000 | AINPUT_SOURCE_CLASS_NONE,
    /** rotary encoder */
    AINPUT_SOURCE_ROTARY_ENCODER = 0x00400000 | AINPUT_SOURCE_CLASS_NONE,

    /** any */
    AINPUT_SOURCE_ANY = 0xffffff00,
};


struct AInputEvent;
/**
 * Input events.
 *
 * Input events are opaque structures.  Use the provided accessors functions to
 * read their properties.
 */
typedef struct AInputEvent AInputEvent;

/**
 * [Non-Standard]: Real undrelying structure for AInputEvent, can be used for creating AInputEvent
 */
typedef struct inputEvent {
    int source; // one of AINPUT_SOURCE_* enum
    int type; // one of AINPUT_EVENT_TYPE_* enum
    int deviceId; // device ID (0-3 for controllers)

    //key event only
    int action; // one of AKEY_EVENT_ACTION_* enum
    int keycode; // one of AKEYCODE_* enum
    int scancode; // device-specific
    int repeatcount;

    //motion event only. up to 10 pointers [per source]
    int motion_action;
    int motion_ptrcount;
    int motion_ptridx[10];
    float motion_x[10];
    float motion_y[10];
    float motion_z[10];
    float motion_rz[10];
    float motion_hat_x[10];
    float motion_hat_y[10];
    float motion_lt[10];
    float motion_rt[10];
} inputEvent;

/**
 * [Non-Standard]: Create new AInputEvent object
 */
AInputEvent *AInputEvent_create(const inputEvent *e);

/** Get the input event type. */
int32_t AInputEvent_getType(const AInputEvent* event);

/** Get the input event source. */
int32_t AInputEvent_getSource(const AInputEvent *event);

/** Get the device id associated with the input event. */
int32_t AInputEvent_getDeviceId(const AInputEvent* event);

/** Get the key event action. */
int32_t AKeyEvent_getAction(const AInputEvent *key_event);

/**
 * Get the key code of the key event.
 * This is the physical key that was pressed, not the Unicode character.
 */
int32_t AKeyEvent_getKeyCode(const AInputEvent *key_event);

/**
 * Get the repeat count of the event.
 * For both key up an key down events, this is the number of times the key has
 * repeated with the first down starting at 0 and counting up from there.  For
 * multiple key events, this is the number of down/up pairs that have occurred.
 */
int32_t AKeyEvent_getRepeatCount(const AInputEvent* key_event);

/**
 * Get the hardware key id of this key event.
 * These values are not reliable and vary from device to device.
 */
int32_t AKeyEvent_getScanCode(const AInputEvent* key_event);

/** Get the combined motion event action code and pointer index. */
int32_t AMotionEvent_getAction(const AInputEvent *motion_event);

/**
 * Get the number of pointers of data contained in this event.
 * Always >= 1.
 */
size_t AMotionEvent_getPointerCount(const AInputEvent *motion_event);

/**
 * Get the pointer identifier associated with a particular pointer
 * data index in this event.  The identifier tells you the actual pointer
 * number associated with the data, accounting for individual pointers
 * going up and down since the start of the current gesture.
 */
int32_t AMotionEvent_getPointerId(const AInputEvent *motion_event, size_t pointer_index);

/**
 * Get the current X coordinate of this event for the given pointer index.
 * Whole numbers are pixels; the value may have a fraction for input devices
 * that are sub-pixel precise.
 */
float AMotionEvent_getX(const AInputEvent *motion_event, size_t pointer_index);

/**
 * Get the current Y coordinate of this event for the given pointer index.
 * Whole numbers are pixels; the value may have a fraction for input devices
 * that are sub-pixel precise.
 */
float AMotionEvent_getY(const AInputEvent *motion_event, size_t pointer_index);

/** Get the value of the request axis for the given pointer index. */
float AMotionEvent_getAxisValue(const AInputEvent* motion_event,
                                int32_t axis, size_t pointer_index);

/**
 * Get the historical value of the request axis for the given pointer index
 * that occurred between this event and the previous motion event.
 */
float AMotionEvent_getHistoricalAxisValue(const AInputEvent* motion_event,
                                          int32_t axis, size_t pointer_index, size_t history_index);

struct AInputQueue;
/**
 * Input queue
 *
 * An input queue is the facility through which you retrieve input
 * events.
 */
typedef struct AInputQueue AInputQueue;

/**
 * [Non-Standard]: Create new AInputQueue object
 */
AInputQueue *AInputQueue_create();

/**
 * [Non-Standard]: Enqueue new input event
 */
void AInputQueue_enqueueEvent(AInputQueue *queue, AInputEvent *event);

/**
 * Add this input queue to a looper for processing.  See
 * ALooper_addFd() for information on the ident, callback, and data params.
 */
void AInputQueue_attachLooper(AInputQueue *queue, ALooper *looper,
                              int ident, ALooper_callbackFunc callback, void *data);

/**
 * Remove the input queue from the looper it is currently attached to.
 */
void AInputQueue_detachLooper(AInputQueue *queue);

/**
 * Returns the next available event from the queue.  Returns a negative
 * value if no events are available or an error has occurred.
 */
int32_t AInputQueue_getEvent(AInputQueue *queue, AInputEvent **outEvent);

/**
 * Sends the key for standard pre-dispatching -- that is, possibly deliver
 * it to the current IME to be consumed before the app.  Returns 0 if it
 * was not pre-dispatched, meaning you can process it right now.  If non-zero
 * is returned, you must abandon the current event processing and allow the
 * event to appear again in the event queue (if it does not get consumed during
 * pre-dispatching).
 */
int32_t AInputQueue_preDispatchEvent(AInputQueue *queue, AInputEvent *event);

/**
 * Report that dispatching has finished with the given event.
 * This must be called after receiving an event with AInputQueue_get_event().
 */
void AInputQueue_finishEvent(AInputQueue *queue, AInputEvent *event, int handled);

#ifdef __cplusplus
}
#endif

#endif // _ANDROID_INPUT_H
