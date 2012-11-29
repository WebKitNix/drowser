#include "Bossa.h"

#include <WebKit2/WKContext.h>
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKType.h>
#include <WebKit2/WKURL.h>
#include <WebKit2/WKPage.h>
#include <WebKit2/WKPreferences.h>
#include <WebKit2/WKPreferencesPrivate.h>
#include <GL/gl.h>
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

static const int UI_HEIGHT = 65;

Bossa::Bossa()
    : m_displayUpdateScheduled(false)
    , m_window(DesktopWindow::create(this, 1024, 600))
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
    if (WKStringIsEqualToUTF8CString(messageName, "addTab")) {
        self->addTab( WKUInt64GetValue((WKUInt64Ref)messageBody) );
    } else if (WKStringIsEqualToUTF8CString(messageName, "setCurrentTab")) {
        self->setCurrentTab( WKUInt64GetValue((WKUInt64Ref)messageBody) );
    } else if (WKStringIsEqualToUTF8CString(messageName, "loadUrl")) {
        WKStringRef wkUrl = reinterpret_cast<WKStringRef>(messageBody);
        size_t wkUrlSize = WKStringGetMaximumUTF8CStringSize(wkUrl);
        char* buffer = new char[wkUrlSize + 1];
        WKStringGetUTF8CString(wkUrl, buffer, wkUrlSize);
        self->loadUrl(buffer);
        delete[] buffer;
    } else if (WKStringIsEqualToUTF8CString(messageName, "back")) {
        self->back();
    } else if (WKStringIsEqualToUTF8CString(messageName, "forward")) {
        self->forward();
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
    m_uiView->setSize(m_window->size());

    WKContextInjectedBundleClient bundleClient;
    std::memset(&bundleClient, 0, sizeof(bundleClient));
    bundleClient.clientInfo = this;
    bundleClient.version = kWKContextInjectedBundleClientCurrentVersion;
    bundleClient.didReceiveMessageFromInjectedBundle = ::didReceiveMessageFromInjectedBundle;
    WKContextSetInjectedBundleClient(m_uiContext, &bundleClient);

    WKPageLoadURL(m_uiView->pageRef(), WKURLCreateWithUTF8CString(("file://" + appPath + "/ui.html").c_str()));

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

void Bossa::onWindowExpose()
{
    scheduleUpdateDisplay();
}

void Bossa::onKeyPress(Nix::KeyEvent* event)
{
    if (!m_uiView)
        return;

    m_uiView->sendEvent(*event);
}

void Bossa::onKeyRelease(Nix::KeyEvent* event)
{
    if (!m_uiView)
        return;

    m_uiView->sendEvent(*event);
}

void Bossa::onMouseWheel(Nix::WheelEvent* event)
{
    m_uiView->sendEvent(*event);
}

void Bossa::onMousePress(Nix::MouseEvent* event)
{
    if (!m_uiView)
        return;

    m_uiView->sendEvent(*event);
}

void Bossa::onMouseRelease(Nix::MouseEvent* event)
{
    m_uiView->sendEvent(*event);
}

void Bossa::onMouseMove(Nix::MouseEvent* event)
{
    if (!m_uiView)
        return;

    m_uiView->sendEvent(*event);
}

void Bossa::onWindowSizeChange(WKSize size)
{
    if (!m_uiView)
        return;

    m_uiView->setSize(size);
    if (m_tabs.size())
        currentTab()->setSize(size);
//         currentTab()->setSize(WKSizeMake(width, height - UI_HEIGHT));
    scheduleUpdateDisplay();
}

void Bossa::onWindowClose()
{
    g_main_loop_quit(m_mainLoop);
}

void Bossa::viewNeedsDisplay(WKRect)
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
    m_window->makeCurrent();

    WKSize size = m_window->size();
    glViewport(0, 0, m_window->size().width, m_window->size().height);
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
    view->setSize(m_window->size());

    m_tabs[tabId] = view;
    m_currentTab = tabId;
}

void Bossa::setCurrentTab(int tabId)
{
    if (!m_tabs.count(tabId))
        return;
    m_currentTab = tabId;
    currentTab()->setSize(m_window->size());
}

void Bossa::loadUrl(const char* url)
{
    printf("Load URL: %s\n", url);
    WKPageLoadURL(currentTab()->pageRef(), WKURLCreateWithUTF8CString(url));
}

void Bossa::back()
{
    WKPageRef page = currentTab()->pageRef();
    if (WKPageCanGoBack(page))
        WKPageGoBack(page);
}

void Bossa::forward()
{
    WKPageRef page = currentTab()->pageRef();
    if (WKPageCanGoForward(page))
        WKPageGoForward(page);
}
