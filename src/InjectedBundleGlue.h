#ifndef InjectedBundleGlue_h
#define InjectedBundleGlue_h

#include <functional>
#include <unordered_map>
#include <string>
#include <WebKit2/WKContext.h>

template<typename T>
T convertWKType(WKTypeRef);

class InjectedBundleGlue
{
public:
    InjectedBundleGlue(WKContextRef);

    template<typename Obj, typename Param>
    void bind(const char* messageName, Obj* obj, void (Obj::*method)(Param))
    {
        setBind(messageName, [obj,method](WKTypeRef msgBody) {
            (obj->*method)(convertWKType<Param>(msgBody));
        });
    }

    template<typename Obj>
    void bind(const char* messageName, Obj* obj, void (Obj::*method)())
    {
        setBind(messageName, [obj,method](WKTypeRef msgBody) {
            (obj->*method)();
        });
    }

    void call(const std::string& messageName, WKTypeRef param) const;

private:

    void setBind(const char* messageName, std::function<void(WKTypeRef)> callback);

    typedef std::unordered_map<std::string, std::function<void(WKTypeRef)>> BindMap;
    BindMap m_bindMap;
};

#endif
