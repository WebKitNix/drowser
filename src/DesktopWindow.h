#ifndef DesktopWindow_h
#define DesktopWindow_h

#include <WebKit2/WKGeometry.h>
#include <NixEvents.h>

class DesktopWindowClient
{
public:
    virtual void onWindowExpose() = 0;
    virtual void onKeyPress(Nix::KeyEvent*) = 0;
    virtual void onKeyRelease(Nix::KeyEvent*) = 0;
    virtual void onMousePress(Nix::MouseEvent*) = 0;
    virtual void onMouseRelease(Nix::MouseEvent*) = 0;
    virtual void onMouseMove(Nix::MouseEvent*) = 0;
    virtual void onMouseWheel(Nix::WheelEvent*) = 0;

    virtual void onWindowSizeChange(WKSize) = 0;
    virtual void onWindowClose() = 0;
};

class DesktopWindow
{
public:
    virtual ~DesktopWindow();

    static DesktopWindow* create(DesktopWindowClient* client, int width, int height);

    WKSize size() const { return m_size; }

    virtual void makeCurrent() = 0;
    virtual void swapBuffers() = 0;
protected:
    DesktopWindowClient* m_client;
    WKSize m_size;

    DesktopWindow(DesktopWindowClient* client, int width, int height);
};

#endif
