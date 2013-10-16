#include "BrowserPlatform.h"
#include "Gamepad.h"

void BrowserPlatform::sampleGamepads(Nix::Gamepads& into)
{
    static GamepadController gamepadController;
    gamepadController.sampleGamepads(into);
}
