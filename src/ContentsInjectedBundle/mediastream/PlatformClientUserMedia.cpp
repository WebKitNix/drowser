#include "PlatformClient.h"
#include "MediaStreamCenter.h"
#include "UserMediaClient.h"

Nix::UserMediaClient* PlatformClient::createUserMediaClient()
{
	printf("[%s]\n", __PRETTY_FUNCTION__);
    return new UserMediaClient();
}

Nix::MediaStreamCenter* PlatformClient::createMediaStreamCenter()
{
	printf("[%s]\n", __PRETTY_FUNCTION__);
    return new MediaStreamCenter();
}
