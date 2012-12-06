#include <WebKit2/WKBundle.h>
#include <WebKit2/WKBundleFrame.h>
#include <WebKit2/WKBundlePage.h>
#include <WebKit2/WKBundleInitialize.h>
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKStringPrivate.h>
#include <WebKit2/WKType.h>
#include <cstdio>
#include <cassert>

// I don't care about windows or gcc < 4.x right now.
#define UIBUNDLE_EXPORT __attribute__ ((visibility("default")))

static WKBundleRef gBundle;

static WKStringRef JSValueRefToWKStringRef(JSContextRef ctx, JSValueRef value)
{
    JSStringRef str = JSValueToStringCopy(ctx, value, 0);
    WKStringRef result = WKStringCreateWithJSString(str);
    JSStringRelease(str);
    return result;
}
extern "C" {

static JSValueRef jsGenericCallback(JSContextRef ctx, JSObjectRef func, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef*)
{
    WKTypeRef param = NULL;
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
    WKBundlePostMessage(gBundle, funcName, param);
    WKRelease(param);
    WKRelease(funcName);

    return JSValueMakeNull(ctx);
}

static void registerJSFunction(JSGlobalContextRef context, const char* name)
{
    JSObjectRef window = JSContextGetGlobalObject(context);
    JSStringRef funcName = JSStringCreateWithUTF8CString(name);

    JSObjectRef jsFunc = JSObjectMakeFunctionWithCallback(context, funcName, jsGenericCallback);
    JSObjectSetProperty(context, window, funcName, jsFunc, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, 0);
    JSStringRelease(funcName);
}

void didClearWindowForFrame(WKBundlePageRef page, WKBundleFrameRef frame, WKBundleScriptWorldRef world, const void *clientInfo)
{
    JSGlobalContextRef context = WKBundleFrameGetJavaScriptContextForWorld(frame, world);

    registerJSFunction(context, "_addTab");
    registerJSFunction(context, "_loadUrl");
    registerJSFunction(context, "_setCurrentTab");
    registerJSFunction(context, "_back");
    registerJSFunction(context, "_forward");
}

void didCreatePage(WKBundleRef bundle, WKBundlePageRef page, const void* clientInfo)
{
    WKBundlePageLoaderClient loaderClient = {
        kWKBundlePageLoaderClientCurrentVersion,
        0,
        0, // didStartProvisionalLoadForFrame,
        0, // didReceiveServerRedirectForProvisionalLoadForFrame,
        0, // didFailProvisionalLoadWithErrorForFrame,
        0, // didCommitLoadForFrame,
        0, // didFinishDocumentLoadForFrame,
        0, // didFinishLoadForFrame,
        0, // didFailLoadWithErrorForFrame,
        0, // didSameDocumentNavigationForFrame,
        0, // didReceiveTitleForFrame,
        0, // didFirstLayoutForFrame
        0, // didFirstVisuallyNonEmptyLayoutForFrame
        0, // didRemoveFrameFromHierarchy
        0, // didDisplayInsecureContentForFrame,
        0, // didRunInsecureContentForFrame,
        didClearWindowForFrame,
        0, // didCancelClientRedirectForFrame,
        0, // willPerformClientRedirectForFrame,
        0, // didHandleOnloadEventsForFrame,
        0, // didLayoutForFrame
        0, // didNewFirstVisuallyNonEmptyLayoutForFrame
        0, // didDetectXSSForFrame,
        0, // shouldGoToBackForwardListItem
        0, // didCreateGlobalObjectForFrame
        0, // willDisconnectDOMWindowExtensionFromGlobalObject
        0, // didReconnectDOMWindowExtensionToGlobalObject
        0, // willDestroyGlobalObjectForDOMWindowExtension
        0, // didFinishProgress, // didFinishProgress
        0, // shouldForceUniversalAccessFromLocalURL
        0, // didReceiveIntentForFrame, // didReceiveIntentForFrame
        0, // registerIntentServiceForFrame, // registerIntentServiceForFrame
        0, // didLayout
    };
    WKBundlePageSetPageLoaderClient(page, &loaderClient);
}

UIBUNDLE_EXPORT void WKBundleInitialize(WKBundleRef bundle, WKTypeRef initializationUserData)
{
    gBundle = bundle;
    WKBundleClient client = {
        kWKBundleClientCurrentVersion,
        0,
        didCreatePage,
        0,
        0, // didInitializePageGroup,
        0, // didReceiveMessage,
        0, // didReceiveMessageToPage
    };
    WKBundleSetClient(bundle, &client);
}
}
