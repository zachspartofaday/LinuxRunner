#ifndef LR_KEYBOARD_POLICY_H
#define LR_KEYBOARD_POLICY_H
#import <Foundation/Foundation.h>
#include <stdint.h>
#include <string.h>

enum { LR_KEY_SHIFT = 1, LR_KEY_CONTROL = 2, LR_KEY_ALT = 4, LR_KEY_COMMAND = 8 };

static inline uint32_t LRSingleKeyCharacter(NSString *text) {
    NSData *data = [text dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
    if (data.length != 4) return 0;
    uint32_t scalar; memcpy(&scalar, data.bytes, sizeof scalar);
    return scalar >= 32 && scalar != 127 && scalar <= 0x10ffff &&
        !(scalar >= 0xd800 && scalar <= 0xdfff) ? scalar : 0;
}

static inline unsigned LRModifierForHID(unsigned hid) {
    // USB keyboard usages E0..E7: left and right Control, Shift, Alt, GUI.
    if (hid < 0xe0 || hid > 0xe7) return 0;
    const unsigned flags[] = {LR_KEY_CONTROL, LR_KEY_SHIFT, LR_KEY_ALT, LR_KEY_COMMAND};
    return flags[(hid - 0xe0) % 4];
}

static inline uint32_t LRHardwareKeySym(NSString *characters, NSString *unmodified, unsigned modifiers) {
    uint32_t scalar = 0;
    // RFB carries the intended symbol, not a physical key. Sending '1' with
    // Shift held makes x11vnc temporarily undo Shift to generate exactly '1'.
    // UIKit's layout-aware characters supplies '!' (and Caps Lock's case).
    if (!(modifiers & (LR_KEY_ALT | LR_KEY_COMMAND)))
        scalar = LRSingleKeyCharacter(characters);
    if (!scalar || ((modifiers & LR_KEY_CONTROL) && !(modifiers & LR_KEY_SHIFT))) {
        NSString *base = unmodified;
        if (modifiers & LR_KEY_SHIFT) base = unmodified.uppercaseString;
        scalar = LRSingleKeyCharacter(base);
        // Control can turn shifted ASCII punctuation into a control code.
        // Recover that shortcut's Shift level when UIKit supplies no text.
        if ((modifiers & LR_KEY_SHIFT) && scalar) {
            const char *plain = "1234567890-=[]\\;',./`";
            const char *shift = "!@#$%^&*()_+{}|:\"<>?~";
            const char *found = scalar < 128 ? strchr(plain, (int) scalar) : NULL;
            if (found) scalar = (unsigned char) shift[found - plain];
        }
    }
    return scalar < 256 ? scalar : 0x01000000U | scalar;
}
#endif
