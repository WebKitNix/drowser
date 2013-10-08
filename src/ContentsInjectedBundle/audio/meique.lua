gstreamer = findPackage("gstreamer-1.0", REQUIRED)
gstreamerApp = findPackage("gstreamer-app-1.0", REQUIRED)
gstreamerAudio = findPackage("gstreamer-audio-1.0", REQUIRED)
gstreamerFft = findPackage("gstreamer-fft-1.0", REQUIRED)

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
