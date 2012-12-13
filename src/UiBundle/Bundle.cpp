#include "Bundle.h"
#include <WebKit2/WKBundle.h>
#include <WebKit2/WKBundleFrame.h>
#include <WebKit2/WKBundlePage.h>
#include <WebKit2/WKBundleInitialize.h>
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKStringPrivate.h>
#include <WebKit2/WKType.h>
#include <cstdio>
#include <cstring>
#include <cassert>

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

static void didCreatePage(WKBundleRef, WKBundlePageRef page, const void* clientInfo)
{
    WKBundlePageLoaderClient loaderClient;
    std::memset(&loaderClient, 0, sizeof(WKBundlePageLoaderClient));
    loaderClient.version = kWKBundlePageLoaderClientCurrentVersion;
    loaderClient.clientInfo = clientInfo;
    loaderClient.didClearWindowObjectForFrame = ::didClearWindowForFrame;

    WKBundlePageSetPageLoaderClient(page, &loaderClient);
}

static void didReceiveMessageToPage(WKBundleRef bundle, WKBundlePageRef page, WKStringRef name, WKTypeRef messageBody, const void*)
{
    // FIXME: Put some order on this mess, because it will grow a lot
    if (WKStringIsEqualToUTF8CString(name, "setProgressBar")) {
        gBundle->callJSFunction("setProgressBar", messageBody);
    }
}

Bundle::Bundle(WKBundleRef bundle)
    : m_bundle(bundle)
{
    WKBundleClient client;
    std::memset(&client, 0, sizeof(WKBundleClient));

    client.version = kWKBundleClientCurrentVersion;
    client.clientInfo = this;
    client.didCreatePage = ::didCreatePage;
    client.didReceiveMessageToPage = ::didReceiveMessageToPage;

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

void Bundle::callJSFunction(const char* name, WKTypeRef args)
{
    // FIXME: Not all functions will get a double parameter :-)
    double value = WKDoubleGetValue((WKDoubleRef)args);
    JSValueRef jsArg = JSValueMakeNumber(m_jsContext, value);

    JSStringRef funcName = JSStringCreateWithUTF8CString(name);
    JSValueRef rawFunc = JSObjectGetProperty(m_jsContext, m_windowObj, funcName, 0);
    JSObjectRef func = JSValueToObject(m_jsContext, rawFunc, 0);
    JSObjectCallAsFunction(m_jsContext, func, m_windowObj, 1, &jsArg, 0);
    JSStringRelease(funcName);

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
