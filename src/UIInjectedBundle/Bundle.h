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

#ifndef Bundle_h
#define Bundle_h

#include <WebKit2/WKBundle.h>

class Bundle
{
public:
    Bundle(WKBundleRef);

    void setJSGlobalContext(JSGlobalContextRef context);
    void registerAPI();

    void callJSFunction(const char *name, int numArgs = 0, JSValueRef* args = 0);
    JSValueRef jsGenericCallback(JSContextRef ctx, JSObjectRef func, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef*);

private:
    WKBundleRef m_bundle;
    JSGlobalContextRef m_jsContext;
    JSObjectRef m_windowObj;

    void registerJSFunction(const char* name);

    // Bundle client
    static void didCreatePage(WKBundleRef, WKBundlePageRef page, const void* clientInfo);
    static void didReceiveMessageToPage(WKBundleRef bundle, WKBundlePageRef page, WKStringRef name, WKTypeRef messageBody, const void*);

    // uiClient
    static void willRunJavaScriptAlert(WKBundlePageRef page, WKStringRef alertText, WKBundleFrameRef frame, const void *clientInfo);
};

#endif
