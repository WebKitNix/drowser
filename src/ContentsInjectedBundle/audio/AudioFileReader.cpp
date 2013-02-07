/*
 *  Copyright (C) 2011, 2012 Igalia S.L
 *  Copyright (C) 2011 Zan Dobersek  <zandobersek@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "AudioFileReader.h"

#include <NixPlatform/WebAudioBus.h>

#include <gio/gio.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/pbutils/pbutils.h>
#include <gst/audio/multichannel.h>

#include <glib.h>
#include <cstring>
#include <cstdio>

using namespace WebKit;

WebData AudioFileReader::loadResource(const char* name)
{
    FILE* pFile;
    long lSize;
    char* buffer;
    size_t result;

    pFile = fopen(name, "rb");

    fseek(pFile , 0 , SEEK_END);
    lSize = ftell(pFile);
    rewind(pFile);

    buffer = (char*) malloc(sizeof(char) * lSize);
    result = fread(buffer, sizeof(char), lSize, pFile);

    // terminate
    fclose(pFile);
    return WebData(buffer, result);
}

static GstCaps* getGStreamerAudioCaps(int channels, float sampleRate)
{
    return gst_caps_new_simple("audio/x-raw-float", "rate", G_TYPE_INT, static_cast<int>(sampleRate), "channels", G_TYPE_INT, channels, "endianness", G_TYPE_INT, G_BYTE_ORDER, "width", G_TYPE_INT, 32, NULL);
}

static void copyGStreamerBuffersToAudioChannel(GstBufferList* buffers, float* audioChannel)
{
    GstBufferListIterator* iter = gst_buffer_list_iterate(buffers);
    gst_buffer_list_iterator_next_group(iter);
    GstBuffer* buffer = gst_buffer_list_iterator_merge_group(iter);
    if (buffer) {
        memcpy(audioChannel, reinterpret_cast<float*>(GST_BUFFER_DATA(buffer)), GST_BUFFER_SIZE(buffer));
        gst_buffer_unref(buffer);
    }
    gst_buffer_list_iterator_free(iter);
}

static GstFlowReturn onAppsinkNewBufferCallback(GstAppSink* sink, gpointer userData)
{
    return static_cast<AudioFileReader*>(userData)->handleBuffer(sink);
}

gboolean messageCallback(GstBus* bus, GstMessage* message, AudioFileReader* reader)
{
    return reader->handleMessage(message);
}

static void onGStreamerDeinterleavePadAddedCallback(GstElement* element, GstPad* pad, AudioFileReader* reader)
{
    reader->handleNewDeinterleavePad(pad);
}

static void onGStreamerDeinterleaveReadyCallback(GstElement* element, AudioFileReader* reader)
{
    reader->deinterleavePadsConfigured();
}

static void onGStreamerDecodebinPadAddedCallback(GstElement* element, GstPad* pad, AudioFileReader* reader)
{
    reader->plugDeinterleave(pad);
}

gboolean enteredMainLoopCallback(gpointer userData)
{
    AudioFileReader* reader = reinterpret_cast<AudioFileReader*>(userData);
    reader->decodeAudioForBusCreation();
    return FALSE;
}

AudioFileReader::AudioFileReader(const void* data, size_t dataSize)
    : m_data(data)
    , m_dataSize(dataSize)
    , m_sampleRate(44100)
    , m_frontLeftBuffers(0)
    , m_frontLeftBuffersIterator(0)
    , m_frontRightBuffers(0)
    , m_frontRightBuffersIterator(0)
    , m_pipeline(0)
    , m_channelSize(0)
    , m_decodebin(0)
    , m_deInterleave(0)
    , m_loop(0)
    , m_errorOccurred(false)
{
}

AudioFileReader::~AudioFileReader()
{
    if (m_pipeline) {
        GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
        g_signal_handlers_disconnect_by_func(bus, reinterpret_cast<gpointer>(messageCallback), this);
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(m_pipeline));
    }

    //////////// FIXME WITH PROPER UNREF OR DELETE!!!!
    /*if (m_decodebin) {
        g_signal_handlers_disconnect_by_func(m_decodebin, reinterpret_cast<gpointer>(onGStreamerDecodebinPadAddedCallback), this);
        gst_object_unref(GST_OBJECT(m_decodebin));
    }*/

    /*if (m_deInterleave) {
        g_signal_handlers_disconnect_by_func(m_deInterleave, reinterpret_cast<gpointer>(onGStreamerDeinterleavePadAddedCallback), this);
        g_signal_handlers_disconnect_by_func(m_deInterleave, reinterpret_cast<gpointer>(onGStreamerDeinterleaveReadyCallback), this);
        gst_object_unref(GST_OBJECT(m_deInterleave));
    }*/
    //////////// FIXME WITH PROPER UNREF OR DELETE!!!!

    gst_buffer_list_iterator_free(m_frontLeftBuffersIterator);
    gst_buffer_list_iterator_free(m_frontRightBuffersIterator);
    gst_buffer_list_unref(m_frontLeftBuffers);
    gst_buffer_list_unref(m_frontRightBuffers);

    //FIXME DELETE THE REST OF THE PTRs!!!*/
}

