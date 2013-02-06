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
#include <vector>

#include "FatalError.h"
#include "InjectedBundleGlue.h"
#include "Tab.h"

Browser::Browser()
    : m_displayUpdateScheduled(false)
    , m_window(DesktopWindow::create(this, 1024, 600))
    , m_glue(0)
    , m_uiFocused(true)
    , m_toolBarHeight(0)
    , m_currentTab(-1)
{
    m_mainLoop = g_main_loop_new(0, false);

    initUi();
}

Browser::~Browser()
{
    for (std::pair<const int, Tab*> p : m_tabs)
        delete p.second;
    m_tabs.clear();

    g_main_loop_unref(m_mainLoop);
    NIXViewRelease(m_uiView);
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

static std::string getUiFile()
{
    std::vector<std::string> locations = {
        getApplicationPath() + "/../share/Lasso/ui.html",
        UI_SEARCH_PATH "/ui.html",
    };
    for (const std::string& location : locations) {
        if (FILE* fp = fopen(location.c_str(), "r")) {
            fclose(fp);
            return location;
        } else {
            puts(location.c_str());
            fflush(stdout);
        }
    }
    throw FatalError("Can't find UI files.");
}

void Browser::initUi()
{
    const std::string appPath = getApplicationPath();
    // FIXME Find a better way to find where the injected bundle is
    WKStringRef wkStr = WKStringCreateWithUTF8CString((appPath + "/../UIInjectedBundle/libUiBundle.so").c_str());
    m_uiContext = WKContextCreateWithInjectedBundlePath(wkStr);
    WKRelease(wkStr);
    wkStr = WKStringCreateWithUTF8CString("Browser");
    m_uiPageGroup = WKPageGroupCreateWithIdentifier(wkStr);
    WKRelease(wkStr);

    WKPreferencesRef preferences = WKPageGroupGetPreferences(m_uiPageGroup);
    WKPreferencesSetAcceleratedCompositingEnabled(preferences, true);

    m_uiView = NIXViewCreate(m_uiContext, m_uiPageGroup);

    NIXViewClient client;
    std::memset(&client, 0, sizeof(NIXViewClient));
    client.version = kNIXViewClientCurrentVersion;
    client.clientInfo = this;
    client.viewNeedsDisplay = [](NIXView, WKRect, const void* client) {
        ((Browser*)client)->scheduleUpdateDisplay();
    };
    client.webProcessCrashed = [](NIXView, const OpaqueWKString*, const void*) {
        puts("UI Webprocess crashed :-(");
    };

    NIXViewSetViewClient(m_uiView, &client);
    NIXViewInitialize(m_uiView);
    NIXViewSetFocused(m_uiView, true);
    NIXViewSetVisible(m_uiView, true);
    NIXViewSetActive(m_uiView, true);
    NIXViewSetSize(m_uiView, m_window->size());
    m_uiPage = NIXViewGetPage(m_uiView);

    m_glue = new InjectedBundleGlue(m_uiContext);
    m_glue->bind("_addTab", this, &Browser::addTab);
    m_glue->bind("_closeTab", this, &Browser::closeTab);
    m_glue->bind("_toolBarHeightChanged", this, &Browser::toolBarHeightChanged);
    m_glue->bind("_setCurrentTab", this, &Browser::setCurrentTab);
    m_glue->bind("_loadUrl", this, &Browser::loadUrlOnCurrentTab);
    m_glue->bindToDispatcher("_back", this, &Tab::back);

    std::string uiHtml = getUiFile();
    WKURLRef wkUrl = WKURLCreateWithUTF8CString(("file://" + uiHtml).c_str());
    WKPageLoadURL(NIXViewGetPage(m_uiView), wkUrl);
    WKRelease(wkUrl);

    // context used on all webpages.
    wkStr = WKStringCreateWithUTF8CString((appPath + "/../ContentsInjectedBundle/libPageBundle.so").c_str());
    m_webContext = WKContextCreateWithInjectedBundlePath(wkStr);
    WKRelease(wkStr);
    wkStr = WKStringCreateWithUTF8CString("Web");
    m_webPageGroup = WKPageGroupCreateWithIdentifier(wkStr);
    WKRelease(wkStr);
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
bool Browser::sendMouseEventToPage(T event)
{
    if (event->y > m_toolBarHeight && m_currentTab != -1) {
        event->y -= m_toolBarHeight;
        currentTab()->sendMouseEvent(event);
        return true;
    }
    return false;
}

void Browser::onWindowExpose()
{
    scheduleUpdateDisplay();
}

void Browser::onKeyPress(NIXKeyEvent* event)
{
    if (!m_uiView)
        return;

    if (m_uiFocused)
        NIXViewSendKeyEvent(m_uiView, event);
    else if (m_currentTab != -1)
        currentTab()->sendKeyEvent(event);
}

void Browser::onKeyRelease(NIXKeyEvent* event)
{
    onKeyPress(event);
}

void Browser::onMouseWheel(NIXWheelEvent* event)
{
    sendMouseEventToPage(event);
}

void Browser::onMousePress(NIXMouseEvent* event)
{
    if (!m_uiView)
        return;

    if (!sendMouseEventToPage(event)) {
        NIXMouseEvent releaseEvent;
        std::memcpy(&releaseEvent, event, sizeof(NIXMouseEvent));
        releaseEvent.type = kNIXInputEventTypeMouseUp;
        m_uiFocused = true;

        NIXViewSendMouseEvent(m_uiView, event);
        NIXViewSendMouseEvent(m_uiView, &releaseEvent);
    }
}

void Browser::onMouseRelease(NIXMouseEvent* event)
{
    sendMouseEventToPage(event);
}

void Browser::onMouseMove(NIXMouseEvent* event)
{
    if (!m_uiView)
        return;

    if (!sendMouseEventToPage(event))
        NIXViewSendMouseEvent(m_uiView, event);
}

void Browser::onWindowSizeChange(WKSize size)
{
    if (!m_uiView)
        return;

    NIXViewSetSize(m_uiView, size);
    if (m_currentTab != -1) {
        size.height -= m_toolBarHeight;
        NIXViewSetSize(currentTab()->webView(), size);
    }
    scheduleUpdateDisplay();
}

void Browser::onWindowClose()
{
    g_main_loop_quit(m_mainLoop);
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
    glViewport(0, 0, size.width, size.height);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    NIXViewPaintToCurrentGLContext(m_uiView);

    if (m_currentTab != -1)
        NIXViewPaintToCurrentGLContext(currentTab()->webView());

    m_window->swapBuffers();
}

Tab* Browser::currentTab()
{
    return m_tabs[m_currentTab];
}

void Browser::addTab(const int& tabId)
{
    assert(m_tabs.count(tabId) == 0);

    Tab* tab = new Tab(tabId, this, m_webContext, m_webPageGroup);
    tab->setViewportTransformation(&m_webViewsTransform);
    m_tabs[tabId] = tab;

    WKSize wndSize = m_window->size();
    wndSize.height -= m_toolBarHeight;
    tab->setSize(wndSize);

    m_currentTab = tabId;
}

void Browser::closeTab(const int& tabId)
{
    assert(m_tabs.count(tabId) != 0);

    Tab* tab = m_tabs[tabId];
    m_tabs.erase(tabId);
    m_currentTab = -1;
    delete tab;
    if (m_tabs.empty())
        onWindowClose();
}

void Browser::toolBarHeightChanged(const int& height)
{
    m_toolBarHeight = height;
    m_webViewsTransform = NIXMatrixMakeTranslation(0, m_toolBarHeight);

    // FIXME: Better to delay this global relayout in a near future
    WKSize contentsSize = m_window->size();
    contentsSize.height -= m_toolBarHeight;

    for (auto p : m_tabs) {
        Tab* tab = p.second;
        tab->setViewportTransformation(&m_webViewsTransform);
        tab->setSize(contentsSize);
    }
}

void Browser::setCurrentTab(const int& tabId)
{
    if (!m_tabs.count(tabId))
        return;
    m_currentTab = tabId;
    NIXViewSetSize(currentTab()->webView(), m_window->size());
}

void Browser::loadUrlOnCurrentTab(const std::string& url)
{
    m_uiFocused = false;
    currentTab()->loadUrl(url);
}
