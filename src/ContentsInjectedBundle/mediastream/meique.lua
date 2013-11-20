mediastream = Library:new("mediaStream", STATIC)
mediastream:usePackage(nix)
mediastream:usePackage(gstreamer)
mediastream:addIncludePath("..")
mediastream:addFiles([[
    BrowserPlatformUserMedia.cpp
    MediaStreamCenter.cpp
]])
