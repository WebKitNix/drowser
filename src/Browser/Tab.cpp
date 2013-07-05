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

#include "Tab.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <cstring>
#include <WebKit2/WKContext.h>
#include <WebKit2/WKError.h>
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKPage.h>
#include <WebKit2/WKFrame.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKURL.h>
#include <WebKit2/WKURLRequest.h>
#include <WebKit2/WKType.h>
#include <WebKit2/WKPreferences.h>
#include <WebKit2/WKPreferencesPrivate.h>
#include <WebKit2/WKHitTestResult.h>
#include "Browser.h"
#include "InjectedBundleGlue.h"

static int nextTabId = 0;

Tab::Tab(Browser* browser)
    : m_id(nextTabId++)
    , m_browser(browser)
{
    // FIXME Find a good way to find where the injected bundle is
    WKStringRef wkStr = WKStringCreateWithUTF8CString((getApplicationPath() + "/../ContentsInjectedBundle/libPageBundle.so").c_str());
    m_context = WKContextCreateWithInjectedBundlePath(wkStr);
    WKRelease(wkStr);

    m_view = WKViewCreate(m_context, browser->contentPageGroup());
    WKViewInitialize(m_view);
    WKViewSetIsFocused(m_view, true);
    WKViewSetIsVisible(m_view, true);
    m_page = WKViewGetPage(m_view);

    WKViewClient client;
    std::memset(&client, 0, sizeof(WKViewClient));
    client.version = kWKViewClientCurrentVersion;
    client.clientInfo = this;
    client.viewNeedsDisplay = &Tab::onViewNeedsDisplayCallback;

    WKViewSetViewClient(m_view, &client);

    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(WKPageLoaderClient));
    loaderClient.version = kWKPageLoaderClientCurrentVersion;
    loaderClient.clientInfo = this;
    loaderClient.didStartProgress = &Tab::onStartProgressCallback;
    loaderClient.didChangeProgress = &Tab::onChangeProgressCallback;
    loaderClient.didFinishProgress = &Tab::onFinishProgressCallback;
    loaderClient.didCommitLoadForFrame = &Tab::onCommitLoadForFrame;
    loaderClient.didReceiveTitleForFrame = &Tab::onReceiveTitleForFrame;
    loaderClient.didFailProvisionalLoadWithErrorForFrame = &Tab::onFailProvisionalLoadWithErrorForFrameCallback;

    WKPageSetPageLoaderClient(m_page, &loaderClient);

    WKPageUIClient uiClient;
    memset(&uiClient, 0, sizeof(WKPageUIClient));
    uiClient.version = kWKPageUIClientCurrentVersion;
    uiClient.clientInfo = this;
    uiClient.createNewPage = &Tab::createNewPageCallback;
    uiClient.mouseDidMoveOverElement = &Tab::onMouseDidMoveOverElement;

    WKPageSetPageUIClient(m_page, &uiClient);
}

Tab::~Tab()
{
    WKPageTerminate(m_page);
    WKRelease(m_context);

    WKRelease(m_view);
}

void Tab::onStartProgressCallback(WKPageRef, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);
    postToBundle(self->m_browser->ui(), "progressStarted", self->m_id);
}

void Tab::onChangeProgressCallback(WKPageRef, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);
    postToBundle(self->m_browser->ui(), "progressChanged", self->m_id, WKPageGetEstimatedProgress(self->m_page));
}

void Tab::onFinishProgressCallback(WKPageRef, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);
    postToBundle(self->m_browser->ui(), "progressFinished", self->m_id);
}

void Tab::onCommitLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef, const void *clientInfo)
{
    Tab* self = ((Tab*)clientInfo);

    if (page != self->m_page || !WKFrameIsMainFrame(frame))
        return;

    WKURLRef url = WKPageCopyActiveURL(page);
    WKStringRef urlString = WKURLCopyString(url);
    postToBundle(self->m_browser->ui(), "urlChanged", self->m_id, urlString);
    WKRelease(url);
    WKRelease(urlString);
}

