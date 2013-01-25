#ifndef Gamepad_h
#define Gamepad_h

extern "C" {
#include <libudev.h>
}

#include <glib.h>
#include <Platform/NixPlatform.h>

#include <map>

#define MAX_GAMEPAD_DEVICES 4

class GamepadDevice;

class GamepadsHandler : public Nix::Platform::GamepadClient {
public:
    GamepadsHandler();
    ~GamepadsHandler();

    virtual Nix::Platform::GamepadDevice* getGamepad(int index);

private:

    void registerDevice(const char* deviceFile);
    void unregisterDevice(const char* deviceFile);
    static bool isGamepadDevice(struct udev_device*);
    static gboolean onGamepadChange(GIOChannel*, GIOCondition, gpointer);

    std::vector<GamepadDevice*> m_gamepads;
    std::map<std::string, unsigned> m_deviceMap;

    struct udev* m_udev;
    struct udev_monitor* m_gamepadsMonitor;
};

#endif // Gamepad_h
