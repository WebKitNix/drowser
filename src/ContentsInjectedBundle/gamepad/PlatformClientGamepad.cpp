#include "PlatformClient.h"
#include "Gamepad.h"

#include <cstdio>

void PlatformClient::initializeGamepadController()
{
    m_gamepadController = GamepadController::create();
}

void PlatformClient::sampleGamepads(WebKit::WebGamepads& into)
{
    if (!WebKit::Platform::current())
        return;

    if (!m_gamepadController)
        return;

    m_gamepadController->sampleGamepads(into);
}
