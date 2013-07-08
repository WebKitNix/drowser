usermedia = Library:new("usermedia", STATIC)
usermedia:usePackage(nix)
usermedia:addIncludePath("..")
usermedia:addFiles([[
    UserMediaClient.cpp
    PlatformClientUserMedia.cpp
]])
