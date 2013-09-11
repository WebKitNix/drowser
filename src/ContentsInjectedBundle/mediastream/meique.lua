mediastream = Library:new("mediastream", STATIC)
mediastream:usePackage(nix)
mediastream:addIncludePath("..")
mediastream:addFiles([[
    UserMediaClient.cpp
    MediaStreamCenter.cpp
    PlatformClientUserMedia.cpp
]])
