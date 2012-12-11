/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef XKeyMappingTable_h
#define XKeyMappingTable_h

#include "NixEvents.h"
#include <X11/keysym.h>

static const unsigned XKeySymMappingTable[] = {

    // misc keys

    XK_Escape,                  kNIXKeyEventKey_Escape,
    XK_Tab,                     kNIXKeyEventKey_Tab,
    XK_ISO_Left_Tab,            kNIXKeyEventKey_Backtab,
    XK_BackSpace,               kNIXKeyEventKey_Backspace,
    XK_Return,                  kNIXKeyEventKey_Return,
    XK_Insert,                  kNIXKeyEventKey_Insert,
    XK_Delete,                  kNIXKeyEventKey_Delete,
    XK_Clear,                   kNIXKeyEventKey_Delete,
    XK_Pause,                   kNIXKeyEventKey_Pause,
    XK_Print,                   kNIXKeyEventKey_Print,
    0x1005FF60,                 kNIXKeyEventKey_SysReq,         // hardcoded Sun SysReq
    0x1007ff00,                 kNIXKeyEventKey_SysReq,         // hardcoded X386 SysReq

    // cursor movement

    XK_Home,                    kNIXKeyEventKey_Home,
    XK_End,                     kNIXKeyEventKey_End,
    XK_Left,                    kNIXKeyEventKey_Left,
    XK_Up,                      kNIXKeyEventKey_Up,
    XK_Right,                   kNIXKeyEventKey_Right,
    XK_Down,                    kNIXKeyEventKey_Down,
    XK_Prior,                   kNIXKeyEventKey_PageUp,
    XK_Next,                    kNIXKeyEventKey_PageDown,

    // modifiers

    XK_Shift_L,                 kNIXKeyEventKey_Shift,
    XK_Shift_R,                 kNIXKeyEventKey_Shift,
    XK_Shift_Lock,              kNIXKeyEventKey_Shift,
    XK_Control_L,               kNIXKeyEventKey_Control,
    XK_Control_R,               kNIXKeyEventKey_Control,
    XK_Meta_L,                  kNIXKeyEventKey_Meta,
    XK_Meta_R,                  kNIXKeyEventKey_Meta,
    XK_Alt_L,                   kNIXKeyEventKey_Alt,
    XK_Alt_R,                   kNIXKeyEventKey_Alt,
    XK_Caps_Lock,               kNIXKeyEventKey_CapsLock,
    XK_Num_Lock,                kNIXKeyEventKey_NumLock,
    XK_Scroll_Lock,             kNIXKeyEventKey_ScrollLock,
    XK_Super_L,                 kNIXKeyEventKey_Super_L,
    XK_Super_R,                 kNIXKeyEventKey_Super_R,
    XK_Menu,                    kNIXKeyEventKey_Menu,
    XK_Hyper_L,                 kNIXKeyEventKey_Hyper_L,
    XK_Hyper_R,                 kNIXKeyEventKey_Hyper_R,
    XK_Help,                    kNIXKeyEventKey_Help,
    0x1000FF74,                 kNIXKeyEventKey_Backtab,        // hardcoded HP backtab
    0x1005FF10,                 kNIXKeyEventKey_F11,            // hardcoded Sun F36 (labeled F11)
    0x1005FF11,                 kNIXKeyEventKey_F12,            // hardcoded Sun F37 (labeled F12)

    // numeric and function keypad keys

    XK_KP_Space,                kNIXKeyEventKey_Space,
    XK_KP_Tab,                  kNIXKeyEventKey_Tab,
    XK_KP_Enter,                kNIXKeyEventKey_Enter,
    XK_KP_Home,                 kNIXKeyEventKey_Home,
    XK_KP_Left,                 kNIXKeyEventKey_Left,
    XK_KP_Up,                   kNIXKeyEventKey_Up,
    XK_KP_Right,                kNIXKeyEventKey_Right,
    XK_KP_Down,                 kNIXKeyEventKey_Down,
    XK_KP_Prior,                kNIXKeyEventKey_PageUp,
    XK_KP_Next,                 kNIXKeyEventKey_PageDown,
    XK_KP_End,                  kNIXKeyEventKey_End,
    XK_KP_Begin,                kNIXKeyEventKey_Clear,
    XK_KP_Insert,               kNIXKeyEventKey_Insert,
    XK_KP_Delete,               kNIXKeyEventKey_Delete,
    XK_KP_Equal,                kNIXKeyEventKey_Equal,
    XK_KP_Multiply,             kNIXKeyEventKey_Asterisk,
    XK_KP_Add,                  kNIXKeyEventKey_Plus,
    XK_KP_Separator,            kNIXKeyEventKey_Comma,
    XK_KP_Subtract,             kNIXKeyEventKey_Minus,
    XK_KP_Decimal,              kNIXKeyEventKey_Period,
    XK_KP_Divide,               kNIXKeyEventKey_Slash,

    // International input method support keys

    // International & multi-key character composition
    XK_ISO_Level3_Shift,        kNIXKeyEventKey_AltGr,
    XK_Multi_key,               kNIXKeyEventKey_Multi_key,
    XK_Codeinput,               kNIXKeyEventKey_Codeinput,
    XK_SingleCandidate,         kNIXKeyEventKey_SingleCandidate,
    XK_MultipleCandidate,       kNIXKeyEventKey_MultipleCandidate,
    XK_PreviousCandidate,       kNIXKeyEventKey_PreviousCandidate,

    // Misc Functions
    XK_Mode_switch,             kNIXKeyEventKey_Mode_switch,
    XK_script_switch,           kNIXKeyEventKey_Mode_switch,

    // dead keys
    XK_dead_grave,              kNIXKeyEventKey_Dead_Grave,
    XK_dead_acute,              kNIXKeyEventKey_Dead_Acute,
    XK_dead_circumflex,         kNIXKeyEventKey_Dead_Circumflex,
    XK_dead_tilde,              kNIXKeyEventKey_Dead_Tilde,
    XK_dead_macron,             kNIXKeyEventKey_Dead_Macron,
    XK_dead_breve,              kNIXKeyEventKey_Dead_Breve,
    XK_dead_abovedot,           kNIXKeyEventKey_Dead_Abovedot,
    XK_dead_diaeresis,          kNIXKeyEventKey_Dead_Diaeresis,
    XK_dead_abovering,          kNIXKeyEventKey_Dead_Abovering,
    XK_dead_doubleacute,        kNIXKeyEventKey_Dead_Doubleacute,
    XK_dead_caron,              kNIXKeyEventKey_Dead_Caron,
    XK_dead_cedilla,            kNIXKeyEventKey_Dead_Cedilla,
    XK_dead_ogonek,             kNIXKeyEventKey_Dead_Ogonek,
    XK_dead_iota,               kNIXKeyEventKey_Dead_Iota,
    XK_dead_voiced_sound,       kNIXKeyEventKey_Dead_Voiced_Sound,
    XK_dead_semivoiced_sound,   kNIXKeyEventKey_Dead_Semivoiced_Sound,
    XK_dead_belowdot,           kNIXKeyEventKey_Dead_Belowdot,
    XK_dead_hook,               kNIXKeyEventKey_Dead_Hook,
    XK_dead_horn,               kNIXKeyEventKey_Dead_Horn,

    0,                          0
};

#endif
