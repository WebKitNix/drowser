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

#ifndef Browser_h
#define Browser_h

#include "DesktopWindow.h"
#include <glib.h>
#include <NIXView.h>
#include <map>
#include <string>

class Tab;

std::string getApplicationPath();

extern "C" {
gboolean callUpdateDisplay(gpointer);
}

class InjectedBundleGlue;

class Browser : public DesktopWindowClient
{
public:
    Browser();
    ~Browser();

    int run();

    // DesktopWindowClient
    virtual void onWindowExpose();
    virtual void onKeyPress(NIXKeyEvent*);
    virtual void onKeyRelease(NIXKeyEvent*);
    virtual void onMousePress(NIXMouseEvent*);
    virtual void onMouseRelease(NIXMouseEvent*);
    virtual void onMouseMove(NIXMouseEvent*);
    virtual void onMouseWheel(NIXWheelEvent*);
    virtual void onWindowSizeChange(WKSize);
    virtual void onWindowClose();

    void addTab(const int& tabId);
    void closeTab(const int& tabId);
    void toolBarHeightChanged(const int& height);
    void setCurrentTab(const int& tabId);
    void loadUrlOnCurrentTab(const std::string& url);
    Tab* currentTab();

    template<typename Param, typename Obj>
    void dispatchMessage(const Param& param, void (Obj::*method)(const Param&));
    template<typename Obj>
    void dispatchMessage(void (Obj::*method)());

    WKPageRef ui() { return m_uiPage; }
    WKPageGroupRef contentPageGroup() { return m_contentPageGroup; }

    WKSize contentsSize() const;

    void scheduleUpdateDisplay();

private:
    GMainLoop* m_mainLoop;
    bool m_displayUpdateScheduled;
    DesktopWindow* m_window;
    InjectedBundleGlue* m_glue;

    NIXView m_uiView;
    WKPageRef m_uiPage;
    WKContextRef m_uiContext;
    WKPageGroupRef m_uiPageGroup;

    bool m_uiFocused;
    int m_toolBarHeight;
    NIXMatrix m_webViewsTransform;

    std::map<int, Tab*> m_tabs;
    int m_currentTab;
    NIXMatrix m_webTransform;
    WKPageGroupRef m_contentPageGroup;

    template<typename T>
    bool sendMouseEventToPage(T event);

    void updateDisplay();
    void initUi();

    friend gboolean callUpdateDisplay(gpointer);
};

#endif
