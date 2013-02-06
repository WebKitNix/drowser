/*
 * Copyright (C) 2012-2013 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
    std::string result;
    result.resize(wkStrSize + 1);
    size_t realSize = WKStringGetUTF8CString(wkStr, &result[0], result.length());
    if (realSize > 0)
        result.resize(realSize - 1);
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
