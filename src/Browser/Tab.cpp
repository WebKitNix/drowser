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
#include <cstring>
#include <WebKit2/WKContext.h>
#include <WebKit2/WKError.h>
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKPage.h>
#include <WebKit2/WKFrame.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKURL.h>
#include <WebKit2/WKType.h>
#include <WebKit2/WKPreferences.h>
#include <WebKit2/WKPreferencesPrivate.h>
#include "Browser.h"
#include "InjectedBundleGlue.h"

Tab::Tab(int id, Browser* browser)
    : m_id(id)
    , m_browser(browser)
{
    // FIXME Find a good way to find where the injected bundle is
    WKStringRef wkStr = WKStringCreateWithUTF8CString((getApplicationPath() + "/../ContentsInjectedBundle/libPageBundle.so").c_str());
    m_context = WKContextCreateWithInjectedBundlePath(wkStr);
    WKRelease(wkStr);

    m_view = NIXViewCreate(m_context, browser->contentPageGroup());
    NIXViewInitialize(m_view);
    NIXViewSetFocused(m_view, true);
    NIXViewSetVisible(m_view, true);
    NIXViewSetActive(m_view, true);
    m_page = NIXViewGetPage(m_view);

    NIXViewClient client;
    std::memset(&client, 0, sizeof(NIXViewClient));
    client.version = kNIXViewClientCurrentVersion;
    client.clientInfo = this;
    client.viewNeedsDisplay = &Tab::onViewNeedsDisplayCallback;

    NIXViewSetViewClient(m_view, &client);

    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(WKPageLoaderClient));
    loaderClient.version = kWKPageLoaderClientCurrentVersion;
    loaderClient.clientInfo = this;
    loaderClient.didStartProgress = &Tab::onStartProgressCallback;
    loaderClient.didChangeProgress = &Tab::onChangeProgressCallback;
    loaderClient.didFinishProgress = &Tab::onFinishProgressCallback;
    loaderClient.didReceiveTitleForFrame = &Tab::onReceiveTitleForFrame;
    loaderClient.didFailProvisionalLoadWithErrorForFrame = &Tab::onFailProvisionalLoadWithErrorForFrameCallback;

    WKPageSetPageLoaderClient(m_page, &loaderClient);
}

Tab::~Tab()
{
    NIXViewRelease(m_view);
    // BUG WKPageTerminate always crash after Nix merge ebddb6a20c6e456556560281c7a4279d48f2cacc
    // WKPageTerminate(m_page);
    WKRelease(m_context);
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

void Tab::onViewNeedsDisplayCallback(NIXView, WKRect, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);
    // FIXME: Only do this is the tab is visible!
    self->m_browser->scheduleUpdateDisplay();
}

void Tab::onReceiveTitleForFrame(WKPageRef page, WKStringRef title, WKFrameRef frame, WKTypeRef, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);

    if (page == self->m_page && WKFrameIsMainFrame(frame))
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

void Tab::setSize(WKSize size)
{
    NIXViewSetSize(m_view, size);
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

void Tab::setViewportTransformation(NIXMatrix* matrix)
{
    NIXViewSetUserViewportTransformation(m_view, matrix);
}

void Tab::loadUrl(const std::string& url)
{
    printf("Load URL: %s\n", url.c_str());
    WKURLRef wkUrl = WKURLCreateWithUTF8CString(url.c_str());
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
