gstreamer = findPackage("gstreamer-0.10", REQUIRED)
gstreamerApp = findPackage("gstreamer-app-0.10", REQUIRED)
gstreamerAudio = findPackage("gstreamer-audio-0.10", REQUIRED)

audio = Library:new("audio", STATIC)
audio:usePackage(gstreamer)
audio:usePackage(gstreamerApp)
audio:usePackage(gstreamerAudio)
audio:addFiles([
    AudioDestination.cpp
    AudioFileReader.cpp
    PlatformClientAudio.cpp
    WebKitWebAudioSourceGStreamer.cpp
])
