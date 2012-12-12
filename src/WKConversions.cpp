#include "WKConversions.h"
#include <cstddef>
#include <string>
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
