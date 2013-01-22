#ifndef PageBundle_h
#define PageBundle_h

#include <WebKit2/WKBundle.h>

class GamepadsHandler;

class PageBundle
{
public:
    PageBundle(WKBundleRef);
    ~PageBundle();

private:
    WKBundleRef m_bundle;
    GamepadsHandler* m_gamepadsHandler;
};

#endif
