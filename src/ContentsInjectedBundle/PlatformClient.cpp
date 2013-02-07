#include "PlatformClient.h"

#include <cstdio>

extern bool initializeAudioBackend();

PlatformClient::PlatformClient()
{
    initializeAudioBackend();
}
