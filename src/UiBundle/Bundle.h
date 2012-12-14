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
