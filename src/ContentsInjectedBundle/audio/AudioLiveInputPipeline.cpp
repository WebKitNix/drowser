/*
 *  Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
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

#include <cstdio>
#include <cassert>

#include "AudioLiveInputPipeline.h"

GstCaps* getGstAudioCaps(int, float);
static void onGStreamerDeinterleavePadAddedCallback(GstElement*, GstPad*, AudioLiveInputPipeline*);
static void onGStreamerDeinterleaveReadyCallback(GstElement*, AudioLiveInputPipeline*);

#ifdef AUDIO_ENABLE_PIPELINE_MSG_DUMP
static gboolean messageCallback(GstBus*, GstMessage*, GstElement*);
#endif

AudioLiveInputPipeline::AudioLiveInputPipeline(float sampleRate)
    : m_sinkList(nullptr)
    , m_sampleRate(sampleRate)
    , m_ready(false)
{
    GST_DEBUG("Constructing  InputReader - sampleRate: %f", m_sampleRate);
    buildInputPipeline();
    start();
}

AudioLiveInputPipeline::~AudioLiveInputPipeline()
{
    if (m_sinkList)
        g_slist_free(m_sinkList);

    if (m_deInterleave) {
        g_signal_handlers_disconnect_by_func(m_deInterleave,
            reinterpret_cast<gpointer>(onGStreamerDeinterleavePadAddedCallback), this);
        g_signal_handlers_disconnect_by_func(m_deInterleave,
            reinterpret_cast<gpointer>(onGStreamerDeinterleaveReadyCallback), this);
    }

    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(m_pipeline));
    }
}

guint AudioLiveInputPipeline::pullChannelBuffers(GSList **bufferList)
{
    if (!m_ready || !m_sinkList) {
        GST_WARNING("Live-input pipeline not yet ready.");
        return 0;
    }

    GSList* it = m_sinkList;
    GstAppSink* sink;
    GstBuffer* buffer;
    guint count;
    for(count = 0; it != nullptr; ++count, it = g_slist_next(it)) {
        sink = GST_APP_SINK(static_cast<GstElement*>(it->data));
        buffer = pullNewBuffer(sink);
        if (!buffer) {
            GST_WARNING_OBJECT(m_pipeline, "Failed to pull input buffer for channel %d", count);
            continue;
        }
        (*bufferList) = g_slist_append(*bufferList, buffer);
    }
    return count;
}

GstBuffer* AudioLiveInputPipeline::pullNewBuffer(GstAppSink* sink)
{
    GstSample* sample = gst_app_sink_pull_sample(sink);
    if (!sample)
        return nullptr;

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        gst_sample_unref(sample);
        return nullptr;
    }

    GstCaps* caps = gst_sample_get_caps(sample);
    if (!caps) {
        gst_sample_unref(sample);
        return nullptr;
    }

    GstAudioInfo info;
    gst_audio_info_from_caps(&info, caps);

    // Check the first audio channel.
    // The buffer is supposed to store data of a single channel anyway.
    switch (GST_AUDIO_INFO_POSITION(&info, 0)) {
    case GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT:
    case GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT:
        // Transferring ownership of buffer to the calling object.
        gst_buffer_ref(buffer);
        break;
    default:
        buffer = nullptr;
        break;
    }

    gst_sample_unref(sample);
    return buffer;
}

gboolean AudioLiveInputPipeline::sendQuery(GstQuery* query)
{
    assert(m_pipeline);
    return gst_element_query(GST_ELEMENT(m_pipeline), query);
}

void AudioLiveInputPipeline::handleNewDeinterleavePad(GstPad* pad)
{
    // A new pad for a planar channel was added in deinterleave.
    // Plug in an appsink so we can pull the data from each channel.
    // Pipeline looks like:
    // ... deinterleave ! appsink.
    GstElement* queue = gst_element_factory_make("queue", nullptr);
    GstElement* sink = gst_element_factory_make("appsink", nullptr);

    gst_bin_add_many(GST_BIN(m_pipeline), queue, sink, nullptr);

    GstPad* sinkPad = gst_element_get_static_pad(queue, "sink");
    gst_pad_link_full(pad, sinkPad, GST_PAD_LINK_CHECK_NOTHING);
    gst_object_unref(GST_OBJECT(sinkPad));

    gst_element_link_pads_full(queue, "src", sink, "sink", GST_PAD_LINK_CHECK_NOTHING);

    m_sinkList = g_slist_prepend(m_sinkList, sink);
    gst_element_sync_state_with_parent(queue);
    gst_element_sync_state_with_parent(sink);
}

void AudioLiveInputPipeline::deinterleavePadsConfigured()
{
    m_ready = true;
}

GstStateChangeReturn AudioLiveInputPipeline::start()
{
    assert(m_ready);
    return gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
}

//#define AUDIO_FAKE_INPUT
void AudioLiveInputPipeline::buildInputPipeline()
{
    // Sub pipeline looks like:
    // ... autoaudiosrc ! audioconvert ! capsfilter ! deinterleave.
    m_pipeline = gst_pipeline_new("live-input");

#ifndef AUDIO_FAKE_INPUT
    // FIXME: Use autoaudiosrc instead of pulsesrc and set properties using
    // gstproxychild, as is being done in AudioDestinatio::configureSinkDevice.
    GstElement *source = gst_element_factory_make("pulsesrc", "liveinputsrc");
    g_object_set(source, "blocksize", (gint64)1024, nullptr);
    g_object_set(source, "buffer-time", (gint64) 1451, nullptr);
    g_object_set(source, "latency-time", (gint64) 1451, nullptr);
#else
    GstElement *source = gst_element_factory_make("audiotestsrc", "fakeinput");
    g_object_set(source, "is-live", TRUE, nullptr);
    g_object_set(source, "blocksize", 2048, nullptr);
    g_object_set(source, "buffer-time", (gint64) 1451, nullptr);
    g_object_set(source, "latency-time", (guint64) 1451, nullptr);
#endif

    m_source = source;

    GstElement* audioConvert  = gst_element_factory_make("audioconvert", nullptr);
    GstElement* capsFilter = gst_element_factory_make("capsfilter", nullptr);
    m_deInterleave = gst_element_factory_make("deinterleave", "deinterleave");

    g_object_set(m_deInterleave, "keep-positions", TRUE, nullptr);
    g_signal_connect(m_deInterleave, "pad-added", G_CALLBACK(onGStreamerDeinterleavePadAddedCallback), this);
    g_signal_connect(m_deInterleave, "no-more-pads", G_CALLBACK(onGStreamerDeinterleaveReadyCallback), this);

    GstCaps* caps = getGstAudioCaps(2, m_sampleRate);
    g_object_set(capsFilter, "caps", caps, nullptr);

    gst_bin_add_many(GST_BIN(m_pipeline), source, audioConvert, capsFilter, m_deInterleave, nullptr);
    gst_element_link_pads_full(source, "src", audioConvert, "sink", GST_PAD_LINK_CHECK_NOTHING);
    gst_element_link_pads_full(audioConvert, "src", capsFilter, "sink", GST_PAD_LINK_CHECK_NOTHING);
    gst_element_link_pads_full(capsFilter, "src", m_deInterleave, "sink", GST_PAD_LINK_CHECK_NOTHING);

    GstPad* pad = gst_element_get_static_pad(m_deInterleave, "sink");
    gst_pad_set_caps(pad, caps);

    m_ready = true;

    gst_element_sync_state_with_parent(source);
    gst_element_sync_state_with_parent(audioConvert);
    gst_element_sync_state_with_parent(capsFilter);
    gst_element_sync_state_with_parent(m_deInterleave);
}

GstCaps* getGstAudioCaps(int channels, float sampleRate)
{
    return gst_caps_new_simple("audio/x-raw", "rate", G_TYPE_INT, static_cast<int>(sampleRate),
        "channels", G_TYPE_INT, channels,
        "format", G_TYPE_STRING, gst_audio_format_to_string(GST_AUDIO_FORMAT_F32),
        "layout", G_TYPE_STRING, "interleaved", nullptr);
}

static void onGStreamerDeinterleavePadAddedCallback(GstElement*, GstPad* pad, AudioLiveInputPipeline* reader)
{
    reader->handleNewDeinterleavePad(pad);
}

static void onGStreamerDeinterleaveReadyCallback(GstElement*, AudioLiveInputPipeline* reader)
{
    reader->deinterleavePadsConfigured();
}
