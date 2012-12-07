#ifndef Bossa_h
#define Bossa_h

#include "DesktopWindow.h"
#include <glib.h>
#include <WebView.h>
#include <WebKit2/WKContext.h>
#include <map>
#include <string>

namespace Nix {
class WebView;
}

extern "C" {
gboolean callUpdateDisplay(gpointer);
}

class InjectedBundleGlue;

class Bossa : public DesktopWindowClient, public Nix::WebViewClient
{
public:
    Bossa();
    ~Bossa();

    int run();

    // DesktopWindowClient
    virtual void onWindowExpose();
    virtual void onKeyPress(Nix::KeyEvent*);
    virtual void onKeyRelease(Nix::KeyEvent*);
    virtual void onMousePress(Nix::MouseEvent*);
    virtual void onMouseRelease(Nix::MouseEvent*);
    virtual void onMouseMove(Nix::MouseEvent*);
    virtual void onMouseWheel(Nix::WheelEvent*);
    virtual void onWindowSizeChange(WKSize);
    virtual void onWindowClose();

    // WebViewClient
    virtual void viewNeedsDisplay(WKRect);
    virtual void webProcessCrashed(WKStringRef url);

    void addTab(int tabId);
    void setCurrentTab(int tabId);
    void loadUrl(std::string);
    void back();
    void forward();
    Nix::WebView* currentTab();

private:
    GMainLoop* m_mainLoop;
    bool m_displayUpdateScheduled;
    DesktopWindow* m_window;
    InjectedBundleGlue* m_glue;

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

    friend gboolean callUpdateDisplay(gpointer);
};

#endif
