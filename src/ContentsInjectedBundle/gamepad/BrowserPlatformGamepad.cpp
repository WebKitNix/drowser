#include "BrowserPlatform.h"
#include "Gamepad.h"

#include <cstdio>

void BrowserPlatform::initializeGamepadController()
{
    m_gamepadController = GamepadController::create();
}

void BrowserPlatform::sampleGamepads(Nix::Gamepads& into)
{
    if (!Nix::Platform::current())
        return;

    if (!m_gamepadController)
        return;

    m_gamepadController->sampleGamepads(into);
}
