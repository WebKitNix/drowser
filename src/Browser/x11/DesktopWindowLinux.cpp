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

#include <cstring>
#include <GL/glx.h>
#include <iostream>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include "FatalError.h"
#include "XlibEventSource.h"
#include "XlibEventUtils.h"

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
    DesktopWindowLinux(DesktopWindowClient* client, int width, int height, bool visible);
    ~DesktopWindowLinux();
    void makeCurrent();
    void swapBuffers();
    void setMouseCursor(unsigned shape);
    void setVisible(bool);
    bool visible() const;

private:
    void freeResources();
    void setup();
    void destroyGLContext();
    void updateSizeIfNeeded(int width, int height);

    void sendKeyboardEventToNix(const XEvent& event);
    void handleXEvent(const XEvent&);
    void updateClickCount(const XButtonPressedEvent* event);

    XVisualInfo* m_visualInfo;
    GLXContext m_context;
    XlibEventSource* m_eventSource;
    Display* m_display;
    Window m_window;
    XIM m_im;
    XIC m_ic;
    Cursor m_cursor;
    unsigned int m_currentX11Cursor;

    double m_lastClickTime;
    int m_lastClickX;
    int m_lastClickY;
    WKEventMouseButton m_lastClickButton;
    int m_clickCount;
    bool m_visible;
};

DesktopWindow* DesktopWindow::create(DesktopWindowClient* client, int width, int height, bool visible)
{
    return new DesktopWindowLinux(client, width, height, visible);
}

DesktopWindowLinux::DesktopWindowLinux(DesktopWindowClient* client, int width, int height, bool visible)
    : DesktopWindow(client, width, height)
    , m_eventSource(0)
    , m_display(0)
    , m_window(0)
    , m_im(0)
    , m_ic(0)
    , m_cursor(0)
    , m_currentX11Cursor(XC_left_ptr)
    , m_lastClickTime(0)
    , m_lastClickX(0)
    , m_lastClickY(0)
    , m_lastClickButton(kWKEventMouseButtonNoButton)
    , m_clickCount(0)
    , m_visible(visible)
{
    try {
        setup();
    } catch(const FatalError&) {
        freeResources();
        throw;
    }

    m_eventSource = new XlibEventSource(m_display, this);

    makeCurrent();
    glEnable(GL_DEPTH_TEST);
}

DesktopWindowLinux::~DesktopWindowLinux()
{
    freeResources();
}

