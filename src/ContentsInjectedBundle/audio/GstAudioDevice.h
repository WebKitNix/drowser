#ifndef GstAudioDevice_h
#define GstAudioDevice_h

#include <gst/gst.h>
#include <NixPlatform/Platform.h>

class GstAudioDevice : public Nix::AudioDevice {
public:
    GstAudioDevice(size_t bufferSize, unsigned numberOfInputChannels, unsigned numberOfChannels, double sampleRate, Nix::AudioDevice::RenderCallback* renderCallback);
    virtual ~GstAudioDevice();

    virtual void start();
    virtual void stop();

    double sampleRate() { return m_sampleRate; }

    void finishBuildingPipelineAfterWavParserPadReady(GstPad*);

private:
    bool m_wavParserAvailable;
    bool m_audioSinkAvailable;
    GstElement* m_pipeline;
    double m_sampleRate;
};

#endif
