#include "PlatformClient.h"
#include "Gamepad.h"

#include <cstdio>

void PlatformClient::initializeGamepadController()
{
    m_gamepadController = GamepadController::create();
}

void PlatformClient::sampleGamepads(Nix::Gamepads& into)
{
    if (!Nix::Platform::current())
        return;

    if (!m_gamepadController)
        return;

    m_gamepadController->sampleGamepads(into);
}
