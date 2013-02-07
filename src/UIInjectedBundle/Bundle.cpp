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

extern "C" {

static Bundle* gBundle = 0;

UIBUNDLE_EXPORT void WKBundleInitialize(WKBundleRef bundle, WKTypeRef initializationUserData)
{
    gBundle = new Bundle(bundle);
}

static JSValueRef jsGenericCallback(JSContextRef ctx, JSObjectRef func, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return gBundle->jsGenericCallback(ctx, func, thisObject, argumentCount, arguments, exception);
}

} // "extern C"

static WKStringRef JSValueRefToWKStringRef(JSContextRef ctx, JSValueRef value)
{
    JSStringRef str = JSValueToStringCopy(ctx, value, 0);
    WKStringRef result = WKStringCreateWithJSString(str);
    JSStringRelease(str);
    return result;
}

static void didClearWindowForFrame(WKBundlePageRef page, WKBundleFrameRef frame, WKBundleScriptWorldRef world, const void *clientInfo)
{
    JSGlobalContextRef context = WKBundleFrameGetJavaScriptContextForWorld(frame, world);

    Bundle* bundle = ((Bundle*)clientInfo);
    bundle->setJSGlobalContext(context);
    bundle->registerAPI();
}

void Bundle::didCreatePage(WKBundleRef, WKBundlePageRef page, const void* clientInfo)
{
    WKBundlePageLoaderClient loaderClient;
    std::memset(&loaderClient, 0, sizeof(WKBundlePageLoaderClient));
    loaderClient.version = kWKBundlePageLoaderClientCurrentVersion;
    loaderClient.clientInfo = clientInfo;
    loaderClient.didClearWindowObjectForFrame = ::didClearWindowForFrame;

    WKBundlePageSetPageLoaderClient(page, &loaderClient);

    WKBundlePageUIClient uiClient;
    std::memset(&uiClient, 0, sizeof(WKBundlePageUIClient));
    uiClient.version = kWKBundlePageUIClientCurrentVersion;
    uiClient.willRunJavaScriptAlert = &Bundle::willRunJavaScriptAlert;
    WKBundlePageSetUIClient(page, &uiClient);
}

Bundle::Bundle(WKBundleRef bundle)
    : m_bundle(bundle)
{
    WKBundleClient client;
    std::memset(&client, 0, sizeof(WKBundleClient));

    client.version = kWKBundleClientCurrentVersion;
    client.clientInfo = this;
    client.didCreatePage = &Bundle::didCreatePage;
    client.didReceiveMessageToPage = &Bundle::didReceiveMessageToPage;

    WKBundleSetClient(bundle, &client);
}

void Bundle::setJSGlobalContext(JSGlobalContextRef context)
{
    m_jsContext = context;
    m_windowObj = JSContextGetGlobalObject(m_jsContext);
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

    JSObjectRef jsFunc = JSObjectMakeFunctionWithCallback(m_jsContext, funcName, ::jsGenericCallback);
    JSObjectSetProperty(m_jsContext, m_windowObj, funcName, jsFunc, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, 0);
    JSStringRelease(funcName);
}

void Bundle::callJSFunction(const char* name, int numArgs, JSValueRef* args)
{
    JSStringRef funcName = JSStringCreateWithUTF8CString(name);
    JSValueRef rawFunc = JSObjectGetProperty(m_jsContext, m_windowObj, funcName, 0);
    JSObjectRef func = JSValueToObject(m_jsContext, rawFunc, 0);
    JSObjectCallAsFunction(m_jsContext, func, m_windowObj, numArgs, numArgs ? args : 0, 0);
    JSStringRelease(funcName);
}

void Bundle::didReceiveMessageToPage(WKBundleRef, WKBundlePageRef, WKStringRef name, WKTypeRef messageBody, const void*)
{
    // FIXME: Put some order on this mess, because it will grow a lot
    if (WKStringIsEqualToUTF8CString(name, "progressChanged")) {
        JSValueRef args[2];
        args[0] = JSValueMakeNumber(gBundle->m_jsContext, fromWK<int>(WKArrayGetItemAtIndex((WKArrayRef)messageBody, 0)));
        args[1] = JSValueMakeNumber(gBundle->m_jsContext, fromWK<double>(WKArrayGetItemAtIndex((WKArrayRef)messageBody, 1)));
        gBundle->callJSFunction("progressChanged", 2, args);
    } else if (WKStringIsEqualToUTF8CString(name, "progressStarted")) {
        JSValueRef arg = JSValueMakeNumber(gBundle->m_jsContext, fromWK<int>(messageBody));
        gBundle->callJSFunction("progressStarted", 1, &arg);
    } else if (WKStringIsEqualToUTF8CString(name, "progressFinished")) {
        JSValueRef arg = JSValueMakeNumber(gBundle->m_jsContext, fromWK<int>(messageBody));
        gBundle->callJSFunction("progressFinished", 1, &arg);
    } else if (WKStringIsEqualToUTF8CString(name, "titleChanged")) {

        int arg0 = fromWK<int>(WKArrayGetItemAtIndex((WKArrayRef)messageBody, 0));
        JSStringRef arg1 = WKStringCopyJSString((WKStringRef)WKArrayGetItemAtIndex((WKArrayRef)messageBody, 1));

        JSValueRef args[2];
        args[0] = JSValueMakeNumber(gBundle->m_jsContext, arg0);
        args[1] = JSValueMakeString(gBundle->m_jsContext, arg1); // We must copy the string to current JS context.

        gBundle->callJSFunction("titleChanged", 2, args);
        JSStringRelease(arg1);
    } else {
        puts("unknow message form Ui");
    }
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
    WKBundlePostMessage(m_bundle, funcName, param);
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
