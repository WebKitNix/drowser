#include "PageBundle.h"
#include "Gamepad.h"

// I don't care about windows or gcc < 4.x right now.
#define UIBUNDLE_EXPORT __attribute__ ((visibility("default")))

extern "C" {

UIBUNDLE_EXPORT void WKBundleInitialize(WKBundleRef bundle, WKTypeRef initializationUserData)
{
    // FIXME: Avoid this leak
    new PageBundle(bundle);
}

}

PageBundle::PageBundle(WKBundleRef bundle)
    : m_bundle(bundle)
{
    m_gamepadsHandler = new GamepadsHandler();
    Nix::Platform::setGamepadClient(m_gamepadsHandler);
}

PageBundle::~PageBundle()
{
    delete m_gamepadsHandler;
}
