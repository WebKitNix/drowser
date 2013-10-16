#include "BrowserPlatform.h"

#include <cstdio>

extern bool initializeAudioBackend();

BrowserPlatform::BrowserPlatform()
{
    initializeAudioBackend();
}
