gstreamer = findPackage("gstreamer-0.10", REQUIRED)
gstreamerApp = findPackage("gstreamer-app-0.10", REQUIRED)
gstreamerAudio = findPackage("gstreamer-audio-0.10", REQUIRED)
gstreamerFft = findPackage("gstreamer-fft-0.10", REQUIRED)

audio = Library:new("audio", STATIC)
audio:addCustomFlags("-std=c++0x")
audio:usePackage(gstreamer)
audio:usePackage(gstreamerApp)
audio:usePackage(gstreamerAudio)
audio:usePackage(gstreamerFft)
audio:usePackage(nix)
audio:addIncludePath("..")
audio:addFiles([[
    GstAudioDevice.cpp
    AudioFileReader.cpp
    FFTGStreamer.cpp
    PlatformClientAudio.cpp
    WebKitWebAudioSourceGStreamer.cpp
]])
