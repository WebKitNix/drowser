#include "Tab.h"
#include <iostream>
#include <cstring>
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKPage.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKURL.h>
#include <WebKit2/WKType.h>
#include "UiConstants.h"
#include "Browser.h"

static void onChangeProgressCallBack(WKPageRef, const void* clientInfo)
{
    ((Tab*)clientInfo)->onChangeProgress();
}


static void onViewNeedsDisplayCallback(NIXView, WKRect, const void* clientInfo)
{
    ((Tab*)clientInfo)->onViewNeedsDisplay();
}

Tab::Tab(Browser* browser, WKContextRef context, WKPageGroupRef pageGroup)
    : m_browser(browser)
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
    client.clientInfo = this;
    client.viewNeedsDisplay = ::onViewNeedsDisplayCallback;

    NIXViewSetViewClient(m_view, &client);

    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(WKPageLoaderClient));
    loaderClient.version = kWKPageLoaderClientCurrentVersion;
    loaderClient.clientInfo = this;
    loaderClient.didChangeProgress = ::onChangeProgressCallBack;

    WKPageSetPageLoaderClient(m_page, &loaderClient);
}

Tab::~Tab()
{
    NIXViewRelease(m_view);
}

void Tab::onViewNeedsDisplay()
{
    // FIXME: Only consider this if the tab is visible
    m_browser->scheduleUpdateDisplay();
}

void Tab::onChangeProgress()
{
    m_browser->progressChanged(this, 100 * WKPageGetEstimatedProgress(m_page));
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
    WKURLRef wkUrl = WKURLCreateWithUTF8CString(url.c_str());
    WKPageLoadURL(m_page, wkUrl);
    WKRelease(wkUrl);
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
