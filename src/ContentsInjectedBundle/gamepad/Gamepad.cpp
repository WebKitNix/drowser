#include "Gamepad.h"

#include <cstdio>
#include <fcntl.h>
#include <linux/joystick.h>
#include <stdint.h>
#include <gio/gunixinputstream.h>

class GamepadDevice {
public:
    static GamepadDevice* create(const char* deviceFile);
    ~GamepadDevice();

    void updateForEvent(struct js_event);
    Nix::Platform::GamepadDevice* getGamepadDevice() { return m_gamepadDeviceNix; }

private:
    GamepadDevice(int fd);
    static gboolean readCallback(GObject* pollableStream, gpointer data);

    int m_fileDescriptor;
    Nix::Platform::GamepadDevice* m_gamepadDeviceNix;
    GInputStream* m_inputStream;
    GSource* m_source;
};

GamepadDevice* GamepadDevice::create(const char* deviceFile)
{
    int fd = open(deviceFile, O_RDONLY | O_NONBLOCK);

    if (fd == -1)
        return 0;

    return new GamepadDevice(fd);
}

GamepadDevice::GamepadDevice(int fd)
    : m_fileDescriptor(fd)
    , m_gamepadDeviceNix(0)
    , m_inputStream(0)
    , m_source(0)
{
    char deviceName[1024];
    if (ioctl(m_fileDescriptor, JSIOCGNAME(sizeof(deviceName)), deviceName) < 0)
        return;

    uint8_t numberOfAxes;
    uint8_t numberOfButtons;
    if (ioctl(m_fileDescriptor, JSIOCGAXES, &numberOfAxes) < 0 || ioctl(m_fileDescriptor, JSIOCGBUTTONS, &numberOfButtons) < 0)
        return;

    m_gamepadDeviceNix = new Nix::Platform::GamepadDevice;
    m_gamepadDeviceNix->id = std::string(deviceName);
    m_gamepadDeviceNix->connected = false;
    m_gamepadDeviceNix->lastTimestamp = 0;
    std::vector<float> axes(numberOfAxes, 0.0);
    std::vector<float> buttons(numberOfButtons, 0.0);
    m_gamepadDeviceNix->axes = axes;
    m_gamepadDeviceNix->buttons = buttons;

    m_inputStream = g_unix_input_stream_new(m_fileDescriptor, FALSE);
    m_source = g_pollable_input_stream_create_source(G_POLLABLE_INPUT_STREAM(m_inputStream), 0);
    g_source_set_callback(m_source, reinterpret_cast<GSourceFunc>(readCallback), this, 0);
    g_source_attach(m_source, 0);
}

GamepadDevice::~GamepadDevice()
{
    close(m_fileDescriptor);

    if (m_source)
        g_source_destroy(m_source);

    if (m_inputStream)
        g_input_stream_close(m_inputStream, 0, 0);

    if (m_gamepadDeviceNix)
        delete m_gamepadDeviceNix;
}

gboolean GamepadDevice::readCallback(GObject* pollableStream, gpointer data)
{
    GamepadDevice* gamepadDevice = reinterpret_cast<GamepadDevice*>(data);

    GError* error = 0;
    struct js_event event;

    gssize len = g_pollable_input_stream_read_nonblocking(G_POLLABLE_INPUT_STREAM(pollableStream),
                                                          &event, sizeof(event), 0, &error);

    if (error)
        return g_error_matches(error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);

    gamepadDevice->updateForEvent(event);

    delete error;
    return TRUE;
}

static float normalizeAxisValue(short value)
{
    // Normalize from range [-32767, 32767] into range [-1.0, 1.0].
    // From GamepadDeviceLinux.cpp (WebCore).
    return value / 32767.0f;
}

static float normalizeButtonValue(short value)
{
    // Normalize from range [0, 1] into range [0.0, 1.0].
    // From GamepadDeviceLinux.cpp (WebCore).
    return value / 1.0f;
}

void GamepadDevice::updateForEvent(struct js_event event)
{
    if (!(event.type & JS_EVENT_AXIS || event.type & JS_EVENT_BUTTON))
        return;

    // Mark the device as connected only if it is not yet connected, the event is not an initialization
    // and the value is not 0 (indicating a genuine interaction with the device).

    if (!m_gamepadDeviceNix->connected && !(event.type & JS_EVENT_INIT) && event.value)
        m_gamepadDeviceNix->connected = true;

    if (event.type & JS_EVENT_AXIS)
        m_gamepadDeviceNix->axes[event.number] = normalizeAxisValue(event.value);
    else if (event.type & JS_EVENT_BUTTON)
        m_gamepadDeviceNix->buttons[event.number] = normalizeButtonValue(event.value);

    m_gamepadDeviceNix->lastTimestamp = event.time;
}

