#include "PlatformClient.h"
#include "UserMediaClient.h"

Nix::UserMediaClient* PlatformClient::createUserMediaClient()
{
    return new UserMediaClient();
}
