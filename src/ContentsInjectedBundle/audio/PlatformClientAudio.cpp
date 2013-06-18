#include "PlatformClient.h"
#include "AudioFileReader.h"
#include "AudioDestination.h"

#include <NixPlatform/WebAudioBus.h>

#include <cstdio>

using namespace WebKit;

bool initializeAudioBackend()
{
    bool didInitialize = true;
    if (!gst_is_initialized())
        didInitialize = gst_init_check(0, 0, 0);

    // If gstreamer was already initialized, then we return a success.
    return didInitialize;
}

bool PlatformClient::loadAudioResource(WebAudioBus* destinationBus, const char* audioFileData, size_t dataSize, double sampleRate)
{
    return AudioFileReader(audioFileData, dataSize).createBus(destinationBus, sampleRate);
}

WebData PlatformClient::loadResource(const char* name)
{
    return AudioFileReader::loadResource(name);
}

WebAudioDevice* PlatformClient::createAudioDevice(size_t bufferSize, unsigned numberOfInputChannels, unsigned numberOfChannels, double sampleRate, WebAudioDevice::RenderCallback* renderCallback)
{
    return new AudioDestination(bufferSize, numberOfInputChannels, numberOfChannels, sampleRate, renderCallback);
}
