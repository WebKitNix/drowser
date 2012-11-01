#include "Bossa.h"

#include <WebKit2/WKContext.h>
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKType.h>
#include <WebKit2/WKURL.h>
#include <WebKit2/WKPreferences.h>
#include <WebKit2/WKPreferencesPrivate.h>
#include <GL/gl.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <cairo.h>
#include <glib.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <libgen.h>
#include <limits.h>
#include <string>

#include "XlibEventUtils.h"

static const double DOUBLE_CLICK_INTERVAL = 300;
static const int UI_HEIGHT = 53;

Bossa::Bossa()
    : m_displayUpdateScheduled(false)
    , m_window(new LinuxWindow(this, 1024, 600))
    , m_lastClickTime(0)
    , m_lastClickX(0)
    , m_lastClickY(0)
    , m_lastClickButton(Nix::MouseEvent::NoButton)
    , m_clickCount(0)
{
    m_mainLoop = g_main_loop_new(0, false);

    initUi();
    cairo_matrix_init_translate(&m_webTransform, 0, UI_HEIGHT);
}

Bossa::~Bossa()
{
     g_main_loop_unref(m_mainLoop);
     delete m_uiView;
     delete m_window;
}

extern "C" {
static void didReceiveMessageFromInjectedBundle(WKContextRef page, WKStringRef messageName, WKTypeRef messageBody, const void *clientInfo)
{
    Bossa* self = reinterpret_cast<Bossa*>(const_cast<void*>(clientInfo));
    if (WKStringIsEqualToUTF8CString(messageName, "addTab"))
        self->addTab( WKUInt64GetValue((WKUInt64Ref)messageBody) );
    else if (WKStringIsEqualToUTF8CString(messageName, "setCurrentTab"))
        self->setCurrentTab( WKUInt64GetValue((WKUInt64Ref)messageBody) );
    else if (WKStringIsEqualToUTF8CString(messageName, "loadUrl")) {
        WKStringRef wkUrl = reinterpret_cast<WKStringRef>(messageBody);
        size_t wkUrlSize = WKStringGetMaximumUTF8CStringSize(wkUrl);
        char* buffer = new char[wkUrlSize + 1];
        WKStringGetUTF8CString(wkUrl, buffer, wkUrlSize);
        self->loadUrl(buffer);
        delete[] buffer;
    }
}
}

static std::string getApplicationPath()
{
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, PATH_MAX);
    if (len != -1) {
        path[len] = 0;
        char* dirPath = strdup(path);
        dirPath = dirname(dirPath);
        strncpy(path, dirPath, PATH_MAX);
        free(dirPath);
        return path;
    } else {
        fprintf(stderr, "Can't read /proc/self/exe!\n");
        exit(1);
    }
}

void Bossa::initUi()
{
    const std::string appPath = getApplicationPath();
    m_uiContext = WKContextCreateWithInjectedBundlePath(WKStringCreateWithUTF8CString((appPath + "/UiBundle/libUiBundle.so").c_str()));
    m_uiPageGroup = WKPageGroupCreateWithIdentifier(WKStringCreateWithUTF8CString("Bossa"));

    WKPreferencesRef preferences = WKPageGroupGetPreferences(m_uiPageGroup);
    WKPreferencesSetAcceleratedCompositingEnabled(preferences, true);

    m_uiView = Nix::WebView::create(m_uiContext, m_uiPageGroup, this);
    m_uiView->initialize();
    m_uiView->setFocused(true);
    m_uiView->setVisible(true);
    m_uiView->setActive(true);
    m_uiView->setSize(m_window->size().first, m_window->size().second);
    WKPageLoadURL(m_uiView->pageRef(), WKURLCreateWithUTF8CString(("file://" + appPath + "/ui.html").c_str()));

    WKContextInjectedBundleClient bundleClient;
    std::memset(&bundleClient, 0, sizeof(bundleClient));
    bundleClient.clientInfo = this;
    bundleClient.didReceiveMessageFromInjectedBundle = ::didReceiveMessageFromInjectedBundle;
    WKContextSetInjectedBundleClient(m_uiContext, &bundleClient);

    m_window->makeCurrent();

    // context used on all webpages.
    m_webContext = WKContextCreate();
    m_webPageGroup = WKPageGroupCreateWithIdentifier(WKStringCreateWithUTF8CString("Web"));
    WKPreferencesRef webPreferences = WKPageGroupGetPreferences(m_webPageGroup);
    WKPreferencesSetAcceleratedCompositingEnabled(webPreferences, true);
}

