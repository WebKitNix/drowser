#ifndef Bundle_h
#define Bundle_h

#include <WebKit2/WKBundle.h>

class Bundle
{
public:
    Bundle(WKBundleRef);

    void setJSGlobalContext(JSGlobalContextRef context);
    void registerAPI();

    void callJSFunction(const char *name, WKTypeRef args);
    JSValueRef jsGenericCallback(JSContextRef ctx, JSObjectRef func, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef*);

private:
    WKBundleRef m_bundle;
    JSGlobalContextRef m_jsContext;
    JSObjectRef m_windowObj;

    void registerJSFunction(const char* name);
};

#endif
