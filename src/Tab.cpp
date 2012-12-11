#include "Tab.h"
#include <iostream>
#include <cstring>
#include <WebKit2/WKURL.h>
#include "UiConstants.h"
#include "Browser.h"

Tab::Tab(Browser* browser, WKContextRef context, WKPageGroupRef pageGroup)
{
    static const NIXMatrix webTransform = NIXMatrixMakeTranslation(0, UI_HEIGHT);

    m_view = NIXViewCreate(context, pageGroup);
    NIXViewInitialize(m_view);
    NIXViewSetFocused(m_view, true);
    NIXViewSetVisible(m_view, true);
    NIXViewSetActive(m_view, true);
    NIXViewSetUserViewportTransformation(m_view, &webTransform);
    m_page = NIXViewGetPage(m_view);

    NIXViewClient client;
    std::memset(&client, 0, sizeof(NIXViewClient));
    client.version = kNIXViewClientCurrentVersion;
    client.clientInfo = browser;

    client.viewNeedsDisplay = [](NIXView, WKRect, const void* client) {
        ((Browser*)client)->scheduleUpdateDisplay();
    };

    NIXViewSetViewClient(m_view, &client);
}

Tab::~Tab()
{
    NIXViewRelease(m_view);
}

void Tab::setSize(WKSize size)
{
    NIXViewSetSize(m_view, size);
}

void Tab::sendKeyEvent(NIXKeyEvent* event)
{
    NIXViewSendKeyEvent(m_view, event);
}

template<>
void Tab::sendMouseEvent<NIXWheelEvent*>(NIXWheelEvent* event)
{
    NIXViewSendWheelEvent(m_view, event);
}

template<>
void Tab::sendMouseEvent<NIXMouseEvent*>(NIXMouseEvent* event)
{
    NIXViewSendMouseEvent(m_view, event);
}

void Tab::loadUrl(const std::string& url)
{
    printf("Load URL: %s\n", url.c_str());
    WKPageLoadURL(m_page, WKURLCreateWithUTF8CString(url.c_str()));
}

void Tab::back()
{
    if (WKPageCanGoBack(m_page))
        WKPageGoBack(m_page);
}

void Tab::forward()
{
    if (WKPageCanGoForward(m_page))
        WKPageGoForward(m_page);
}
