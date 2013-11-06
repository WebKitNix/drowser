#ifndef BrowserPlatform_h
#define BrowserPlatform_h

#include <NixPlatform/Platform.h>

class GamepadController;

class BrowserPlatform : public Nix::Platform {
public:
    BrowserPlatform();

    // Audio --------------------------------------------------------------
    virtual float audioHardwareSampleRate() override { return 44100; }
    virtual size_t audioHardwareBufferSize() override { return 128; }
    virtual unsigned audioHardwareOutputChannels() override { return 2; }

    // Creates a device for audio I/O.
    // Pass in (numberOfInputChannels > 0) if live/local audio input is desired.
    virtual Nix::AudioDevice* createAudioDevice(const char* inputDeviceId, size_t bufferSize, unsigned numberOfInputChannels, unsigned numberOfChannels, double sampleRate, Nix::AudioDevice::RenderCallback* renderCallback) override;

    virtual Nix::MediaStreamCenter* createMediaStreamCenter() override;

    // Decodes the in-memory audio file data and returns the linear PCM audio data in the destinationBus.
    virtual Nix::MultiChannelPCMData* decodeAudioResource(const void* audioFileData, size_t dataSize, double sampleRate) override;

    // Gamepad
    virtual void sampleGamepads(Nix::Gamepads& into) override;

    // FFTFrame
    virtual Nix::FFTFrame* createFFTFrame(unsigned fftsize) override;

    // Media player
    virtual Nix::MediaPlayer* createMediaPlayer(Nix::MediaPlayerClient*) override;
};

#endif
