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

#include "DesktopWindow.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <GL/glx.h>

#include "FatalError.h"
#include "XlibEventSource.h"
#include "XlibEventUtils.h"

#include <stdio.h>
#include <string.h> // for memset

static Atom wmDeleteMessageAtom;
static const double DOUBLE_CLICK_INTERVAL = 300;

class ScopedXFree
{
public:
    ScopedXFree(void* ptr) : m_ptr(ptr) {}
    ScopedXFree(const ScopedXFree&) = delete;
    ~ScopedXFree() { XFree(m_ptr); }
private:
    void* m_ptr;
};

class DesktopWindowLinux : public DesktopWindow, public XlibEventSource::Client {
public:
    DesktopWindowLinux(DesktopWindowClient* client, int width, int height);
    ~DesktopWindowLinux();
    void makeCurrent();
    void swapBuffers();
    void setMouseCursor(MouseCursor cursor);
private:
    void setup();
    void destroyGLContext();
    void updateSizeIfNeeded(int width, int height);

    void handleXEvent(const XEvent&);
    void updateClickCount(const XButtonPressedEvent* event);

    XVisualInfo* m_visualInfo;
    GLXContext m_context;
    XlibEventSource* m_eventSource;
    Display* m_display;
    Window m_window;
    Cursor m_cursor;
    unsigned int m_currentX11Cursor;

    double m_lastClickTime;
    int m_lastClickX;
    int m_lastClickY;
    WKEventMouseButton m_lastClickButton;
    int m_clickCount;
};

DesktopWindow* DesktopWindow::create(DesktopWindowClient* client, int width, int height)
{
    return new DesktopWindowLinux(client, width, height);
}

DesktopWindowLinux::DesktopWindowLinux(DesktopWindowClient* client, int width, int height)
    : DesktopWindow(client, width, height)
    , m_eventSource(0)
    , m_display(0)
    , m_cursor(0)
    , m_currentX11Cursor(XC_left_ptr)
    , m_lastClickTime(0)
    , m_lastClickX(0)
    , m_lastClickY(0)
    , m_lastClickButton(kWKEventMouseButtonNoButton)
    , m_clickCount(0)
{
    setup();

    m_eventSource = new XlibEventSource(m_display, this);

    makeCurrent();
    glEnable(GL_DEPTH_TEST);
}

DesktopWindowLinux::~DesktopWindowLinux()
{
    delete m_eventSource;
    destroyGLContext();
    XDestroyWindow(m_display, m_window);
    if (m_cursor)
        XFreeCursor(m_display, m_cursor);
    XCloseDisplay(m_display);
}

void DesktopWindowLinux::makeCurrent()
{
    glXMakeCurrent(m_display, m_window, m_context);
}

void DesktopWindowLinux::swapBuffers()
{
    glXSwapBuffers(m_display, m_window);
}

void DesktopWindowLinux::setup()
{
    m_display = XOpenDisplay(0);
    if (!m_display)
        throw FatalError("Couldn't connect to X server");

    int attributes[] = {
                GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
                GLX_DOUBLEBUFFER,  True,
                GLX_RENDER_TYPE,
                GLX_RGBA_BIT,
                GLX_RED_SIZE,      1,
                GLX_GREEN_SIZE,    1,
                GLX_BLUE_SIZE,     1,
                GLX_ALPHA_SIZE,    1,
                GLX_TRANSPARENT_TYPE,
                GLX_NONE,
                None
    };

    int numReturned = 0;
    GLXFBConfig* fbConfigs(glXChooseFBConfig(m_display, DefaultScreen(m_display), attributes, &numReturned));
    if (!fbConfigs)
        throw FatalError("No double buffered config available");

    GLXFBConfig fbConfig = fbConfigs[0];
    ScopedXFree x(fbConfigs);

    m_visualInfo = glXGetVisualFromFBConfig(m_display, fbConfig);
    if (!m_visualInfo)
        throw FatalError("No appropriate visual found.");

    XSetWindowAttributes setAttributes;
    setAttributes.colormap = XCreateColormap(m_display, DefaultRootWindow(m_display), m_visualInfo->visual, AllocNone);
    setAttributes.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | PointerMotionMask;
    m_window = XCreateWindow(m_display, DefaultRootWindow(m_display),
                                0, 0, m_size.width, m_size.height, 0,
                                m_visualInfo->depth, InputOutput, m_visualInfo->visual,
                                CWColormap | CWEventMask, &setAttributes);

    wmDeleteMessageAtom = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(m_display, m_window, &wmDeleteMessageAtom, 1);

    XMapWindow(m_display, m_window);
    XStoreName(m_display, m_window, "Drowser");

    m_context = glXCreateNewContext(m_display, fbConfig, GLX_RGBA_TYPE, NULL, GL_TRUE);
    if (!m_context)
        throw FatalError("glXCreateContext() failed.");
}

