#include "BrowserPlatform.h"
#include "AudioFileReader.h"
#include "GstAudioDevice.h"

#include <NixPlatform/AudioBus.h>

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

bool BrowserPlatform::loadAudioResource(AudioBus* destinationBus, const char* audioFileData, size_t dataSize, double sampleRate)
{
    return AudioFileReader(audioFileData, dataSize).createBus(destinationBus, sampleRate);
}

Data BrowserPlatform::loadResource(const char* name)
{
    return AudioFileReader::loadResource(name);
}

AudioDevice* BrowserPlatform::createAudioDevice(size_t bufferSize, unsigned numberOfInputChannels, unsigned numberOfChannels, double sampleRate, AudioDevice::RenderCallback* renderCallback)
{
    return new GstAudioDevice(bufferSize, numberOfInputChannels, numberOfChannels, sampleRate, renderCallback);
}
