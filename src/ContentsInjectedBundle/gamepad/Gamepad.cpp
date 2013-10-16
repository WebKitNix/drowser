/*
 * Copyright (C) 2012-2013 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Gamepad.h"

#include <glib.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <linux/joystick.h>
#include <stdint.h>

class GamepadDevice {
public:
    static GamepadDevice* create(const char*, Nix::Gamepad*);
    ~GamepadDevice();

    void updateForEvent(struct js_event);
    Nix::Gamepad* device() { return m_nixGamepad; }

private:
    GamepadDevice(int, Nix::Gamepad*);
    static gboolean readCallback(GObject* pollableStream, gpointer data);

    int m_fileDescriptor;
    Nix::Gamepad* m_nixGamepad;
    GInputStream* m_inputStream;
    GSource* m_source;
};

GamepadDevice* GamepadDevice::create(const char* deviceFile, Nix::Gamepad* gamepad)
{
    int fd = open(deviceFile, O_RDONLY | O_NONBLOCK);

    if (fd == -1)
        return 0;

    return new GamepadDevice(fd, gamepad);
}

GamepadDevice::GamepadDevice(int fd, Nix::Gamepad* gamepad)
    : m_fileDescriptor(fd)
    , m_nixGamepad(gamepad)
    , m_inputStream(0)
    , m_source(0)
{
    memset(m_nixGamepad->id, 0, sizeof(m_nixGamepad->id));
    if (ioctl(m_fileDescriptor, JSIOCGNAME(sizeof(m_nixGamepad->id)), m_nixGamepad->id) < 0)
        return;
    uint8_t numberOfAxes;
    uint8_t numberOfButtons;
    if (ioctl(m_fileDescriptor, JSIOCGAXES, &numberOfAxes) < 0 || ioctl(m_fileDescriptor, JSIOCGBUTTONS, &numberOfButtons) < 0)
        return;

    m_nixGamepad->connected = false;
    m_nixGamepad->timestamp = 0;
    m_nixGamepad->axesLength = std::min<unsigned>(numberOfAxes, Nix::Gamepad::axesLengthCap);
    m_nixGamepad->buttonsLength = std::min<unsigned>(numberOfButtons, Nix::Gamepad::buttonsLengthCap);

    unsigned i, total;
    for (i = 0, total = m_nixGamepad->axesLength; i < total; i++)
        m_nixGamepad->axes[i] = 0;
    for (i = 0, total = m_nixGamepad->buttonsLength; i < total; i++)
        m_nixGamepad->buttons[i] = 0;

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

    // XXX: maybe there is a cleaner way to do this?
    memset(m_nixGamepad, 0, sizeof(Nix::Gamepad));
}

gboolean GamepadDevice::readCallback(GObject* pollableStream, gpointer data)
{
    GamepadDevice* gamepadDevice = reinterpret_cast<GamepadDevice*>(data);

    GError* error = 0;
    struct js_event event;

    g_pollable_input_stream_read_nonblocking(G_POLLABLE_INPUT_STREAM(pollableStream),
                                             &event, sizeof(event), 0, &error);

    if (error) {
        gboolean ret = g_error_matches(error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
        g_error_free(error);
        return ret;
    }

    gamepadDevice->updateForEvent(event);

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

    if (!m_nixGamepad->connected && !(event.type & JS_EVENT_INIT) && event.value)
        m_nixGamepad->connected = true;

    if (event.type & JS_EVENT_AXIS && event.number < Nix::Gamepad::axesLengthCap)
        m_nixGamepad->axes[event.number] = normalizeAxisValue(event.value);
    else if (event.type & JS_EVENT_BUTTON && event.number < Nix::Gamepad::buttonsLengthCap)
        m_nixGamepad->buttons[event.number] = normalizeButtonValue(event.value);

    m_nixGamepad->timestamp = event.time;
}

//-------------- end of GamepadDevice

GamepadController::GamepadController()
    : m_gamepadDevices(Nix::Gamepads::itemsLengthCap)
{
    m_udev = udev_new();
    m_gamepadsMonitor = udev_monitor_new_from_netlink(m_udev, "udev");

    udev_monitor_enable_receiving(m_gamepadsMonitor);
    udev_monitor_filter_add_match_subsystem_devtype(m_gamepadsMonitor, "input", 0);

    GIOChannel *channel = g_io_channel_unix_new(udev_monitor_get_fd(m_gamepadsMonitor));
    g_io_add_watch(channel, GIOCondition(G_IO_IN), static_cast<GIOFunc>(&GamepadController::onGamepadChange), this);
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

GamepadController::~GamepadController()
{
    udev_unref(m_udev);
    udev_monitor_unref(m_gamepadsMonitor);

    for (unsigned i = 0; i <= m_gamepadDevices.size(); i++)
        delete m_gamepadDevices[i];

    m_gamepadDevices.clear();
}

GamepadController* GamepadController::create()
{
    return new GamepadController();
}

void GamepadController::registerDevice(const char* deviceFile)
{
    for (unsigned index = 0; index < m_gamepadDevices.size(); index++) {
        if (!m_gamepadDevices[index]) {
            m_gamepadDevices[index] = GamepadDevice::create(deviceFile, &m_gamepads.items[index]);
            m_deviceMap.insert(std::make_pair(std::string(deviceFile), index));
            break;
        }
    }
}

void GamepadController::unregisterDevice(const char* deviceFile)
{
    std::string key(deviceFile);
    std::map<std::string, unsigned>::iterator it;

    if ((it = m_deviceMap.find(key)) == m_deviceMap.end())
        return;

    unsigned index = m_deviceMap[key];

    m_deviceMap.erase(it);

    delete m_gamepadDevices[index];
    m_gamepadDevices[index] = 0;
}

gboolean GamepadController::onGamepadChange(GIOChannel* source, GIOCondition condition, gpointer data)
{
    GamepadController* handler = static_cast<GamepadController*>(data);

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

gboolean GamepadController::isGamepadDevice(struct udev_device* device)
{
    const char* deviceFile = udev_device_get_devnode(device);
    const char* sysfsPath = udev_device_get_syspath(device);

    if (!deviceFile || !sysfsPath)
        return FALSE;

    if (!udev_device_get_property_value(device, "ID_INPUT") || !udev_device_get_property_value(device, "ID_INPUT_JOYSTICK"))
        return FALSE;

    return g_str_has_prefix(deviceFile, "/dev/input/js");
}

void GamepadController::sampleGamepads(Nix::Gamepads& into)
{
    for (unsigned i = 0; i < Nix::Gamepads::itemsLengthCap; ++i)
        memcpy(&into.items[i], &m_gamepads.items[i], sizeof(m_gamepads.items[i]));

    into.length = m_gamepadDevices.size();
}

