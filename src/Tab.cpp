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
    client.viewNeedsDisplay = &Tab::onViewNeedsDisplayCallback;

    NIXViewSetViewClient(m_view, &client);

    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(WKPageLoaderClient));
    loaderClient.version = kWKPageLoaderClientCurrentVersion;
    loaderClient.clientInfo = this;
    loaderClient.didStartProgress = &Tab::onStartProgressCallback;
    loaderClient.didChangeProgress = &Tab::onChangeProgressCallback;
    loaderClient.didFinishProgress = &Tab::onFinishProgressCallback;

    WKPageSetPageLoaderClient(m_page, &loaderClient);
}

Tab::~Tab()
{
    NIXViewRelease(m_view);
}

void Tab::onStartProgressCallback(WKPageRef, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);
    self->m_browser->progressStarted(self);
}

void Tab::onChangeProgressCallback(WKPageRef, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);
    self->m_browser->progressChanged(self, WKPageGetEstimatedProgress(self->m_page));
}

void Tab::onFinishProgressCallback(WKPageRef, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);
    self->m_browser->progressFinished(self);
}

void Tab::onViewNeedsDisplayCallback(NIXView, WKRect, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);
    // FIXME: Only do this is the tab is visible!
    self->m_browser->scheduleUpdateDisplay();
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
