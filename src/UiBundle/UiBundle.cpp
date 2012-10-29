#include <WebKit2/WKBundle.h>
#include <WebKit2/WKBundleFrame.h>
#include <WebKit2/WKBundlePage.h>
#include <WebKit2/WKBundleInitialize.h>
#include <cstdio>
#include <cassert>

#if defined _WIN32
    #if UIBUNDLE_EXPORT
        #define LIBSHIBOKEN_API __declspec(dllexport)
    #else
        #ifdef _MSC_VER
            #define UIBUNDLE_EXPORT __declspec(dllimport)
        #endif
    #endif
#elif __GNUC__ >= 4
    #define UIBUNDLE_EXPORT __attribute__ ((visibility("default")))
#else
    #define UIBUNDLE_EXPORT
#endif

JSValueRef addTab(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    assert(argumentCount == 1);
    assert(JSValueIsString(arguments[0]));

    JSStringRef jsStr = JSValueToStringCopy(context, arguments[0], 0);
    char str[512];
    JSStringGetUTF8CString(jsStr, str, sizeof(str)-1);
    JSStringRelease(jsStr);


//     WKBundlePostMessage();

    return thisObject;
}

void didClearWindowForFrame(WKBundlePageRef page, WKBundleFrameRef frame, WKBundleScriptWorldRef world, const void *clientInfo)
{
    JSGlobalContextRef context = WKBundleFrameGetJavaScriptContextForWorld(frame, world);
    JSObjectRef window = JSContextGetGlobalObject(context);

    JSObjectRef addTabFunc = JSObjectMakeFunctionWithCallback(context, JSStringCreateWithUTF8CString("addTab"), addTab);
    JSObjectSetProperty(context, window, JSStringCreateWithUTF8CString("addTab"), addTabFunc, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, 0);
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

void willDestroyPage(WKBundleRef bundle, WKBundlePageRef page, const void* clientInfo)
{
}

extern "C" {
UIBUNDLE_EXPORT void WKBundleInitialize(WKBundleRef bundle, WKTypeRef initializationUserData)
{
    WKBundleClient client = {
        kWKBundleClientCurrentVersion,
        0,
        didCreatePage,
        willDestroyPage,
        0, // didInitializePageGroup,
        0, // didReceiveMessage,
        0, // didReceiveMessageToPage
    };
    WKBundleSetClient(bundle, &client);
}
}
