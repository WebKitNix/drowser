audio = Library:new("audio", STATIC)
audio:usePackage(gstreamer)
audio:usePackage(gstreamerApp)
audio:usePackage(gstreamerAudio)
audio:usePackage(gstreamerFft)
audio:usePackage(gstreamPbUtils)
audio:usePackage(nix)
audio:addIncludePath("..")
audio:addFiles([[
    GstAudioDevice.cpp
    AudioFileReader.cpp
    FFTGStreamer.cpp
    BrowserPlatformAudio.cpp
    WebKitWebAudioSourceGStreamer.cpp
]])
