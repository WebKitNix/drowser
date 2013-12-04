audio = Library:new("audio", STATIC)
audio:usePackage(gstreamer)
audio:usePackage(gstreamerApp)
audio:usePackage(gstreamerAudio)
audio:usePackage(gstreamerFft)
audio:usePackage(gstreamPbUtils)
audio:usePackage(nix)
audio:addIncludePath("..")
audio:addFiles([[
    AudioFileReader.cpp
    AudioLiveInputPipeline.cpp
    BrowserPlatformAudio.cpp
    FFTGStreamer.cpp
    GstAudioDevice.cpp
    WebKitWebAudioSourceGStreamer.cpp
]])