void Tab::onViewNeedsDisplayCallback(WKViewRef, WKRect, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);
    // FIXME: Only do this is the tab is visible!
    self->m_browser->scheduleUpdateDisplay();
}

void Tab::onReceiveTitleForFrame(WKPageRef page, WKStringRef title, WKFrameRef frame, WKTypeRef, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);

    if (page != self->m_page || !WKFrameIsMainFrame(frame))
        return;

    postToBundle(self->m_browser->ui(), "titleChanged", self->m_id, title);
}

void Tab::onFailProvisionalLoadWithErrorForFrameCallback(WKPageRef page, WKFrameRef frame, WKErrorRef error, WKTypeRef, const void*)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    WKStringRef wkErrorDescription = WKErrorCopyLocalizedDescription(error);
    WKPageLoadPlainTextString(page, wkErrorDescription);
    WKRelease(wkErrorDescription);
}

WKPageRef Tab::createNewPageCallback(WKPageRef, WKURLRequestRef urlRequest, WKDictionaryRef, WKEventModifiers, WKEventMouseButton, const void* clientInfo)
{
    WKURLRef urlRef = WKURLRequestCopyURL(urlRequest);
    assert(urlRef);

    Tab* self = ((Tab*)clientInfo);
    WKStringRef urlStr = WKURLCopyString(urlRef);
    Tab* newTab = self->m_browser->requestTab();
    newTab->loadUrl(fromWK<std::string>(urlStr));

    WKRelease(urlStr);
    WKRelease(urlRef);

    // FIXME: This must return newTab->m_page and the tabs must share the same context.
    return 0;
}

void Tab::onMouseDidMoveOverElement(WKPageRef, WKHitTestResultRef hitTestResult, WKEventModifiers, WKTypeRef, const void *clientInfo)
{
    Tab* self = ((Tab*)clientInfo);

    WKURLRef url = WKHitTestResultCopyAbsoluteLinkURL(hitTestResult);
    if (url) {
        self->m_browser->window()->setMouseCursor(DesktopWindow::Hand);
        WKRelease(url);
    } else {
        self->m_browser->window()->setMouseCursor(DesktopWindow::Arrow);
    }
}

void Tab::setSize(WKSize size)
{
    WKViewSetSize(m_view, size);
}

void Tab::sendKeyEvent(NIXKeyEvent* event)
{
    NIXViewSendKeyEvent(m_view, event);
}

template<>
void Tab::sendMouseEvent<NIXWheelEvent*>(NIXWheelEvent* event)
{
    NIXViewSendWheelEvent(m_view, event);
}

template<>
void Tab::sendMouseEvent<NIXMouseEvent*>(NIXMouseEvent* event)
{
    NIXViewSendMouseEvent(m_view, event);
}

void Tab::setViewportTranslation(int left, int top)
{
    WKViewSetUserViewportTranslation(m_view, left, top);
}

static bool hasValidPrefix(const std::string& url)
{
    const char* validPrefixes[] = {"http://" , "https://", "file://", "ftp://", 0};
    for (const char* prefix : validPrefixes) {
        if (url.find(prefix) == 0)
            return true;
    }
    return false;
}

void Tab::loadUrl(const std::string& url)
{
    std::string fixedUrl(url);
    if (!hasValidPrefix(fixedUrl)) {
        if (std::ifstream(fixedUrl.c_str()))
            fixedUrl.insert(0, "file:///");
        else
            fixedUrl.insert(0, "http://");
    }

    std::cout << "Load URL: " << fixedUrl << std::endl;
    WKURLRef wkUrl = WKURLCreateWithUTF8CString(fixedUrl.c_str());
    WKPageLoadURL(m_page, wkUrl);
    WKRelease(wkUrl);
}

void Tab::back()
{
    if (WKPageCanGoBack(m_page))
        WKPageGoBack(m_page);
}

void Tab::forward()
{
    if (WKPageCanGoForward(m_page))
        WKPageGoForward(m_page);
}

void Tab::reload()
{
    WKPageReload(m_page);
}
