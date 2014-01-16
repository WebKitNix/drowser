audio = Library:new("audio", STATIC)
audio:use(gstreamer)
audio:use(gstreamerApp)
audio:use(gstreamerAudio)
audio:use(gstreamerFft)
audio:use(gstreamPbUtils)
audio:use(nix)
audio:addIncludePath("..")
audio:addFiles([[
    AudioFileReader.cpp
    AudioLiveInputPipeline.cpp
    BrowserPlatformAudio.cpp
    FFTGStreamer.cpp
    GstAudioDevice.cpp
    WebKitWebAudioSourceGStreamer.cpp
]])