void DesktopWindowLinux::destroyGLContext()
{
    glXMakeCurrent(m_display, None, 0);
    glXDestroyContext(m_display, m_context);
    XFree(m_visualInfo);
}

static inline bool isKeypadKeysym(const KeySym symbol)
{
    // Following keypad symbols are specified on Xlib Programming Manual (section: Keyboard Encoding).
    return symbol >= 0xFF80 && symbol <= 0xFFBD;
}

static KeySym chooseSymbolForXKeyEvent(const XKeyEvent* event, bool* useUpperCase)
{
    KeySym firstSymbol = XLookupKeysym(const_cast<XKeyEvent*>(event), 0);
    KeySym secondSymbol = XLookupKeysym(const_cast<XKeyEvent*>(event), 1);
    KeySym lowerCaseSymbol, upperCaseSymbol, chosenSymbol;
    XConvertCase(firstSymbol, &lowerCaseSymbol, &upperCaseSymbol);
    bool numLockModifier = event->state & Mod2Mask;
    bool capsLockModifier = event->state & LockMask;
    bool shiftModifier = event->state & ShiftMask;
    if (numLockModifier && isKeypadKeysym(secondSymbol)) {
        chosenSymbol = shiftModifier ? firstSymbol : secondSymbol;
    } else if (lowerCaseSymbol == upperCaseSymbol) {
        chosenSymbol = shiftModifier ? secondSymbol : firstSymbol;
    } else if (shiftModifier == capsLockModifier)
        chosenSymbol = firstSymbol;
    else
        chosenSymbol = secondSymbol;

    *useUpperCase = (lowerCaseSymbol != upperCaseSymbol && chosenSymbol == upperCaseSymbol);
    XConvertCase(chosenSymbol, &lowerCaseSymbol, &upperCaseSymbol);
    return upperCaseSymbol;
}

static NIXKeyEvent convertXKeyEventToNixKeyEvent(const XKeyEvent* event, const KeySym& symbol, bool useUpperCase)
{
    NIXKeyEvent ev;
    memset(&ev, 0, sizeof(NIXKeyEvent));
    ev.type = (event->type == KeyPress) ? kNIXInputEventTypeKeyDown : kNIXInputEventTypeKeyUp;
    ev.modifiers = convertXEventModifiersToNativeModifiers(event->state);
    ev.timestamp = convertXEventTimeToNixTimestamp(event->time);
    ev.shouldUseUpperCase = useUpperCase;
    ev.isKeypad = isKeypadKeysym(symbol);
    ev.key = convertXKeySymToNativeKeycode(symbol);
    return ev;
}

static void XEventToNix(const XEvent& event, NIXKeyEvent* nixEvent)
{
    bool shouldUseUpperCase;
    const XKeyEvent* keyEvent = reinterpret_cast<const XKeyEvent*>(&event);
    KeySym symbol = chooseSymbolForXKeyEvent(keyEvent, &shouldUseUpperCase);
    *nixEvent = convertXKeyEventToNixKeyEvent(keyEvent, symbol, shouldUseUpperCase);
}

