#ifndef BrowserPlatform_h
#define BrowserPlatform_h

#include <NixPlatform/Platform.h>

class GamepadController;

class BrowserPlatform : public Nix::Platform {
public:
    BrowserPlatform();

    // Audio --------------------------------------------------------------
    virtual float audioHardwareSampleRate() { return 44100; }
    virtual size_t audioHardwareBufferSize() { return 128; }
    virtual unsigned audioHardwareOutputChannels() { return 2; }

    // Creates a device for audio I/O.
    // Pass in (numberOfInputChannels > 0) if live/local audio input is desired.
    virtual Nix::AudioDevice* createAudioDevice(size_t bufferSize, unsigned numberOfInputChannels, unsigned numberOfChannels, double sampleRate, Nix::AudioDevice::RenderCallback* renderCallback);

    // Resources -----------------------------------------------------------
    // Returns a blob of data corresponding to the named resource.
    virtual Nix::Data loadResource(const char* name);

    // Decodes the in-memory audio file data and returns the linear PCM audio data in the destinationBus.
    // A sample-rate conversion to sampleRate will occur if the file data is at a different sample-rate.
    // Returns true on success.
    virtual bool loadAudioResource(Nix::AudioBus* destinationBus, const char* audioFileData, size_t dataSize, double sampleRate);

    // Gamepad
    virtual void sampleGamepads(Nix::Gamepads& into);

    // FFTFrame
    virtual Nix::FFTFrame* createFFTFrame(unsigned fftsize);

    // Media player
    virtual Nix::MediaPlayer* createMediaPlayer(Nix::MediaPlayerClient*);
};

#endif
