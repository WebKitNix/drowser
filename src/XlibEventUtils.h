#ifndef XlibEventUtils_h
#define XlibEventUtils_h

#include "XKeyMappingTable.h"
#include <NixEvents.h>
#include <X11/keysym.h>
#include <ctype.h>
#include <glib.h>

static Nix::KeyEvent::Key convertXKeySymToNativeKeycode(unsigned int keysym)
{
    using namespace Nix;

    for (int i = 0; XKeySymMappingTable[i]; i += 2) {
        if (XKeySymMappingTable[i] == keysym)
            return static_cast<KeyEvent::Key>(XKeySymMappingTable[i+1]);
    }

    if (keysym < 256) {
        // upper-case key, if known
        if (isprint((int)keysym))
            return static_cast<KeyEvent::Key>(toupper((int)keysym));
    } else if (keysym >= XK_F1 && keysym <= XK_F35) {
        // function keys
        return static_cast<KeyEvent::Key>(KeyEvent::Key_F1 + ((int)keysym - XK_F1));
    } else if (keysym >= XK_KP_Space && keysym <= XK_KP_9) {
        if (keysym >= XK_KP_0) {
            // numeric keypad keys
            return static_cast<KeyEvent::Key>(KeyEvent::Key_0 + ((int)keysym - XK_KP_0));
        }
    }
    return KeyEvent::Key_unknown;
}

static int convertXEventModifiersToNativeModifiers(int s)
{
    using namespace Nix;

    int ret = 0;
    if (s & ShiftMask)
        ret |= InputEvent::ShiftKey;
    if (s & ControlMask)
        ret |= InputEvent::ControlKey;
    if (s & LockMask)
        ret |= InputEvent::CapsLockKey;
    if (s & Mod1Mask)
        ret |= InputEvent::AltKey;
    if (s & Mod2Mask)
        ret |= InputEvent::NumLockKey;
    if (s & Mod4Mask)
        ret |= InputEvent::MetaKey;
    return ret;
}

static Nix::MouseEvent::Button convertXEventButtonToNativeMouseButton(unsigned int mouseButton)
{
    using namespace Nix;

    switch (mouseButton) {
    case Button1:
        return MouseEvent::LeftButton;
    case Button2:
        return MouseEvent::MiddleButton;
    case Button3:
        return MouseEvent::RightButton;
    default:
        return MouseEvent::NoButton;
    }
}

static inline double convertXEventTimeToNixTimestamp(double time)
{
    return time / 1000.0;
}

#endif
