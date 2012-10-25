#ifndef XlibEventSource_h
#define XlibEventSource_h

#include <X11/X.h>
#include <X11/Xlib.h>
#include <glib.h>

struct WrappedGSource;

// Integrates Xlib events with the Glib event loop, by using an GSource for the Xlib connection.
class XlibEventSource {
public:
    class Client {
    public:
        virtual void handleXEvent(const XEvent&) = 0;
    };

    XlibEventSource(Display*, Client*);
    ~XlibEventSource();

private:
    friend struct WrappedGSource;

    Display* m_display;
    Client* m_client;
    GPollFD m_pollFD;
    WrappedGSource* m_source;
};

#endif // XlibEventSource_h