GstFlowReturn AudioFileReader::handleBuffer(GstAppSink* sink)
{
    GstBuffer* buffer = gst_app_sink_pull_buffer(sink);
    if (!buffer)
        return GST_FLOW_ERROR;

    GstCaps* caps = gst_buffer_get_caps(buffer);
    GstStructure* structure = gst_caps_get_structure(caps, 0);

    gint channels = 0;
    if (!gst_structure_get_int(structure, "channels", &channels) || !channels) {
        gst_caps_unref(caps);
        gst_buffer_unref(buffer);
        return GST_FLOW_ERROR;
    }

    gint sampleRate = 0;
    if (!gst_structure_get_int(structure, "rate", &sampleRate) || !sampleRate) {
        gst_caps_unref(caps);
        gst_buffer_unref(buffer);
        return GST_FLOW_ERROR;
    }

    gint width = 0;
    if (!gst_structure_get_int(structure, "width", &width) || !width) {
        gst_caps_unref(caps);
        gst_buffer_unref(buffer);
        return GST_FLOW_ERROR;
    }

    GstClockTime duration = (static_cast<guint64>(GST_BUFFER_SIZE(buffer)) * 8 * GST_SECOND) / (sampleRate * channels * width);
    int frames = GST_CLOCK_TIME_TO_FRAMES(duration, sampleRate);

    // Check the first audio channel. The buffer is supposed to store
    // data of a single channel anyway.
    GstAudioChannelPosition* positions = gst_audio_get_channel_positions(structure);
    switch (positions[0]) {
    case GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT:
        gst_buffer_list_iterator_add(m_frontLeftBuffersIterator, buffer);
        m_channelSize += frames;
        break;
    case GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT:
        gst_buffer_list_iterator_add(m_frontRightBuffersIterator, buffer);
        break;
    default:
        gst_buffer_unref(buffer);
        break;
    }

    g_free(positions);
    gst_caps_unref(caps);
    return GST_FLOW_OK;
}

gboolean AudioFileReader::handleMessage(GstMessage* message)
{
    //GError** error;
    //gchar** debug;
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
        g_main_loop_quit(m_loop);
        break;
    case GST_MESSAGE_WARNING:
        //gst_message_parse_warning(message, error, debug);
        //g_warning("Warning: %d, %s. Debug output: %s", error->code,  error->message, debug);
        break;
    case GST_MESSAGE_ERROR:
        //gst_message_parse_error(message, error, debug);
        //g_warning("Error: %d, %s. Debug output: %s", error->code,  error->message, debug);
        m_errorOccurred = true;
        g_main_loop_quit(m_loop);
        break;
    default:
        break;
    }

    //delete error;
    //delete debug;
    return TRUE;
}

void AudioFileReader::handleNewDeinterleavePad(GstPad* pad)
{
    // A new pad for a planar channel was added in deinterleave. Plug
    // in an appsink so we can pull the data from each
    // channel. Pipeline looks like:
    // ... deinterleave ! queue ! appsink.
    GstElement* queue = gst_element_factory_make("queue", 0);
    GstElement* sink = gst_element_factory_make("appsink", 0);

    GstAppSinkCallbacks callbacks;
    callbacks.eos = 0;
    callbacks.new_preroll = 0;
    callbacks.new_buffer_list = 0;
    callbacks.new_buffer = onAppsinkNewBufferCallback;
    gst_app_sink_set_callbacks(GST_APP_SINK(sink), &callbacks, this, 0);

    g_object_set(sink, "sync", FALSE, NULL);

    GstCaps* caps = getGStreamerAudioCaps(1, m_sampleRate);
    gst_app_sink_set_caps(GST_APP_SINK(sink), caps);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(m_pipeline), queue, sink, NULL);

    GstPad* sinkPad = gst_element_get_static_pad(queue, "sink");
    gst_pad_link(pad, sinkPad);
    gst_object_unref(GST_OBJECT(sinkPad));

    gst_element_link_pads_full(queue, "src", sink, "sink", GST_PAD_LINK_CHECK_NOTHING);

    gst_element_set_state(queue, GST_STATE_READY);
    gst_element_set_state(sink, GST_STATE_READY);
}

void AudioFileReader::deinterleavePadsConfigured()
{
    // All deinterleave src pads are now available, let's roll to
    // PLAYING so data flows towards the sinks and it can be retrieved.
    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
}

