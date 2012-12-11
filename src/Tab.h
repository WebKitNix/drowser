#ifndef Tab_h
#define Tab_h

#include <string>
#include <functional>
#include <WebKit2/WKContext.h>
#include <NIXView.h>

class Browser;

class Tab {
public:
    Tab(Browser* browser, WKContextRef context, WKPageGroupRef pageGroup);
    ~Tab();

    // temporary method while things is changing
    NIXView webView() { return m_view; }
    void setSize(WKSize);
    void sendKeyEvent(NIXKeyEvent*);
    template<typename T>
    void sendMouseEvent(T);

    void loadUrl(const std::string& url);
    void back();
    void forward();

private:
    NIXView m_view;
    WKPageRef m_page;
};

#endif
