#include "DesktopWindow.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

#include "XlibEventSource.h"
#include "XlibEventUtils.h"

#include <stdio.h>

static Atom wmDeleteMessageAtom;
static const double DOUBLE_CLICK_INTERVAL = 300;

class DesktopWindowLinux : public DesktopWindow, public XlibEventSource::Client {
public:
    DesktopWindowLinux(DesktopWindowClient* client, int width, int height);
    ~DesktopWindowLinux();
    void makeCurrent();
    void swapBuffers();

private:
    void createGLContext();
    void destroyGLContext();
    VisualID setupXVisualID();
    Window createXWindow(VisualID visualID);
    void updateSizeIfNeeded(int width, int height);

    void handleXEvent(const XEvent&);
    void updateClickCount(const XButtonPressedEvent* event);

    XVisualInfo* m_visualInfo;
    GLXContext m_context;
    XlibEventSource* m_eventSource;
    Display* m_display;
    Window m_window;

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
    , m_display(XOpenDisplay(0))
    , m_lastClickTime(0)
    , m_lastClickX(0)
    , m_lastClickY(0)
    , m_lastClickButton(kWKEventMouseButtonNoButton)
    , m_clickCount(0)
{
    if (!m_display)
        throw "couldn't connect to X server"; // FIXME: Replace by a exception type

    VisualID visualID = setupXVisualID();
    m_eventSource = new XlibEventSource(m_display, this);
    m_window = createXWindow(visualID);

    createGLContext();
    makeCurrent();
    glEnable(GL_DEPTH_TEST);
}

DesktopWindowLinux::~DesktopWindowLinux()
{
    delete m_eventSource;
    destroyGLContext();
    XDestroyWindow(m_display, m_window);
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

VisualID DesktopWindowLinux::setupXVisualID()
{
    GLint attributes[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None
    };
    m_visualInfo = glXChooseVisual(m_display, 0, attributes);

    if (!m_visualInfo)
        throw "glXChooseVisual() failed.";

    return m_visualInfo->visualid;
}

void DesktopWindowLinux::createGLContext()
{
    m_context = glXCreateContext(m_display, m_visualInfo, 0, GL_TRUE);
    if (!m_context)
        throw "glXCreateContext() failed.";
}

void DesktopWindowLinux::destroyGLContext()
{
    glXMakeCurrent(m_display, None, 0);
    glXDestroyContext(m_display, m_context);
    XFree(m_visualInfo);
}

Window DesktopWindowLinux::createXWindow(VisualID visualID)
{
    XVisualInfo visualInfoTemplate;
    int visualInfoCount;
    visualInfoTemplate.visualid = visualID;
    XVisualInfo* visualInfo = XGetVisualInfo(m_display, VisualIDMask, &visualInfoTemplate, &visualInfoCount);
    if (!visualInfo)
        throw "couldn't get X visual";

    Window rootWindow = DefaultRootWindow(m_display);

    XSetWindowAttributes setAttributes;
    setAttributes.colormap = XCreateColormap(m_display, rootWindow, visualInfo->visual, AllocNone);
    setAttributes.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | PointerMotionMask;

    Window window = XCreateWindow(m_display, rootWindow, 0, 0, m_size.width, m_size.height, 0, visualInfo->depth, InputOutput, visualInfo->visual, CWColormap | CWEventMask, &setAttributes);
    XFree(visualInfo);

    wmDeleteMessageAtom = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(m_display, window, &wmDeleteMessageAtom, 1);

    XMapWindow(m_display, window);
    XStoreName(m_display, window, "Browser");

    return window;
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
        XEventToNix(event, &ev);
        m_client->onKeyPress(&ev);
        break;
    }
    case KeyRelease: {
        NIXKeyEvent ev;
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
        if (event.xclient.data.l[0] == wmDeleteMessageAtom)
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
        || m_lastClickButton != event->button
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

