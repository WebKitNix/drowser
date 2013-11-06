mediastream = Library:new("mediaStream", STATIC)
mediastream:usePackage(nix)
mediastream:addIncludePath("..")
mediastream:addFiles([[
    BrowserPlatformUserMedia.cpp
    MediaStreamCenter.cpp
]])
