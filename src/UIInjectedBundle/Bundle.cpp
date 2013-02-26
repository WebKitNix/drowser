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

#include "Bundle.h"
#include <WebKit2/WKBundle.h>
#include <WebKit2/WKBundleFrame.h>
#include <WebKit2/WKBundlePage.h>
#include <WebKit2/WKBundleInitialize.h>
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKStringPrivate.h>
#include <WebKit2/WKType.h>
#include <WebKit2/WKArray.h>
#include "WKConversions.h"
#include <cstdio>
#include <cstring>
#include <cassert>
#include <iostream>
#include <string>

// I don't care about windows or gcc < 4.x right now.
#define UIBUNDLE_EXPORT __attribute__ ((visibility("default")))

static Bundle* gBundle = 0;

extern "C" {
UIBUNDLE_EXPORT void WKBundleInitialize(WKBundleRef bundle, WKTypeRef initializationUserData)
{
    gBundle = new Bundle(bundle);
}
} // "extern C"

Bundle::Bundle(WKBundleRef bundle)
    : m_bundle(bundle)
    , m_jsContext(0)
    , m_windowObj(0)
{
    WKBundleClient client;
    std::memset(&client, 0, sizeof(WKBundleClient));

    client.version = kWKBundleClientCurrentVersion;
    client.clientInfo = this;
    client.didCreatePage = &Bundle::didCreatePage;
    client.didReceiveMessageToPage = &Bundle::didReceiveMessageToPage;

    WKBundleSetClient(bundle, &client);
}

void Bundle::didClearWindowForFrame(WKBundlePageRef page, WKBundleFrameRef frame, WKBundleScriptWorldRef world, const void *clientInfo)
{
    JSGlobalContextRef context = WKBundleFrameGetJavaScriptContextForWorld(frame, world);

    Bundle* bundle = ((Bundle*)clientInfo);
    bundle->m_jsContext = context;
    bundle->m_windowObj = JSContextGetGlobalObject(context);

    bundle->registerAPI();
    WKBundlePostMessage(bundle->m_bundle, (WKStringRef)toWK("didUiReady"), 0);
}

void Bundle::didCreatePage(WKBundleRef, WKBundlePageRef page, const void* clientInfo)
{
    WKBundlePageLoaderClient loaderClient;
    std::memset(&loaderClient, 0, sizeof(WKBundlePageLoaderClient));
    loaderClient.version = kWKBundlePageLoaderClientCurrentVersion;
    loaderClient.clientInfo = clientInfo;
    loaderClient.didClearWindowObjectForFrame = &Bundle::didClearWindowForFrame;

    WKBundlePageSetPageLoaderClient(page, &loaderClient);

    WKBundlePageUIClient uiClient;
    std::memset(&uiClient, 0, sizeof(WKBundlePageUIClient));
    uiClient.version = kWKBundlePageUIClientCurrentVersion;
    uiClient.willRunJavaScriptAlert = &Bundle::willRunJavaScriptAlert;
    WKBundlePageSetUIClient(page, &uiClient);
}

void Bundle::didReceiveMessageToPage(WKBundleRef, WKBundlePageRef, WKStringRef name, WKTypeRef messageBody, const void*)
{
    gBundle->callJSFunction(WKStringCopyJSString(name), gBundle->toJSVector(messageBody));
}

void Bundle::registerAPI()
{
    assert(m_jsContext);

    const char* funcs[] = {
        "_addTab",
        "_closeTab",
        "_toolBarHeightChanged",
        "_loadUrl",
        "_setCurrentTab",
        "_back",
        "_forward",
        0
    };
    for (int i = 0; funcs[i]; ++i)
        registerJSFunction(funcs[i]);
}

void Bundle::registerJSFunction(const char* name)
{
    JSStringRef funcName = JSStringCreateWithUTF8CString(name);

    JSObjectRef jsFunc = JSObjectMakeFunctionWithCallback(m_jsContext, funcName, jsGenericCallback);
    JSObjectSetProperty(m_jsContext, m_windowObj, funcName, jsFunc, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, 0);
    JSStringRelease(funcName);
}

