/*
 * Copyright (C) 2012-2013 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XlibEventUtils_h
#define XlibEventUtils_h

#include "XKeyMappingTable.h"
#include <NIXEvents.h>
#include <X11/keysym.h>
#include <ctype.h>
#include <glib.h>

static NIXKeyEventKey convertXKeySymToNativeKeycode(unsigned int keysym)
{
    for (int i = 0; XKeySymMappingTable[i]; i += 2) {
        if (XKeySymMappingTable[i] == keysym)
            return static_cast<NIXKeyEventKey>(XKeySymMappingTable[i+1]);
    }

    if (keysym < 256) {
        // upper-case key, if known
        if (isprint((int)keysym))
            return static_cast<NIXKeyEventKey>(toupper((int)keysym));
    } else if (keysym >= XK_F1 && keysym <= XK_F35) {
        // function keys
        return static_cast<NIXKeyEventKey>(kNIXKeyEventKey_F1 + ((int)keysym - XK_F1));
    } else if (keysym >= XK_KP_Space && keysym <= XK_KP_9) {
        if (keysym >= XK_KP_0) {
            // numeric keypad keys
            return static_cast<NIXKeyEventKey>(kNIXKeyEventKey_0 + ((int)keysym - XK_KP_0));
        }
    }
    return kNIXKeyEventKey_unknown;
}

static uint32_t convertXEventModifiersToNativeModifiers(int s)
{
    int ret = 0;
    if (s & ShiftMask)
        ret |= kNIXInputEventModifiersShiftKey;
    if (s & ControlMask)
        ret |= kNIXInputEventModifiersControlKey;
    if (s & LockMask)
        ret |= kNIXInputEventModifiersCapsLockKey;
    if (s & Mod1Mask)
        ret |= kNIXInputEventModifiersAltKey;
    if (s & Mod2Mask)
        ret |= kNIXInputEventModifiersNumLockKey;
    if (s & Mod4Mask)
        ret |= kNIXInputEventModifiersMetaKey;
    return ret;
}

static WKEventMouseButton convertXEventButtonToNativeMouseButton(unsigned int mouseButton)
{
    switch (mouseButton) {
    case Button1:
        return kWKEventMouseButtonLeftButton;
    case Button2:
        return kWKEventMouseButtonMiddleButton;
    case Button3:
        return kWKEventMouseButtonRightButton;
    default:
        return kWKEventMouseButtonNoButton;
    }
}

static inline double convertXEventTimeToNixTimestamp(double time)
{
    return time / 1000.0;
}

#endif