void AudioFileReader::plugDeinterleave(GstPad* pad)
{
    // A decodebin pad was added, plug in a deinterleave element to
    // separate each planar channel. Sub pipeline looks like
    // ... decodebin2 ! audioconvert ! audioresample ! capsfilter ! deinterleave.
    //FIXME: POSSIBLE LEAKING PTR WARNING
    GstElement* audioConvert  = gst_element_factory_make("audioconvert", 0);
    GstElement* audioResample = gst_element_factory_make("audioresample", 0);
    GstElement* capsFilter = gst_element_factory_make("capsfilter", 0);
    m_deInterleave = gst_element_factory_make("deinterleave", "deinterleave");

    g_object_set(m_deInterleave, "keep-positions", TRUE, NULL);
    g_signal_connect(m_deInterleave, "pad-added", G_CALLBACK(onGStreamerDeinterleavePadAddedCallback), this);
    g_signal_connect(m_deInterleave, "no-more-pads", G_CALLBACK(onGStreamerDeinterleaveReadyCallback), this);

    GstCaps* caps = getGStreamerAudioCaps(2, m_sampleRate);
    g_object_set(capsFilter, "caps", caps, NULL);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(m_pipeline), audioConvert, audioResample, capsFilter, m_deInterleave, NULL);

    GstPad* sinkPad = gst_element_get_static_pad(audioConvert, "sink");
    gst_pad_link(pad, sinkPad);
    gst_object_unref(GST_OBJECT(sinkPad));

    gst_element_link_pads_full(audioConvert, "src", audioResample, "sink", GST_PAD_LINK_CHECK_NOTHING);
    gst_element_link_pads_full(audioResample, "src", capsFilter, "sink", GST_PAD_LINK_CHECK_NOTHING);
    gst_element_link_pads_full(capsFilter, "src", m_deInterleave, "sink", GST_PAD_LINK_CHECK_NOTHING);

    gst_element_sync_state_with_parent(audioConvert);
    gst_element_sync_state_with_parent(audioResample);
    gst_element_sync_state_with_parent(capsFilter);
    gst_element_sync_state_with_parent(m_deInterleave);
}

void AudioFileReader::decodeAudioForBusCreation()
{
    // Build the pipeline (giostreamsrc | filesrc) ! decodebin2
    // A deinterleave element is added once a src pad becomes available in decodebin.
    m_pipeline = gst_pipeline_new(0);

    //FIXME: POSSIBLE LEAKING PTR WARNING
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message", G_CALLBACK(messageCallback), this);

    GstElement* source = gst_element_factory_make("giostreamsrc", 0);
    //FIXME: POSSIBLE LEAKING PTR WARNING
    GInputStream* memoryStream = g_memory_input_stream_new_from_data(m_data, m_dataSize, 0);
    g_object_set(source, "stream", memoryStream, NULL);

    m_decodebin = gst_element_factory_make("decodebin2", 0);
    g_signal_connect(m_decodebin, "pad-added", G_CALLBACK(onGStreamerDecodebinPadAddedCallback), this);
    gst_bin_add_many(GST_BIN(m_pipeline), source, m_decodebin, NULL);
    gst_element_link_pads_full(source, "src", m_decodebin, "sink", GST_PAD_LINK_CHECK_NOTHING);
    gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
}

bool AudioFileReader::createBus(WebAudioBus* destinationBus, float sampleRate)
{
    m_sampleRate = sampleRate;

    m_frontLeftBuffers = gst_buffer_list_new();
    m_frontLeftBuffersIterator = gst_buffer_list_iterate(m_frontLeftBuffers);
    gst_buffer_list_iterator_add_group(m_frontLeftBuffersIterator);

    m_frontRightBuffers = gst_buffer_list_new();
    m_frontRightBuffersIterator = gst_buffer_list_iterate(m_frontRightBuffers);
    gst_buffer_list_iterator_add_group(m_frontRightBuffersIterator);

    //FIXME: POSSIBLE LEAKING PTR WARNING
    GMainContext* context = g_main_context_new();
    g_main_context_push_thread_default(context);
    m_loop = g_main_loop_new(context, FALSE);

    // Start the pipeline processing just after the loop is started.
    GSource* timeoutSource = g_timeout_source_new(0);
    g_source_attach(timeoutSource, context);
    g_source_set_callback(timeoutSource, reinterpret_cast<GSourceFunc>(enteredMainLoopCallback), this, 0);

    g_main_loop_run(m_loop);
    g_main_context_pop_thread_default(context);

    if (m_errorOccurred)
        return false;

    // This allocates everything for us (stereo)! :)
    destinationBus->initialize(2, m_channelSize, m_sampleRate);

    copyGStreamerBuffersToAudioChannel(m_frontLeftBuffers, destinationBus->channelData(0));
    copyGStreamerBuffersToAudioChannel(m_frontRightBuffers, destinationBus->channelData(1));

    return true;
}
