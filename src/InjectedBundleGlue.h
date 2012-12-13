#ifndef InjectedBundleGlue_h
#define InjectedBundleGlue_h

#include <functional>
#include <unordered_map>
#include <string>
#include <WebKit2/WKContext.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKType.h>
#include <WebKit2/WKPage.h>

template<typename T>
T fromWK(WKTypeRef);

template<typename T>
WKTypeRef toWK(const T&);

template<typename T>
static void postToBundle(WKPageRef page, const char* message, const T& value)
{
    WKStringRef wkMessage = WKStringCreateWithUTF8CString(message);
    WKPagePostMessageToInjectedBundle(page, wkMessage, toWK(value));
    WKRelease(wkMessage);
}

void postToBundle(WKPageRef page, const char* message);

class InjectedBundleGlue
{
public:
    InjectedBundleGlue(WKContextRef);

    template<typename Obj, typename Param>
    void bind(const char* messageName, Obj* obj, void (Obj::*method)(const Param&))
    {
        m_bindMap[messageName] = [obj,method](WKTypeRef msgBody) {
            (obj->*method)(fromWK<Param>(msgBody));
        };
    }

    template<typename Obj>
    void bind(const char* messageName, Obj* obj, void (Obj::*method)())
    {
        m_bindMap[messageName] = [obj,method](WKTypeRef msgBody) {
            (obj->*method)();
        };
    }

    template<typename Obj, typename ObjReceiver, typename Param>
    void bindToDispatcher(const char* messageName, Obj* obj, void (ObjReceiver::*method)(const Param&))
    {
        m_bindMap[messageName] = [obj,method](WKTypeRef msgBody) {
            (obj->dispatchMessage)(fromWK<Param>(msgBody), method);
        };
    }

    template<typename Obj, typename ObjReceiver>
    void bindToDispatcher(const char* messageName, Obj* obj, void (ObjReceiver::*method)())
    {
        m_bindMap[messageName] = [obj,method](WKTypeRef msgBody) {
            (obj->dispatchMessage)(method);
        };
    }

    void call(const std::string& messageName, WKTypeRef param) const;

private:
    typedef std::unordered_map<std::string, std::function<void(WKTypeRef)> > BindMap;
    BindMap m_bindMap;
};

#endif
