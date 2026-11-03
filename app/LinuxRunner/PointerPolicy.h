// External pointer arbitration for the Linux desktop surface.
//
// iOS can report one physical mouse through several public channels, depending
// on device and settings: UIKit touches (AssistiveTouch synthesizes direct touches
// for its pointer on iPhone; iPad pointers produce indirect-pointer touches and
// hover), UIKit scroll events, and GameController's GCMouse (relative motion,
// buttons, wheel). Positions from UIKit are absolute and match the pointer the
// user sees; GCMouse motion is relative and has no on-screen pointer of its own.
// These rules choose exactly one source per signal so a click is never sent twice
// and the Linux pointer never drifts away from the visible iOS pointer.
#ifndef LINUXRUNNER_POINTER_POLICY_H
#define LINUXRUNNER_POINTER_POLICY_H
#include <stdbool.h>

enum { LR_MOUSE_MODE_AUTO = 0, LR_MOUSE_MODE_TOUCH = 1, LR_MOUSE_MODE_POINTER = 2 };
enum { LR_POINTER_LEFT = 1, LR_POINTER_MIDDLE = 2, LR_POINTER_RIGHT = 3 };

typedef struct {
    int mode;                 // LR_MOUSE_MODE_*
    bool assistive_touch;     // UIAccessibilityIsAssistiveTouchRunning()
    bool gc_mouse;            // a GCMouse is connected
    bool indirect_pointer;    // an indirect-pointer touch has been seen (iPad-style pointer)
    bool hover;               // UIKit hover events have been seen
} LRPointerEnvironment;

// Pointer mode: one touch is the mouse (press on touch-down, move, release on
// touch-up, at its exact position; no tap or long-press delays).
static inline bool lr_pointer_mode(LRPointerEnvironment e) {
    if (e.mode == LR_MOUSE_MODE_POINTER) return true;
    if (e.mode == LR_MOUSE_MODE_TOUCH) return false;
    return e.gc_mouse || e.assistive_touch || e.indirect_pointer;
}

// GCMouse relative motion moves the Linux pointer only when iOS shows no pointer
// whose absolute position UIKit reports (otherwise the two would drift apart).
static inline bool lr_pointer_use_gc_motion(LRPointerEnvironment e) {
    return e.gc_mouse && !e.assistive_touch && !e.indirect_pointer && !e.hover;
}

// Use GCMouse for physical buttons whenever available, independently of the
// absolute UIKit position source. Detecting an indirect pointer between a
// button's press and release must not discard the release and leave it held.
// AssistiveTouch supplies its left click as a direct touch instead.
static inline bool lr_pointer_use_gc_button(LRPointerEnvironment e, int button) {
    if (button == LR_POINTER_LEFT && e.assistive_touch) return false;
    return e.gc_mouse;
}

// UIKit remains the button source for fingers/Pencil and pointers that have no
// GCMouse button stream. One physical click has one owner even when both APIs
// report it; UIKit still supplies the absolute position in either case.
static inline bool lr_pointer_use_uikit_button(LRPointerEnvironment e, bool indirect, int button) {
    return !indirect || !lr_pointer_use_gc_button(e, button);
}

// X11 button bit for a UIKit touch in pointer mode: secondary button mask maps
// to the right button (bit 4), otherwise left (bit 1).
static inline unsigned lr_pointer_touch_button(bool secondary_mask) { return secondary_mask ? 4u : 1u; }

// Wheel: UIKit scroll events are ignored while GCMouse reports the same wheel.
static inline bool lr_pointer_use_uikit_wheel(double now, double last_gc_scroll) {
    return now - last_gc_scroll > 0.25;
}
#endif