void DesktopWindowLinux::handleXEvent(const XEvent& event)
{
    if (event.type == ConfigureNotify) {
        updateSizeIfNeeded(event.xconfigure.width, event.xconfigure.height);
        return;
    }

    if (!m_client)
        return;

    switch (event.type) {
    case Expose:
        m_client->onWindowExpose();
        break;
    case KeyPress: {
        NIXKeyEvent ev;
        memset(&ev, 0, sizeof(NIXKeyEvent));
        XEventToNix(event, &ev);
        m_client->onKeyPress(&ev);
        break;
    }
    case KeyRelease: {
        NIXKeyEvent ev;
        memset(&ev, 0, sizeof(NIXKeyEvent));
        XEventToNix(event, &ev);
        m_client->onKeyRelease(&ev);
        break;
    }
    case ButtonPress: {
        const XButtonPressedEvent* xEvent = reinterpret_cast<const XButtonReleasedEvent*>(&event);

        if (xEvent->button == 4 || xEvent->button == 5) {
            // Same constant we use inside WebView to calculate the ticks. See also WebCore::Scrollbar::pixelsPerLineStep().
            const float pixelsPerStep = 40.0f;

            NIXWheelEvent ev;
            ev.type = kNIXInputEventTypeWheel;
            ev.modifiers = convertXEventModifiersToNativeModifiers(xEvent->state);
            ev.timestamp = convertXEventTimeToNixTimestamp(xEvent->time);
            ev.x = xEvent->x;
            ev.y = xEvent->y;
            ev.globalX = xEvent->x_root;
            ev.globalY = xEvent->y_root;
            ev.delta = pixelsPerStep * (xEvent->button == 4 ? 1 : -1);
            ev.orientation = xEvent->state & Mod1Mask ? kNIXWheelEventOrientationHorizontal : kNIXWheelEventOrientationVertical;
            m_client->onMouseWheel(&ev);
            break;
        }
        updateClickCount(xEvent);

        NIXMouseEvent ev;
        ev.type = kNIXInputEventTypeMouseDown;
        ev.button = convertXEventButtonToNativeMouseButton(xEvent->button);
        ev.x = xEvent->x;
        ev.y = xEvent->y;
        ev.globalX = xEvent->x_root;
        ev.globalY = xEvent->y_root;
        ev.clickCount = m_clickCount;
        ev.modifiers = convertXEventModifiersToNativeModifiers(xEvent->state);
        ev.timestamp = convertXEventTimeToNixTimestamp(xEvent->time);
        m_client->onMousePress(&ev);
        break;
    }
    case ButtonRelease: {
        const XButtonReleasedEvent* xEvent = reinterpret_cast<const XButtonReleasedEvent*>(&event);
        if (xEvent->button == 4 || xEvent->button == 5)
            break;

        NIXMouseEvent ev;
        ev.type = kNIXInputEventTypeMouseUp;
        ev.button = convertXEventButtonToNativeMouseButton(xEvent->button);
        ev.x = xEvent->x;
        ev.y = xEvent->y;
        ev.globalX = xEvent->x_root;
        ev.globalY = xEvent->y_root;
        ev.clickCount = 0;
        ev.modifiers = convertXEventModifiersToNativeModifiers(xEvent->state);
        ev.timestamp = convertXEventModifiersToNativeModifiers(xEvent->state);

        m_client->onMouseRelease(&ev);
        break;
    }
    case ClientMessage:
        if ((Atom)event.xclient.data.l[0] == wmDeleteMessageAtom)
            m_client->onWindowClose();
        break;
    case MotionNotify: {
        NIXMouseEvent ev;
        const XPointerMovedEvent* xEvent = reinterpret_cast<const XPointerMovedEvent*>(&event);
        ev.type = kNIXInputEventTypeMouseMove;
        ev.button = kWKEventMouseButtonNoButton;
        ev.x = xEvent->x;
        ev.y = xEvent->y;
        ev.globalX = xEvent->x_root;
        ev.globalY = xEvent->y_root;
        ev.clickCount = 0;
        ev.modifiers = convertXEventModifiersToNativeModifiers(xEvent->state);
        ev.timestamp = convertXEventTimeToNixTimestamp(xEvent->time);
        m_client->onMouseMove(&ev);
        break;
    }
    }
}

void DesktopWindowLinux::updateClickCount(const XButtonPressedEvent* event)
{
    if (m_lastClickX != event->x
        || m_lastClickY != event->y
        || m_lastClickButton != convertXEventButtonToNativeMouseButton(event->button)
        || event->time - m_lastClickTime >= DOUBLE_CLICK_INTERVAL)
        m_clickCount = 1;
    else
        ++m_clickCount;

    m_lastClickX = event->x;
    m_lastClickY = event->y;
    m_lastClickButton = convertXEventButtonToNativeMouseButton(event->button);
    m_lastClickTime = event->time;
}

void DesktopWindowLinux::updateSizeIfNeeded(int width, int height)
{
    if (width == m_size.width && height == m_size.height)
        return;

    m_size = WKSizeMake(width, height);

    if (m_client)
        m_client->onWindowSizeChange(m_size);
}

void DesktopWindowLinux::setMouseCursor(MouseCursor cursor)
{
    unsigned int x11Cursor;
    switch (cursor) {
        case Arrow:
            x11Cursor = XC_left_ptr;
            break;
        case Hand:
            x11Cursor = XC_hand2;
            break;
    }

    if (m_currentX11Cursor == x11Cursor)
        return;

    if (m_cursor) {
        XFreeCursor(m_display, m_cursor);
        m_cursor = 0;
    }
    m_cursor = XCreateFontCursor(m_display, x11Cursor);
    XDefineCursor(m_display, m_window, m_cursor);
    m_currentX11Cursor = x11Cursor;
}
