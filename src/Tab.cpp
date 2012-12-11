#include "Tab.h"
#include <iostream>
#include <WebView.h>
#include <WebKit2/WKURL.h>
#include "UiConstants.h"

Tab::Tab(Nix::WebView* view)
    : m_view(view)
{
}

Tab::~Tab()
{
    // TODO: delete webview :-)
    // this will be done when the C-Api land.
}

void Tab::loadUrl(const std::string& url)
{
    printf("Load URL: %s\n", url.c_str());
    WKPageLoadURL(m_view->pageRef(), WKURLCreateWithUTF8CString(url.c_str()));
}

void Tab::back()
{
    WKPageRef page = m_view->pageRef();
    if (WKPageCanGoBack(page))
        WKPageGoBack(page);
}

void Tab::forward()
{
    WKPageRef page = m_view->pageRef();
    if (WKPageCanGoForward(page))
        WKPageGoForward(page);
}
