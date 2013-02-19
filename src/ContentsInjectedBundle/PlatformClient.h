#ifndef PlatformClient_h
#define PlatformClient_h

#include <NixPlatform/Platform.h>

class GamepadController;

class PlatformClient : public WebKit::Platform {
public:
    PlatformClient();

    // Audio --------------------------------------------------------------
    virtual double audioHardwareSampleRate() { return 44100; }
    virtual size_t audioHardwareBufferSize() { return 128; }

    // Creates a device for audio I/O.
    // Pass in (numberOfInputChannels > 0) if live/local audio input is desired.
    virtual WebKit::WebAudioDevice* createAudioDevice(size_t bufferSize, unsigned numberOfInputChannels, unsigned numberOfChannels, double sampleRate, WebKit::WebAudioDevice::RenderCallback* renderCallback);

    // Resources -----------------------------------------------------------
    // Returns a blob of data corresponding to the named resource.
    virtual WebKit::WebData loadResource(const char* name);

    // Decodes the in-memory audio file data and returns the linear PCM audio data in the destinationBus.
    // A sample-rate conversion to sampleRate will occur if the file data is at a different sample-rate.
    // Returns true on success.
    virtual bool loadAudioResource(WebKit::WebAudioBus* destinationBus, const char* audioFileData, size_t dataSize, double sampleRate);

    // Gamepad
    virtual void sampleGamepads(WebKit::WebGamepads& into);

    // FFTFrame
    virtual WebKit::WebFFTFrame* createFFTFrame(unsigned fftsize);
    virtual WebKit::WebFFTFrame* createFFTFrame(const WebKit::WebFFTFrame* frame);
private:
    // Gamepad
    void initializeGamepadController();
    GamepadController* m_gamepadController;
};

#endif
