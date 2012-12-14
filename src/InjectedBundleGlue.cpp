#include "InjectedBundleGlue.h"
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKString.h>

template<>
int fromWK<int>(WKTypeRef value)
{
    return WKUInt64GetValue((WKUInt64Ref)value);
}

template<>
std::string fromWK(WKTypeRef value)
{
    WKStringRef wkStr = reinterpret_cast<WKStringRef>(value);
    size_t wkStrSize = WKStringGetMaximumUTF8CStringSize(wkStr);
    char* buffer = new char[wkStrSize + 1];
    WKStringGetUTF8CString(wkStr, buffer, wkStrSize);
    std::string result(buffer);
    delete[] buffer;
    return result;
}

template<>
WKTypeRef toWK<double>(const double& value)
{
    return WKDoubleCreate(value);
}

template<>
WKTypeRef toWK<int>(const int& value)
{
    return WKUInt64Create(value);
}

extern "C" {
static void didReceiveMessageFromInjectedBundle(WKContextRef page, WKStringRef messageName, WKTypeRef messageBody, const void *clientInfo)
{
    InjectedBundleGlue* self = reinterpret_cast<InjectedBundleGlue*>(const_cast<void*>(clientInfo));

    std::string name = fromWK<std::string>(messageName);
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
