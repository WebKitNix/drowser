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

#include "XlibEventSource.h"

#include "assert.h"

struct WrappedGSource {
    GSource source;
    XlibEventSource* xlibEventSource;

    Display* display() { return xlibEventSource->m_display; }
    XlibEventSource::Client* client() { return xlibEventSource->m_client; }
};

static gboolean eventSourceCheck(GSource* source)
{
    WrappedGSource* wrappedSource = reinterpret_cast<WrappedGSource*>(source);
    return XPending(wrappedSource->display());
}

static gboolean eventSourcePrepare(GSource* source, gint* timeout)
{
    if (timeout)
        *timeout = -1;
    return eventSourceCheck(source);
}

static gboolean eventSourceDispatch(GSource* source, GSourceFunc callback, gpointer user_data)
{
    WrappedGSource* wrappedSource = reinterpret_cast<WrappedGSource*>(source);
    Display* display = wrappedSource->display();

    do {
        XEvent event;
        XNextEvent(display, &event);
        wrappedSource->client()->handleXEvent(event);
    } while (XPending(display));

    if (callback)
        callback(user_data);

    return true;
}

static GSourceFuncs eventSourceFuncs = {
    eventSourcePrepare,
    eventSourceCheck,
    eventSourceDispatch,
    0
};

XlibEventSource::XlibEventSource(Display* display, Client* client)
    : m_display(display)
    , m_client(client)
    , m_source(0)
{
    assert(display);
    assert(client);

    m_pollFD.fd = XConnectionNumber(display);
    m_pollFD.events = G_IO_IN | G_IO_HUP | G_IO_ERR;
    m_pollFD.revents = 0;

    m_source = reinterpret_cast<WrappedGSource*>(g_source_new(&eventSourceFuncs, sizeof(WrappedGSource)));
    m_source->xlibEventSource = this;

    g_source_attach(&m_source->source, 0);
    g_source_add_poll(&m_source->source, &m_pollFD);
}

XlibEventSource::~XlibEventSource()
{
    g_source_remove_poll(&m_source->source, &m_pollFD);
    g_source_destroy(&m_source->source);
}
