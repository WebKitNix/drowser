#include "BrowserPlatform.h"
#include "MediaStreamCenter.h"

Nix::MediaStreamCenter* BrowserPlatform::createMediaStreamCenter()
{
    return new MediaStreamCenter();
}
