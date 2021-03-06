#include "BrowserPlatform.h"
#include "AudioFileReader.h"
#include "GstAudioDevice.h"

#include <NixPlatform/MultiChannelPCMData.h>

#include <cstdio>

using namespace Nix;

bool initializeAudioBackend()
{
    bool didInitialize = true;
    if (!gst_is_initialized())
        didInitialize = gst_init_check(0, 0, 0);

    // If gstreamer was already initialized, then we return a success.
    return didInitialize;
}

MultiChannelPCMData* BrowserPlatform::decodeAudioResource(const void* audioFileData, size_t dataSize, double sampleRate)
{
    return AudioFileReader(audioFileData, dataSize).createBus(sampleRate);
}

AudioDevice* BrowserPlatform::createAudioDevice(const char* inputDeviceId, size_t bufferSize, unsigned numberOfInputChannels, unsigned numberOfChannels, double sampleRate, AudioDevice::RenderCallback* renderCallback)
{
    return new GstAudioDevice(inputDeviceId, bufferSize, numberOfInputChannels, numberOfChannels, sampleRate, renderCallback);
}