GamepadsHandler::GamepadsHandler()
    : m_gamepads(MAX_GAMEPAD_DEVICES)
{
    m_udev = udev_new();
    m_gamepadsMonitor = udev_monitor_new_from_netlink(m_udev, "udev");

    udev_monitor_enable_receiving(m_gamepadsMonitor);
    udev_monitor_filter_add_match_subsystem_devtype(m_gamepadsMonitor, "input", 0);

    GIOChannel *channel = g_io_channel_unix_new(udev_monitor_get_fd(m_gamepadsMonitor));
    g_io_add_watch(channel, GIOCondition(G_IO_IN), static_cast<GIOFunc>(&GamepadsHandler::onGamepadChange), this);
    g_io_channel_unref(channel);

    struct udev_enumerate* enumerate = udev_enumerate_new(m_udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_add_match_property(enumerate, "ID_INPUT_JOYSTICK", "1");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry* cur;
    struct udev_list_entry* devs = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(cur, devs)
    {
        const char* devname = udev_list_entry_get_name(cur);
        struct udev_device* device = udev_device_new_from_syspath(m_udev, devname);
        if (isGamepadDevice(device))
            registerDevice(udev_device_get_devnode(device));
        udev_device_unref(device);
    }
    udev_enumerate_unref(enumerate);
}

GamepadsHandler::~GamepadsHandler()
{
    udev_unref(m_udev);
    udev_monitor_unref(m_gamepadsMonitor);

    // FIXME: perhaps the correct approach here would to check if the current gamepadClient
    // is this handler or not.
    Nix::Platform::setGamepadClient(0);

    for (unsigned i = 0; i <= m_gamepads.size(); i++)
        delete m_gamepads[i];
    m_gamepads.clear();
}

void GamepadsHandler::registerDevice(const char* deviceFile)
{
    for (unsigned index = 0; index < m_gamepads.size(); index++) {
        if (!m_gamepads[index]) {
            m_gamepads[index] = GamepadDevice::create(deviceFile);
            m_deviceMap.insert(std::pair<std::string, unsigned>(std::string(deviceFile), index));
            break;
        }
    }
}

void GamepadsHandler::unregisterDevice(const char* deviceFile)
{
    std::string key(deviceFile);
    std::map<std::string, unsigned>::iterator it;

    if ((it = m_deviceMap.find(key)) == m_deviceMap.end())
        return;

    unsigned index = m_deviceMap[key];

    m_deviceMap.erase(it);

    delete m_gamepads[index];
    m_gamepads[index] = 0;
}

gboolean GamepadsHandler::onGamepadChange(GIOChannel* source, GIOCondition condition, gpointer data)
{
    GamepadsHandler* handler = static_cast<GamepadsHandler*>(data);

    if (!handler->m_gamepadsMonitor)
        return TRUE;

    struct udev_device* device = udev_monitor_receive_device(handler->m_gamepadsMonitor);

    if (!isGamepadDevice(device))
        return TRUE;

    const char* deviceFile = udev_device_get_devnode(device);
    const char* action = udev_device_get_action(device);

    if (!g_strcmp0(action, "add"))
        handler->registerDevice(deviceFile);
    else if (!g_strcmp0(action, "remove"))
        handler->unregisterDevice(deviceFile);

    return TRUE;
}

bool GamepadsHandler::isGamepadDevice(struct udev_device* device)
{
    const char* deviceFile = udev_device_get_devnode(device);
    const char* sysfsPath = udev_device_get_syspath(device);

    if (!deviceFile || !sysfsPath)
        return FALSE;

    if (!udev_device_get_property_value(device, "ID_INPUT") || !udev_device_get_property_value(device, "ID_INPUT_JOYSTICK"))
        return FALSE;

    return g_str_has_prefix(deviceFile, "/dev/input/js");
}

Nix::Platform::GamepadDevice* GamepadsHandler::getGamepad(int index)
{
    if (index < 0 || index >= m_gamepads.size() || !m_gamepads[index])
        return 0;

    return m_gamepads[index]->getGamepadDevice();
}
