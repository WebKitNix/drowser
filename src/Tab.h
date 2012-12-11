#ifndef Tab_h
#define Tab_h

#include <string>
#include <WebKit2/WKContext.h>

namespace Nix {
class WebView;
}

class Tab {
public:
    Tab(Nix::WebView* view);
    ~Tab();

    // temporary method while things is changing
    Nix::WebView* webView() { return m_view; }

    void loadUrl(const std::string& url);
    void back();
    void forward();
private:
    Nix::WebView* m_view;
};

#endif
