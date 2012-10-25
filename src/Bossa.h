#ifndef Bossa_h
#define Bossa_h

#include "LinuxWindow.h"
#include <glib.h>
#include <WebView.h>
#include <WebKit2/WKContext.h>

namespace Nix {
class WebView;
}

class Bossa : public LinuxWindowClient, public Nix::WebViewClient
{
public:
    Bossa();
    ~Bossa();

    int run();

    // LinuxWindowClient
    virtual void handleExposeEvent();
    virtual void handleKeyPressEvent(const XKeyPressedEvent&) {}
    virtual void handleKeyReleaseEvent(const XKeyReleasedEvent&) {}
    virtual void handleButtonPressEvent(const XButtonPressedEvent&);
    virtual void handleButtonReleaseEvent(const XButtonReleasedEvent&);
    virtual void handlePointerMoveEvent(const XPointerMovedEvent&);
    virtual void handleSizeChanged(int, int);
    virtual void handleClosed();

    // WebViewClient
    virtual void viewNeedsDisplay(int, int, int, int);
    
private:
    GMainLoop* m_mainLoop;
    LinuxWindow* m_window;

    Nix::WebView* m_uiView;
    WKContextRef m_uiContext;
    WKPageGroupRef m_uiPageGroup;
    
    double m_lastClickTime;
    int m_lastClickX;
    int m_lastClickY;
    Nix::MouseEvent::Button m_lastClickButton;
    unsigned m_clickCount;
    
    void updateDisplay();
    void updateClickCount(const XButtonPressedEvent& event);
    void handleWheelEvent(const XButtonPressedEvent& event);
};

#endif
