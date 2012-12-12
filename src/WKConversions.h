#ifndef WKConversions_h
#define WKConversions_h

#include <WebKit2/WKType.h>

template<typename T>
T fromWK(WKTypeRef);

template<typename T>
WKTypeRef toWK(const T&);

#endif
