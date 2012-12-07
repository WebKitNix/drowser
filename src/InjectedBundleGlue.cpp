#include "InjectedBundleGlue.h"
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKString.h>

template<>
int convertWKType<int>(WKTypeRef value)
{
    return WKUInt64GetValue((WKUInt64Ref)value);
}

template<>
std::string convertWKType(WKTypeRef value)
{
    WKStringRef wkStr = reinterpret_cast<WKStringRef>(value);
    size_t wkStrSize = WKStringGetMaximumUTF8CStringSize(wkStr);
    char* buffer = new char[wkStrSize + 1];
    WKStringGetUTF8CString(wkStr, buffer, wkStrSize);
    std::string result(buffer);
    delete[] buffer;
    return result;
}

extern "C" {
static void didReceiveMessageFromInjectedBundle(WKContextRef page, WKStringRef messageName, WKTypeRef messageBody, const void *clientInfo)
{
    InjectedBundleGlue* self = reinterpret_cast<InjectedBundleGlue*>(const_cast<void*>(clientInfo));

    std::string name = convertWKType<std::string>(messageName);
    self->call(name, messageBody);
}
}

InjectedBundleGlue::InjectedBundleGlue(WKContextRef context)
{
    WKContextInjectedBundleClient bundleClient;
    std::memset(&bundleClient, 0, sizeof(bundleClient));
    bundleClient.clientInfo = this;
    bundleClient.version = kWKContextInjectedBundleClientCurrentVersion;
    bundleClient.didReceiveMessageFromInjectedBundle = ::didReceiveMessageFromInjectedBundle;
    WKContextSetInjectedBundleClient(context, &bundleClient);
}

void InjectedBundleGlue::call(const std::string& messageName, WKTypeRef param) const
{
    try {
        m_bindMap.at(messageName)(param);
    } catch (std::out_of_range&) {
        std::cerr << "Unknown message from injected bundle: " << messageName << std::endl;
    }
}

void InjectedBundleGlue::setBind(const char* messageName, std::function<void(WKTypeRef)> callback)
{
    m_bindMap[messageName] = callback;
}

