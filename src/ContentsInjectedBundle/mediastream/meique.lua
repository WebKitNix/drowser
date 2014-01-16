mediastream = Library:new("mediaStream", STATIC)
mediastream:use(nix)
mediastream:use(gstreamer)
mediastream:addIncludePath("..")
mediastream:addFiles([[
    BrowserPlatformUserMedia.cpp
    MediaStreamCenter.cpp
]])