int Bossa::run()
{
    g_main_loop_run(m_mainLoop);
    return 0;
}

void Bossa::handleExposeEvent()
{
    scheduleUpdateDisplay();
}

static inline bool isKeypadKeysym(const KeySym symbol)
{
    // Following keypad symbols are specified on Xlib Programming Manual (section: Keyboard Encoding).
    return (symbol >= 0xFF80 && symbol <= 0xFFBD);
}

static KeySym chooseSymbolForXKeyEvent(const XKeyEvent& event, bool* useUpperCase)
{
    KeySym firstSymbol = XLookupKeysym(const_cast<XKeyEvent*>(&event), 0);
    KeySym secondSymbol = XLookupKeysym(const_cast<XKeyEvent*>(&event), 1);
    KeySym lowerCaseSymbol, upperCaseSymbol, chosenSymbol;
    XConvertCase(firstSymbol, &lowerCaseSymbol, &upperCaseSymbol);
    bool numLockModifier = event.state & Mod2Mask;
    bool capsLockModifier = event.state & LockMask;
    bool shiftModifier = event.state & ShiftMask;
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

static Nix::KeyEvent convertXKeyEventToNixKeyEvent(const XKeyEvent& event, const KeySym& symbol, bool useUpperCase)
{
    Nix::KeyEvent ev;
    ev.type = (event.type == KeyPress) ? Nix::InputEvent::KeyDown : Nix::InputEvent::KeyUp;
    ev.modifiers = convertXEventModifiersToNativeModifiers(event.state);
    ev.timestamp = convertXEventTimeToNixTimestamp(event.time);
    ev.shouldUseUpperCase = useUpperCase;
    ev.isKeypad = isKeypadKeysym(symbol);
    ev.key = convertXKeySymToNativeKeycode(symbol);
    return ev;
}

void Bossa::handleKeyPressEvent(const XKeyPressedEvent& event)
{
    if (!m_uiView)
        return;

    bool shouldUseUpperCase;
    KeySym symbol = chooseSymbolForXKeyEvent(event, &shouldUseUpperCase);
    Nix::KeyEvent ev = convertXKeyEventToNixKeyEvent(event, symbol, shouldUseUpperCase);
    m_uiView->sendEvent(ev);
}

void Bossa::handleKeyReleaseEvent(const XKeyReleasedEvent& event)
{
    if (!m_uiView)
        return;

    bool shouldUseUpperCase;
    KeySym symbol = chooseSymbolForXKeyEvent(event, &shouldUseUpperCase);
    Nix::KeyEvent ev = convertXKeyEventToNixKeyEvent(event, symbol, shouldUseUpperCase);
    m_uiView->sendEvent(ev);
}

void Bossa::updateClickCount(const XButtonPressedEvent& event)
{
    if (m_lastClickX != event.x
        || m_lastClickY != event.y
        || m_lastClickButton != event.button
        || event.time - m_lastClickTime >= DOUBLE_CLICK_INTERVAL)
        m_clickCount = 1;
    else
        ++m_clickCount;

    m_lastClickX = event.x;
    m_lastClickY = event.y;
    m_lastClickButton = convertXEventButtonToNativeMouseButton(event.button);
    m_lastClickTime = event.time;
}

void Bossa::handleWheelEvent(const XButtonPressedEvent& event)
{
    // Same constant we use inside WebView to calculate the ticks. See also WebCore::Scrollbar::pixelsPerLineStep().
    const float pixelsPerStep = 40.0f;

    Nix::WheelEvent ev;
    ev.type = Nix::InputEvent::Wheel;
    ev.modifiers = convertXEventModifiersToNativeModifiers(event.state);
    ev.timestamp = convertXEventTimeToNixTimestamp(event.time);
    ev.x = event.x;
    ev.y = event.y;
    ev.globalX = event.x_root;
    ev.globalY = event.y_root;
    ev.delta = pixelsPerStep * (event.button == 4 ? 1 : -1);
    ev.orientation = event.state & Mod1Mask ? Nix::WheelEvent::Horizontal : Nix::WheelEvent::Vertical;
    m_uiView->sendEvent(ev);
}

void Bossa::handleButtonPressEvent(const XButtonPressedEvent& event)
{
    if (!m_uiView)
        return;

    if (event.button == 4 || event.button == 5) {
        handleWheelEvent(event);
        return;
    }

    updateClickCount(event);

    Nix::MouseEvent ev;
    ev.type = Nix::InputEvent::MouseDown;
    ev.button = convertXEventButtonToNativeMouseButton(event.button);
    ev.x = event.x;
    ev.y = event.y;
    ev.globalX = event.x_root;
    ev.globalY = event.y_root;
    ev.clickCount = m_clickCount;
    ev.modifiers = convertXEventModifiersToNativeModifiers(event.state);
    ev.timestamp = convertXEventTimeToNixTimestamp(event.time);

    m_uiView->sendEvent(ev);
}

void Bossa::handleButtonReleaseEvent(const XButtonReleasedEvent& event)
{
    if (!m_uiView || event.button == 4 || event.button == 5)
        return;

    Nix::MouseEvent ev;
    ev.type = Nix::InputEvent::MouseUp;
    ev.button = convertXEventButtonToNativeMouseButton(event.button);
    ev.x = event.x;
    ev.y = event.y;
    ev.globalX = event.x_root;
    ev.globalY = event.y_root;
    ev.clickCount = 0;
    ev.modifiers = convertXEventModifiersToNativeModifiers(event.state);
    ev.timestamp = convertXEventModifiersToNativeModifiers(event.state);
    m_uiView->sendEvent(ev);
}

void Bossa::handlePointerMoveEvent(const XPointerMovedEvent& event)
{
    if (!m_uiView)
        return;

    Nix::MouseEvent ev;
    ev.type = Nix::InputEvent::MouseMove;
    ev.button = Nix::MouseEvent::NoButton;
    ev.x = event.x;
    ev.y = event.y;
    ev.globalX = event.x_root;
    ev.globalY = event.y_root;
    ev.clickCount = 0;
    ev.modifiers = convertXEventModifiersToNativeModifiers(event.state);
    ev.timestamp = convertXEventTimeToNixTimestamp(event.time);
    m_uiView->sendEvent(ev);
}

void Bossa::handleSizeChanged(int width, int height)
{
    if (!m_uiView)
        return;

    m_uiView->setSize(width, height);
    if (m_tabs.size())
        currentTab()->setSize(width, height - UI_HEIGHT);
    scheduleUpdateDisplay();
}

void Bossa::handleClosed()
{
    g_main_loop_quit(m_mainLoop);
}

void Bossa::viewNeedsDisplay(int, int, int, int)
{
    scheduleUpdateDisplay();
}

void Bossa::webProcessCrashed(WKStringRef url)
{
    puts("UI Webprocess crashed :-(");
}

gboolean callUpdateDisplay(gpointer data)
{
    Bossa* browser = reinterpret_cast<Bossa*>(data);

    assert(browser->m_displayUpdateScheduled);
    browser->m_displayUpdateScheduled = false;
    browser->updateDisplay();
    return 0;
}

void Bossa::scheduleUpdateDisplay()
{
    if (m_displayUpdateScheduled)
        return;

    m_displayUpdateScheduled = true;
    g_timeout_add(0, callUpdateDisplay, this);
}

void Bossa::updateDisplay()
{
    std::pair<int, int> size = m_window->size();
    glViewport(0, 0, m_window->size().first, m_window->size().second);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_uiView->paintToCurrentGLContext();

    if (m_tabs.size())
        currentTab()->paintToCurrentGLContext();

    m_window->swapBuffers();
}

Nix::WebView* Bossa::currentTab()
{
    return m_tabs[m_currentTab];
}

void Bossa::addTab(int tabId)
{
    assert(m_tabs.count(tabId) == 0);
    Nix::WebView* view = Nix::WebView::create(m_webContext, m_webPageGroup, this);
    view->initialize();
    view->setFocused(true);
    view->setVisible(true);
    view->setActive(true);
    view->setUserViewportTransformation(m_webTransform);
    view->setSize(m_window->size().first, m_window->size().second - 66);

    m_tabs[tabId] = view;
    m_currentTab = tabId;
}

void Bossa::setCurrentTab(int tabId)
{
    if (!m_tabs.count(tabId))
        return;
    m_currentTab = tabId;
    currentTab()->setSize(m_window->size().first, m_window->size().second - UI_HEIGHT);
}

void Bossa::loadUrl(const char* url)
{
    printf("Load URL: %s\n", url);
    WKPageLoadURL(currentTab()->pageRef(), WKURLCreateWithUTF8CString(url));
}
