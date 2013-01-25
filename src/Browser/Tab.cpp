#include "Tab.h"
#include <iostream>
#include <cstring>
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKPage.h>
#include <WebKit2/WKFrame.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKURL.h>
#include <WebKit2/WKType.h>
#include "Browser.h"
#include "InjectedBundleGlue.h"

Tab::Tab(int id, Browser* browser, WKContextRef context, WKPageGroupRef pageGroup)
    : m_id(id)
    , m_browser(browser)
{
    m_view = NIXViewCreate(context, pageGroup);
    NIXViewInitialize(m_view);
    NIXViewSetFocused(m_view, true);
    NIXViewSetVisible(m_view, true);
    NIXViewSetActive(m_view, true);
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
    loaderClient.didReceiveTitleForFrame = &Tab::onReceiveTitleForFrame;

    WKPageSetPageLoaderClient(m_page, &loaderClient);
}

Tab::~Tab()
{
    NIXViewRelease(m_view);
}

void Tab::onStartProgressCallback(WKPageRef, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);
    postToBundle(self->m_browser->ui(), "progressStarted", self->m_id);
}

void Tab::onChangeProgressCallback(WKPageRef, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);
    postToBundle(self->m_browser->ui(), "progressChanged", self->m_id, WKPageGetEstimatedProgress(self->m_page));
}

void Tab::onFinishProgressCallback(WKPageRef, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);
    postToBundle(self->m_browser->ui(), "progressFinished", self->m_id);
}

void Tab::onViewNeedsDisplayCallback(NIXView, WKRect, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);
    // FIXME: Only do this is the tab is visible!
    self->m_browser->scheduleUpdateDisplay();
}

void Tab::onReceiveTitleForFrame(WKPageRef page, WKStringRef title, WKFrameRef frame, WKTypeRef, const void* clientInfo)
{
    Tab* self = ((Tab*)clientInfo);

    if (page == self->m_page && WKFrameIsMainFrame(frame))
        postToBundle(self->m_browser->ui(), "titleChanged", self->m_id, title);
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

void Tab::setViewportTransformation(NIXMatrix* matrix)
{
    NIXViewSetUserViewportTransformation(m_view, matrix);
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
