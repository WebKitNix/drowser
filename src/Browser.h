#ifndef Browser_h
#define Browser_h

#include "DesktopWindow.h"
#include <glib.h>
#include <NIXView.h>
#include <map>
#include <string>

class Tab;

extern "C" {
gboolean callUpdateDisplay(gpointer);
}

class InjectedBundleGlue;

class Browser : public DesktopWindowClient
{
public:
    Browser();
    ~Browser();

    int run();

    // DesktopWindowClient
    virtual void onWindowExpose();
    virtual void onKeyPress(NIXKeyEvent*);
    virtual void onKeyRelease(NIXKeyEvent*);
    virtual void onMousePress(NIXMouseEvent*);
    virtual void onMouseRelease(NIXMouseEvent*);
    virtual void onMouseMove(NIXMouseEvent*);
    virtual void onMouseWheel(NIXWheelEvent*);
    virtual void onWindowSizeChange(WKSize);
    virtual void onWindowClose();

    void addTab(const int& tabId);
    void closeTab(const int& tabId);
    void toolBarHeightChanged(const int& height);
    void setCurrentTab(const int& tabId);
    void loadUrlOnCurrentTab(const std::string& url);
    Tab* currentTab();

    template<typename Param, typename Obj>
    void dispatchMessage(const Param& param, void (Obj::*method)(const Param&));
    template<typename Obj>
    void dispatchMessage(void (Obj::*method)());

    WKPageRef ui() { return m_uiPage; }

    void scheduleUpdateDisplay();
    void progressStarted(Tab*);
    void progressChanged(Tab*, double);
    void progressFinished(Tab*);

private:
    GMainLoop* m_mainLoop;
    bool m_displayUpdateScheduled;
    DesktopWindow* m_window;
    InjectedBundleGlue* m_glue;

    NIXView m_uiView;
    WKPageRef m_uiPage;
    WKContextRef m_uiContext;
    WKPageGroupRef m_uiPageGroup;

    bool m_uiFocused;
    int m_toolBarHeight;
    NIXMatrix m_webViewsTransform;

    std::map<int, Tab*> m_tabs;
    int m_currentTab;
    NIXMatrix m_webTransform;
    WKPageGroupRef m_webPageGroup;
    WKContextRef m_webContext;

    template<typename T>
    bool sendMouseEventToPage(T event);

    void updateDisplay();
    void initUi();

    friend gboolean callUpdateDisplay(gpointer);
};

#endif
