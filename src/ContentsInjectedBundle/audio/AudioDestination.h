#ifndef AudioDestination_h
#define AudioDestination_h

#include <gst/gst.h>
#include <NixPlatform/Platform.h>

class AudioDestination : public WebKit::WebAudioDevice {
public:
    AudioDestination(size_t bufferSize, unsigned numberOfInputChannels, unsigned numberOfChannels, double sampleRate, WebKit::WebAudioDevice::RenderCallback* renderCallback);
    virtual ~AudioDestination();

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
