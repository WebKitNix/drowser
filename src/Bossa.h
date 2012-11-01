#ifndef Bossa_h
#define Bossa_h

#include "LinuxWindow.h"
#include <glib.h>
#include <WebView.h>
#include <WebKit2/WKContext.h>
#include <map>

namespace Nix {
class WebView;
}

extern "C" {
gboolean callUpdateDisplay(gpointer);
}

class Bossa : public LinuxWindowClient, public Nix::WebViewClient
{
public:
    Bossa();
    ~Bossa();

    int run();

    // LinuxWindowClient
    // TODO: Move X stuff away from here and remove LinuxWindowClient inheritance
    virtual void handleExposeEvent();
    virtual void handleKeyPressEvent(const XKeyPressedEvent&);
    virtual void handleKeyReleaseEvent(const XKeyReleasedEvent&);
    virtual void handleButtonPressEvent(const XButtonPressedEvent&);
    virtual void handleButtonReleaseEvent(const XButtonReleasedEvent&);
    virtual void handlePointerMoveEvent(const XPointerMovedEvent&);
    virtual void handleSizeChanged(int, int);
    virtual void handleClosed();

    // WebViewClient
    virtual void viewNeedsDisplay(int, int, int, int);
    virtual void webProcessCrashed(WKStringRef url);

    void addTab(int tabId);
    void setCurrentTab(int tabId);
    void loadUrl(const char*);
    void back();
    void forward();
    Nix::WebView* currentTab();

private:
    GMainLoop* m_mainLoop;
    bool m_displayUpdateScheduled;
    LinuxWindow* m_window;

    Nix::WebView* m_uiView;
    WKContextRef m_uiContext;
    WKPageGroupRef m_uiPageGroup;

    double m_lastClickTime;
    int m_lastClickX;
    int m_lastClickY;
    Nix::MouseEvent::Button m_lastClickButton;
    unsigned m_clickCount;


    std::map<int, Nix::WebView*> m_tabs;
    int m_currentTab;
    cairo_matrix_t m_webTransform;
    WKPageGroupRef m_webPageGroup;
    WKContextRef m_webContext;

    void scheduleUpdateDisplay();
    void updateDisplay();
    void initUi();
    void updateClickCount(const XButtonPressedEvent& event);
    void handleWheelEvent(const XButtonPressedEvent& event);

    friend gboolean callUpdateDisplay(gpointer);
};

#endif