void Bundle::callJSFunction(JSStringRef name, const std::vector<JSValueRef>& args)
{
    JSValueRef rawFunc = JSObjectGetProperty(m_jsContext, m_windowObj, name, 0);
    if (JSValueIsUndefined(m_jsContext, rawFunc)) {
        char buffer[64];
        JSStringGetUTF8CString(name, buffer, sizeof(buffer));
        std::cerr << "Can't find JS function " << buffer << std::endl;
        return;
    }
    JSObjectRef func = JSValueToObject(m_jsContext, rawFunc, 0);
    JSObjectCallAsFunction(m_jsContext, func, m_windowObj, args.size(), args.size() ? args.data() : 0, 0);
}

JSValueRef Bundle::toJS(WKTypeRef wktype)
{
    WKTypeID tid = WKGetTypeID(wktype);
    if (tid == WKDoubleGetTypeID()) {
        return JSValueMakeNumber(gBundle->m_jsContext, WKDoubleGetValue((WKDoubleRef)wktype));
    } else if (tid == WKUInt64GetTypeID()) {
        return JSValueMakeNumber(gBundle->m_jsContext, WKUInt64GetValue((WKUInt64Ref)wktype));
    } else if (tid == WKStringGetTypeID()) {
        JSStringRef str = WKStringCopyJSString((WKStringRef)wktype);
        JSValueRef jsValue = JSValueMakeString(gBundle->m_jsContext, str);
        JSStringRelease(str);
        return jsValue;
    } else {
        std::cerr << "Unknown WKTypeID" << std::endl;
        return 0;
    }
}

std::vector<JSValueRef> Bundle::toJSVector(WKTypeRef wktype)
{
    if (WKGetTypeID(wktype) != WKArrayGetTypeID())
        return { toJS(wktype) };

    WKArrayRef array = (WKArrayRef) wktype;
    int size = WKArrayGetSize(array);
    std::vector<JSValueRef> result(size);

    for (int i = 0; i < size; ++i)
        result[i] = toJS(WKArrayGetItemAtIndex(array, i));

    return result;
}

static WKStringRef JSValueRefToWKStringRef(JSContextRef ctx, JSValueRef value)
{
    JSStringRef str = JSValueToStringCopy(ctx, value, 0);
    WKStringRef result = WKStringCreateWithJSString(str);
    JSStringRelease(str);
    return result;
}

JSValueRef Bundle::jsGenericCallback(JSContextRef ctx, JSObjectRef func, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef*) {
    WKTypeRef param = 0;
    if (argumentCount == 1) {
        JSType type = JSValueGetType(ctx, arguments[0]);
        switch(type) {
            case kJSTypeNumber:
                param = WKUInt64Create(JSValueToNumber(ctx, arguments[0], 0));
                break;
            case kJSTypeString:
                param = JSValueRefToWKStringRef(ctx, arguments[0]);
                break;
            case kJSTypeObject:
            case kJSTypeNull:
            case kJSTypeUndefined:
            default:
                break;
        }
    }

    JSStringRef propName = JSStringCreateWithUTF8CString("name");
    JSValueRef propValue = JSObjectGetProperty(ctx, func, propName, 0);
    JSStringRelease(propName);

    WKStringRef funcName = JSValueRefToWKStringRef(ctx, propValue);
    WKBundlePostMessage(gBundle->m_bundle, funcName, param);
    if (param)
        WKRelease(param);
    WKRelease(funcName);

    return JSValueMakeNull(ctx);
}

void Bundle::willRunJavaScriptAlert(WKBundlePageRef, WKStringRef alertText, WKBundleFrameRef, const void*)
{
    std::string text = fromWK<std::string>(alertText);
    std::cout << "ALERT: " << text << std::endl;
}
