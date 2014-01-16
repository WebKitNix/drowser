mediaPlayer = Library:new("mediaPlayer", STATIC)
mediaPlayer:addFiles([[
    MediaPlayer.cpp
]])

mediaPlayer:addIncludePath("..")
mediaPlayer:use(gstreamer)
mediaPlayer:use(gstreamerApp)
mediaPlayer:use(gstreamerAudio)
mediaPlayer:use(gstreamerFft)
mediaPlayer:use(gstreamPbUtils)
mediaPlayer:use(nix)
