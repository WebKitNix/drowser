#ifndef LinuxWindow_h
#define LinuxWindow_h

#include "XlibEventSource.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <utility>

void fatalError(const char* message);

const int DEFAULT_WIDTH = 800;
const int DEFAULT_HEIGHT = 600;

class LinuxWindowClient {
public:
    virtual ~LinuxWindowClient() { };
    virtual void handleExposeEvent() = 0;
    virtual void handleKeyPressEvent(const XKeyPressedEvent&) = 0;
    virtual void handleKeyReleaseEvent(const XKeyReleasedEvent&) = 0;
    virtual void handleButtonPressEvent(const XButtonPressedEvent&) = 0;
    virtual void handleButtonReleaseEvent(const XButtonReleasedEvent&) = 0;
    virtual void handlePointerMoveEvent(const XPointerMovedEvent&) = 0;

    virtual void handleSizeChanged(int width, int height) = 0;
    virtual void handleClosed() = 0;
};

class LinuxWindow : public XlibEventSource::Client {
public:
    LinuxWindow(LinuxWindowClient*, int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT);
    ~LinuxWindow();

    std::pair<int, int> size() const;

    void makeCurrent();
    void swapBuffers();

    LinuxWindowClient* client() const { return m_client; }
    Display* display() const { return m_display; }

private:
    struct GLContextData;

    VisualID setupXVisualID();
    void createGLContext();
    void destroyGLContext();

    // XlibEventSource::Client.
    virtual void handleXEvent(const XEvent&);

    Window createXWindow(VisualID visualID);
    void updateSizeIfNeeded(int width, int height);

    LinuxWindowClient* m_client;
    XlibEventSource* m_eventSource;

    Display* m_display;
    Window m_window;

    GLContextData* m_glContextData;

    int m_width;
    int m_height;
};

#endif // LinuxWindow_h
