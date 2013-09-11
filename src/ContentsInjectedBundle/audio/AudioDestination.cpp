/*
 *  Copyright (C) 2011 Igalia S.L
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

#include "AudioDestination.h"
#include "WebKitWebAudioSourceGStreamer.h"

#include <NixPlatform/CString.h>
#include <NixPlatform/String.h>
#include <NixPlatform/Vector.h>
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include <cstring>

using namespace Nix;

#ifndef GST_API_VERSION_1
static void onGStreamerWavparsePadAddedCallback(GstElement* element, GstPad* pad, AudioDestination* destination)
{
    destination->finishBuildingPipelineAfterWavParserPadReady(pad);
}
#endif

AudioDestination::AudioDestination(const Nix::String& inputDeviceId, size_t bufferSize, unsigned, unsigned, double sampleRate, AudioDevice::RenderCallback* renderCallback)
    : m_wavParserAvailable(false)
    , m_audioSinkAvailable(false)
    , m_pipeline(0)
    , m_sampleRate(sampleRate)
    , m_bufferSize(bufferSize)
    , m_isDevice(false)
    , m_loopId(0)
    , m_inputDeviceId(inputDeviceId)
    , m_renderCallback(renderCallback)
{
    printf("[%s] %p {%s}\n", __PRETTY_FUNCTION__, this, m_inputDeviceId.utf8().data());

    if (!std::strcmp(m_inputDeviceId.utf8().data(), "autoaudiosrc;default"))
        m_isDevice = true;

    // FIXME: NUMBER OF CHANNELS NOT USED??????????/ WHY??????????
    m_pipeline = gst_pipeline_new("play");

    GstElement* webkitAudioSrc = reinterpret_cast<GstElement*>(g_object_new(WEBKIT_TYPE_WEB_AUDIO_SRC,
                                                                            "rate", sampleRate,
                                                                            "handler", renderCallback,
                                                                            "frames", bufferSize, NULL));

    GstElement* wavParser = gst_element_factory_make("wavparse", 0);

    m_wavParserAvailable = wavParser;

    //ASSERT_WITH_MESSAGE(m_wavParserAvailable, "Failed to create GStreamer wavparse element");
    if (!m_wavParserAvailable)
        return;

#ifndef GST_API_VERSION_1
    g_signal_connect(wavParser, "pad-added", G_CALLBACK(onGStreamerWavparsePadAddedCallback), this);
#endif
    gst_bin_add_many(GST_BIN(m_pipeline), webkitAudioSrc, wavParser, NULL);
    gst_element_link_pads_full(webkitAudioSrc, "src", wavParser, "sink", GST_PAD_LINK_CHECK_NOTHING);

#ifdef GST_API_VERSION_1
    GstPad* srcPad = gst_element_get_static_pad(wavParser, "src");
    finishBuildingPipelineAfterWavParserPadReady(srcPad);
#endif
}

AudioDestination::~AudioDestination()
{
    printf("[%s] %p\n", __PRETTY_FUNCTION__, this);
    gst_element_set_state(m_pipeline, GST_STATE_NULL);
    gst_object_unref(m_pipeline);
    g_source_remove(m_loopId);
}

void AudioDestination::finishBuildingPipelineAfterWavParserPadReady(GstPad* pad)
{
    //FIXME: POSSIBLE LEAK!!!
    GstElement* audioSink = gst_element_factory_make("autoaudiosink", 0);
    m_audioSinkAvailable = audioSink;

    if (!audioSink)
        return;

    // Autoaudiosink does the real sink detection in the GST_STATE_NULL->READY transition
    // so it's best to roll it to READY as soon as possible to ensure the underlying platform
    // audiosink was loaded correctly.
    GstStateChangeReturn stateChangeReturn = gst_element_set_state(audioSink, GST_STATE_READY);
    if (stateChangeReturn == GST_STATE_CHANGE_FAILURE) {
        gst_element_set_state(audioSink, GST_STATE_NULL);
        m_audioSinkAvailable = false;
        return;
    }

    GstElement* audioConvert = gst_element_factory_make("audioconvert", 0);
    gst_bin_add_many(GST_BIN(m_pipeline), audioConvert, audioSink, NULL);

    // Link wavparse's src pad to audioconvert sink pad.
    GstPad* sinkPad = gst_element_get_static_pad(audioConvert, "sink");
    gst_pad_link_full(pad, sinkPad, GST_PAD_LINK_CHECK_NOTHING);

    // Link audioconvert to audiosink and roll states.
    gst_element_link_pads_full(audioConvert, "src", audioSink, "sink", GST_PAD_LINK_CHECK_NOTHING);
    gst_element_sync_state_with_parent(audioConvert);
    gst_element_sync_state_with_parent(audioSink); //FIXME: I believe we should delete audioSink after this.
}

void AudioDestination::start()
{
    printf("[%s] %p {%s}\n", __PRETTY_FUNCTION__, this, m_inputDeviceId.utf8().data());
    if (m_isDevice) {
        m_loopId = g_idle_add(s_audioProcessLoop, this);
        return;
    }

    if (!m_wavParserAvailable)
        return;

    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
}

void AudioDestination::stop()
{
    if (!m_wavParserAvailable || !m_audioSinkAvailable)
        return;

    gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
}

gboolean AudioDestination::audioProcessLoop()
{
    printf("[%s] %p\n", __PRETTY_FUNCTION__, this);
    /*
    size_t bufferSize = this->m_bufferSize;
    Nix::Vector<float*> sourceDataVector((size_t) 2);
    static bool first = true;
    for (size_t i = 0; i < sourceDataVector.size(); ++i) {
        sourceDataVector[i] = new float[bufferSize];
        if (first)
            for (size_t j = 0; j < bufferSize; ++j)
                sourceDataVector[i][j] = (float) j;
        else
            std::memset(sourceDataVector[i], 0, bufferSize * sizeof(float));
    }
    first = false;

    Nix::Vector<float*> audioDataVector((size_t) 2);
    for (size_t i = 0; i < audioDataVector.size(); ++i) {
        audioDataVector[i] = new float[bufferSize];
        std::memset(audioDataVector[i], 0, bufferSize * sizeof(float));
    }

    this->m_renderCallback->render(sourceDataVector, audioDataVector, bufferSize);
    for (size_t i = 0; i < audioDataVector.size(); ++i) {
        for (size_t j = 0; j < bufferSize; ++j) {
            printf("%f ", audioDataVector[i][j]);
        }
    }
    printf("\n");

    static bool shouldContinue = true;
    static float count = 1;
    for (size_t i = 0; i < bufferSize; ++i)
        if (audioDataVector[0][i] != 0)
            if (audioDataVector[0][i] == audioDataVector[1][i] && audioDataVector[0][i] == count)
                count += 1;
            else
                shouldContinue = false;

    for (size_t i = 0; i < sourceDataVector.size(); ++i)
        delete[] sourceDataVector[i];
    for (size_t i = 0; i < audioDataVector.size(); ++i)
        delete[] audioDataVector[i];
    return shouldContinue;
    //*/
    static int a = 5;
    return (a-- != 0);
}
