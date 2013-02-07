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

#ifndef InjectedBundleGlue_h
#define InjectedBundleGlue_h

#include <functional>
#include <unordered_map>
#include <string>
#include <WebKit2/WKContext.h>
#include <WebKit2/WKPage.h>
#include <WebKit2/WKMutableArray.h>
#include <WebKit2/WKString.h>
#include "WKConversions.h"

inline WKTypeRef createArg() { return 0; }
inline WKTypeRef createArg(const WKTypeRef& value) { return value; }

inline void _nop(...) {}

template<typename ... T>
WKTypeRef createArg(WKTypeRef first, T...t)
{
    WKMutableArrayRef pack = WKMutableArrayCreate();
    WKArrayAppendItem(pack, first);
    _nop((WKArrayAppendItem(pack, t), 1)...);
    return pack;
}

template<typename ...T>
static void postToBundle(WKPageRef page, const char* message, const T& ... values)
{
    WKStringRef wkMessage = WKStringCreateWithUTF8CString(message);
    WKPagePostMessageToInjectedBundle(page, wkMessage, createArg(toWK(values)...));
    WKRelease(wkMessage);
}

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
