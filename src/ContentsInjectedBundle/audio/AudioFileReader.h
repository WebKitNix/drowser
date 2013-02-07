#ifndef AudioFileReader_h
#define AudioFileReader_h

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include <NixPlatform/Platform.h>

class AudioFileReader {
public:
    AudioFileReader(const void* data, size_t dataSize);
    ~AudioFileReader();

    static WebKit::WebData loadResource(const char* name);

    bool createBus(WebKit::WebAudioBus* destinationBus, float sampleRate);

    GstFlowReturn handleBuffer(GstAppSink*);
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
    GstBufferListIterator* m_frontLeftBuffersIterator;
    GstBufferList* m_frontRightBuffers;
    GstBufferListIterator* m_frontRightBuffersIterator;
    GstElement* m_pipeline;
    unsigned m_channelSize;
    GstElement* m_decodebin;
    GstElement* m_deInterleave;
    GMainLoop* m_loop;
    bool m_errorOccurred;
};

#endif
