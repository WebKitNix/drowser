#include "Browser.h"

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

#include "InjectedBundleGlue.h"
#include "UiConstants.h"
#include "Tab.h"

Browser::Browser()
    : m_displayUpdateScheduled(false)
    , m_window(DesktopWindow::create(this, 1024, 600))
    , m_glue(0)
    , m_uiFocused(true)
{
    m_mainLoop = g_main_loop_new(0, false);

    initUi();
    cairo_matrix_init_translate(&m_webTransform, 0, UI_HEIGHT);
}

Browser::~Browser()
{
     g_main_loop_unref(m_mainLoop);
     delete m_uiView;
     delete m_window;
     delete m_glue;
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

void Browser::initUi()
{
    const std::string appPath = getApplicationPath();
    m_uiContext = WKContextCreateWithInjectedBundlePath(WKStringCreateWithUTF8CString((appPath + "/UiBundle/libUiBundle.so").c_str()));
    m_uiPageGroup = WKPageGroupCreateWithIdentifier(WKStringCreateWithUTF8CString("Browser"));

    WKPreferencesRef preferences = WKPageGroupGetPreferences(m_uiPageGroup);
    WKPreferencesSetAcceleratedCompositingEnabled(preferences, true);

    m_uiView = Nix::WebView::create(m_uiContext, m_uiPageGroup, this);
    m_uiView->initialize();
    m_uiView->setFocused(true);
    m_uiView->setVisible(true);
    m_uiView->setActive(true);
    m_uiView->setSize(m_window->size());

    m_glue = new InjectedBundleGlue(m_uiContext);
    m_glue->bind("_addTab", this, &Browser::addTab);
    m_glue->bind("_setCurrentTab", this, &Browser::setCurrentTab);
    m_glue->bindToDispatcher("_loadUrl", this, &Tab::loadUrl);
    m_glue->bindToDispatcher("_back", this, &Tab::back);

    // FIXME: This should probably be a list of location to search for the Ui files.
    WKPageLoadURL(m_uiView->pageRef(), WKURLCreateWithUTF8CString("file://" UI_SEARCH_PATH "/ui.html"));

    // context used on all webpages.
    m_webContext = WKContextCreate();
    m_webPageGroup = WKPageGroupCreateWithIdentifier(WKStringCreateWithUTF8CString("Web"));
    WKPreferencesRef webPreferences = WKPageGroupGetPreferences(m_webPageGroup);
    WKPreferencesSetAcceleratedCompositingEnabled(webPreferences, true);
}

int Browser::run()
{
    g_main_loop_run(m_mainLoop);
    return 0;
}

template<typename Param, typename Obj>
void Browser::dispatchMessage(const Param& param, void (Obj::*method)(const Param&))
{
    assert(currentTab());
    (currentTab()->*method)(param);
}

template<typename Obj>
void Browser::dispatchMessage(void (Obj::*method)())
{
    assert(currentTab());
    (currentTab()->*method)();
}

template<typename T>
void Browser::sendEvent(T event)
{
    currentTab()->webView()->sendEvent(*event);
}

template<typename T>
bool Browser::sendEventToPage(T event)
{
    if (event->y > UI_HEIGHT && !m_tabs.empty()) {
        event->y -= UI_HEIGHT;
        sendEvent(event);
        return true;
    }
    return false;
}

void Browser::onWindowExpose()
{
    scheduleUpdateDisplay();
}

void Browser::onKeyPress(Nix::KeyEvent* event)
{
    if (!m_uiView)
        return;

    if (m_uiFocused)
        m_uiView->sendEvent(*event);
    else if (!m_tabs.empty())
        currentTab()->webView()->sendEvent(*event);
}

void Browser::onKeyRelease(Nix::KeyEvent* event)
{
    onKeyPress(event);
}

void Browser::onMouseWheel(Nix::WheelEvent* event)
{
    sendEventToPage(event);
}

void Browser::onMousePress(Nix::MouseEvent* event)
{
    if (!m_uiView)
        return;

    if (!sendEventToPage(event)) {
        Nix::MouseEvent releaseEvent;
        std::memcpy(&releaseEvent, event, sizeof(Nix::MouseEvent));
        releaseEvent.type = Nix::InputEvent::MouseUp;
        m_uiFocused = true;

        m_uiView->sendEvent(*event);
        m_uiView->sendEvent(releaseEvent);
    }
}

void Browser::onMouseRelease(Nix::MouseEvent* event)
{
    sendEventToPage(event);
}

void Browser::onMouseMove(Nix::MouseEvent* event)
{
    if (!m_uiView)
        return;

    if (!sendEventToPage(event))
        m_uiView->sendEvent(*event);
}

void Browser::onWindowSizeChange(WKSize size)
{
    if (!m_uiView)
        return;

    m_uiView->setSize(size);
    if (m_tabs.size()) {
        size.height -= UI_HEIGHT;
        currentTab()->webView()->setSize(size);
    }
    scheduleUpdateDisplay();
}

void Browser::onWindowClose()
{
    g_main_loop_quit(m_mainLoop);
}

void Browser::viewNeedsDisplay(WKRect)
{
    scheduleUpdateDisplay();
}

void Browser::webProcessCrashed(WKStringRef url)
{
    puts("UI Webprocess crashed :-(");
}

gboolean callUpdateDisplay(gpointer data)
{
    Browser* browser = reinterpret_cast<Browser*>(data);

    assert(browser->m_displayUpdateScheduled);
    browser->m_displayUpdateScheduled = false;
    browser->updateDisplay();
    return 0;
}

void Browser::scheduleUpdateDisplay()
{
    if (m_displayUpdateScheduled)
        return;

    m_displayUpdateScheduled = true;
    g_timeout_add(0, callUpdateDisplay, this);
}

void Browser::updateDisplay()
{
    m_window->makeCurrent();

    WKSize size = m_window->size();
    glViewport(0, 0, m_window->size().width, m_window->size().height);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_uiView->paintToCurrentGLContext();

    if (m_tabs.size())
        currentTab()->webView()->paintToCurrentGLContext();

    m_window->swapBuffers();
}

Tab* Browser::currentTab()
{
    return m_tabs[m_currentTab];
}

void Browser::addTab(int tabId)
{
    assert(m_tabs.count(tabId) == 0);
    Nix::WebView* view = Nix::WebView::create(m_webContext, m_webPageGroup, this);
    view->initialize();
    view->setFocused(true);
    view->setVisible(true);
    view->setActive(true);
    view->setUserViewportTransformation(m_webTransform);
    WKSize wndSize = m_window->size();
    wndSize.height -= UI_HEIGHT;
    view->setSize(wndSize);

    m_tabs[tabId] = new Tab(view);
    m_currentTab = tabId;
}

void Browser::setCurrentTab(int tabId)
{
    if (!m_tabs.count(tabId))
        return;
    m_currentTab = tabId;
    currentTab()->webView()->setSize(m_window->size());
}
