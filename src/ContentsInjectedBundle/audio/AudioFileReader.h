#ifndef AudioFileReader_h
#define AudioFileReader_h

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include <NixPlatform/Platform.h>

class AudioFileReader {
public:
    AudioFileReader(const void* data, size_t dataSize);
    ~AudioFileReader();

    static Nix::Data loadResource(const char* name);

    bool createBus(Nix::AudioBus* destinationBus, float sampleRate);

    GstFlowReturn handleSample(GstAppSink*);
    gboolean handleMessage(GstMessage*);
    void handleNewDeinterleavePad(GstPad*);
    void deinterleavePadsConfigured();
    void plugDeinterleave(GstPad*);
    void decodeAudioForBusCreation();

private:
    const void* m_data;
    size_t m_dataSize;
    float m_sampleRate;

    GstBufferList* m_frontLeftBuffers;
    GstBufferList* m_frontRightBuffers;
    GstElement* m_pipeline;
    unsigned m_channelSize;
    GstElement* m_decodebin;
    GstElement* m_deInterleave;
    GMainLoop* m_loop;
    bool m_errorOccurred;
};

#endif
