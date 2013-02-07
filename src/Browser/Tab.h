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

#ifndef Tab_h
#define Tab_h

#include <string>
#include <functional>
#include <WebKit2/WKContext.h>
#include <NIXView.h>

class Browser;

class Tab {
public:
    Tab(int id, Browser* browser, WKContextRef context, WKPageGroupRef pageGroup);
    ~Tab();

    // temporary method while things is changing
    NIXView webView() { return m_view; }
    void setSize(WKSize);
    void sendKeyEvent(NIXKeyEvent*);
    template<typename T>
    void sendMouseEvent(T);

    void setViewportTransformation(NIXMatrix*);

    void loadUrl(const std::string& url);
    void back();
    void forward();

private:
    int m_id;
    Browser* m_browser;
    NIXView m_view;
    WKPageRef m_page;

    static void onViewNeedsDisplayCallback(NIXView, WKRect, const void* clientInfo);
    static void onStartProgressCallback(WKPageRef, const void* clientInfo);
    static void onChangeProgressCallback(WKPageRef, const void* clientInfo);
    static void onFinishProgressCallback(WKPageRef, const void* clientInfo);
    static void onReceiveTitleForFrame(WKPageRef page, WKStringRef title, WKFrameRef frame, WKTypeRef userData, const void* clientInfo);
};

#endif