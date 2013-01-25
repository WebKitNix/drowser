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
