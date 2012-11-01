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

extern "C" {

static JSValueRef addTab(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    assert(argumentCount == 1);
    assert(JSValueIsNumber(context, arguments[0]));

    double tabId = JSValueToNumber(context, arguments[0], 0);
    WKBundlePostMessage(gBundle, WKStringCreateWithUTF8CString("addTab"), WKUInt64Create(tabId));

    return JSValueMakeNull(context);
}

static JSValueRef loadUrl(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    assert(argumentCount == 1);
    assert(JSValueIsString(context, arguments[0]));

    WKStringRef url = WKStringCreateWithJSString(JSValueToStringCopy(context, arguments[0], 0));
    WKBundlePostMessage(gBundle, WKStringCreateWithUTF8CString("loadUrl"), url);

    return JSValueMakeNull(context);
}

static JSValueRef setCurrentTab(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    assert(argumentCount == 1);
    assert(JSValueIsNumber(context, arguments[0]));

    double tabId = JSValueToNumber(context, arguments[0], 0);
    WKBundlePostMessage(gBundle, WKStringCreateWithUTF8CString("setCurrentTab"), WKUInt64Create(tabId));

    return JSValueMakeNull(context);
}

static void registerJSFunction(JSGlobalContextRef context, const char* name, JSObjectCallAsFunctionCallback callback)
{
    JSObjectRef window = JSContextGetGlobalObject(context);
    JSStringRef funcName = JSStringCreateWithUTF8CString(name);
    JSObjectRef jsFunc = JSObjectMakeFunctionWithCallback(context, funcName, callback);
    JSObjectSetProperty(context, window, funcName, jsFunc, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, 0);
}

#define REGISTER_JS_FUNCTION(context, function) registerJSFunction(context, "_" #function, function)

void didClearWindowForFrame(WKBundlePageRef page, WKBundleFrameRef frame, WKBundleScriptWorldRef world, const void *clientInfo)
{
    JSGlobalContextRef context = WKBundleFrameGetJavaScriptContextForWorld(frame, world);

    REGISTER_JS_FUNCTION(context, addTab);
    REGISTER_JS_FUNCTION(context, loadUrl);
    REGISTER_JS_FUNCTION(context, setCurrentTab);
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