void DesktopWindowLinux::freeResources()
{
    delete m_eventSource;
    if (m_context)
        destroyGLContext();
    if (m_window)
        XDestroyWindow(m_display, m_window);
    if (m_cursor)
        XFreeCursor(m_display, m_cursor);
    if (m_ic)
        XDestroyIC(m_ic);
    if (m_im)
        XCloseIM(m_im);
    if (m_display)
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
    char* loc = setlocale(LC_ALL, "");
    if (!loc)
        std::cerr << "Could not use the the default environment locale.\n";

    if (!XSupportsLocale())
        std::cerr << "Default locale \"" << (loc ? loc : "") << "\" is no supported.\n";

    // When changing the locale being used we must call XSetLocaleModifiers (refer to manpage).
    if (!XSetLocaleModifiers(""))
        std::cerr << "Could not set locale modifiers for locale \"" << (loc ? loc : "") << "\".\n";

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

    m_im = XOpenIM(m_display, 0, 0, 0);
    if (!m_im)
        throw FatalError("Could not open input method.");

    m_ic = XCreateIC(m_im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, m_window, NULL);
    if (!m_ic)
        throw FatalError("Could not open input context.");

    wmDeleteMessageAtom = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(m_display, m_window, &wmDeleteMessageAtom, 1);

    if (m_visible)
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

void DesktopWindowLinux::sendKeyboardEventToNix(const XEvent& event)
{
    if (XFilterEvent(const_cast<XEvent*>(&event), m_window))
        return;

    NIXKeyEvent ev;
    memset(&ev, 0, sizeof(NIXKeyEvent));
    char buf[20];
    bool shouldUseUpperCase;
    const XKeyEvent* keyEvent = reinterpret_cast<const XKeyEvent*>(&event);
    KeySym symbol = chooseSymbolForXKeyEvent(keyEvent, &shouldUseUpperCase);
    ev = convertXKeyEventToNixKeyEvent(keyEvent, symbol, shouldUseUpperCase);

    Status status;
    int count = Xutf8LookupString(m_ic, const_cast<XKeyEvent*>(keyEvent), buf, 20, &symbol, &status);
    if (count) {
        buf[count] = '\0';
        ev.text = buf;
    }

    if (ev.type == kNIXInputEventTypeKeyDown)
        m_client->onKeyPress(&ev);
    else
        m_client->onKeyRelease(&ev);
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
    case KeyPress:
    case KeyRelease:
        sendKeyboardEventToNix(event);
        break;
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
        ev.button = m_lastClickButton;
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

void DesktopWindowLinux::setMouseCursor(unsigned shape)
{
    static unsigned cursorShapes[] = {
        XC_left_ptr,                // "cursor/pointer",
        XC_X_cursor,                // "cursor/cross",
        XC_hand2,                   // "cursor/hand",
        XC_xterm,                   // "cursor/i_beam",
        XC_watch,                   // "cursor/wait",
        XC_question_arrow,          // "cursor/help",
        XC_right_side,              // "cursor/east_resize",
        XC_top_side,                // "cursor/north_resize",
        XC_top_right_corner,        // "cursor/north_east_resize",
        XC_top_left_corner,         // "cursor/north_west_resize",
        XC_bottom_side,             // "cursor/south_resize",
        XC_bottom_right_corner,     // "cursor/south_east_resize",
        XC_bottom_left_corner,      // "cursor/south_west_resize",
        XC_left_side,               // "cursor/west_resize",
        XC_sb_v_double_arrow,       // "cursor/north_south_resize",
        XC_sb_h_double_arrow,       // "cursor/east_west_resize",
        XC_left_ptr,                // "cursor/north_east_south_west_resize",
        XC_left_ptr,                // "cursor/north_west_south_east_resize",
        XC_left_ptr,                // "cursor/column_resize",
        XC_left_ptr,                // "cursor/row_resize",
        XC_tcross,                  // "cursor/middle_panning",
        XC_right_tee,               // "cursor/east_panning",
        XC_top_tee,                 // "cursor/north_panning",
        XC_ur_angle,                // "cursor/north_east_panning",
        XC_ul_angle,                // "cursor/north_west_panning",
        XC_bottom_tee,              // "cursor/south_panning",
        XC_lr_angle,                // "cursor/south_east_panning",
        XC_ll_angle,                // "cursor/south_west_panning",
        XC_left_tee,                // "cursor/west_panning",
        XC_fleur,                   // "cursor/move",                     duplicated!
        XC_left_ptr,                // "cursor/vertical_text",
        XC_left_ptr,                // "cursor/cell",
        XC_left_ptr,                // "cursor/context_menu",
        XC_left_ptr,                // "cursor/alias",
        XC_exchange,                // "cursor/progress",
        XC_left_ptr,                // "cursor/no_drop",
        XC_left_ptr,                // "cursor/copy",
        XC_left_ptr,                // "cursor/none",
        XC_X_cursor,                // "cursor/not_allowed",
        XC_X_cursor,                // "cursor/zoom_in",
        XC_X_cursor,                // "cursor/zoom_out",
        XC_fleur,                   // "cursor/grab",                     duplicated!
        XC_fleur,                   // "cursor/grabbing",                 duplicated!
        XC_left_ptr                 // custom type
    };

    if (shape > sizeof(cursorShapes)/sizeof(unsigned))
        return;

    unsigned int x11Cursor = cursorShapes[shape];
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

void DesktopWindowLinux::setVisible(bool visible)
{
    if (visible == m_visible)
        return;

    if (visible)
        XMapWindow(m_display, m_window);
    else
        XUnmapWindow(m_display, m_window);

    m_visible = visible;
}

bool DesktopWindowLinux::visible() const
{
    return m_visible;
}
