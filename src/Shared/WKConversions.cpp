#include "WKConversions.h"
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKArray.h>
#include <string>
#include <vector>

template<>
int fromWK(WKTypeRef value)
{
    return WKUInt64GetValue((WKUInt64Ref)value);
}

template<>
double fromWK(WKTypeRef value)
{
    return WKDoubleGetValue((WKDoubleRef)value);
}

template<>
std::string fromWK(WKTypeRef value)
{
    WKStringRef wkStr = reinterpret_cast<WKStringRef>(value);
    size_t wkStrSize = WKStringGetMaximumUTF8CStringSize(wkStr);
    std::string result;
    result.resize(wkStrSize + 1);
    size_t realSize = WKStringGetUTF8CString(wkStr, &result[0], result.length());
    if (realSize > 0)
        result.resize(realSize - 1);
    return result;
}

template<>
WKTypeRef toWK(const double& value)
{
    return WKDoubleCreate(value);
}

template<>
WKTypeRef toWK(const int& value)
{
    return WKUInt64Create(value);
}

template<>
WKTypeRef toWK(const std::string& value)
{
    return WKStringCreateWithUTF8CString(value.c_str());
}

template<>
WKTypeRef toWK(const std::vector<std::string>& value)
{
    WKTypeRef items[value.size()];
    for (unsigned int i = 0; i < value.size(); ++i) {
        items[i] = toWK(value[i]);
    }
    WKArrayRef result = WKArrayCreate(items, value.size());
    return result;
}
