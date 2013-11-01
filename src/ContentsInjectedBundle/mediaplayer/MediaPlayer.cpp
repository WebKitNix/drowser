/*
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "MediaPlayer.h"
#include "BrowserPlatform.h"

#include <gst/audio/streamvolume.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

using namespace std;

// GstPlayFlags flags from playbin. It is the policy of GStreamer to
// not publicly expose element-specific enums. That's why this
// GstPlayFlags enum has been copied here.
typedef enum {
    GST_PLAY_FLAG_VIDEO         = (1 << 0),
    GST_PLAY_FLAG_AUDIO         = (1 << 1),
    GST_PLAY_FLAG_TEXT          = (1 << 2),
    GST_PLAY_FLAG_VIS           = (1 << 3),
    GST_PLAY_FLAG_SOFT_VOLUME   = (1 << 4),
    GST_PLAY_FLAG_NATIVE_AUDIO  = (1 << 5),
    GST_PLAY_FLAG_NATIVE_VIDEO  = (1 << 6),
    GST_PLAY_FLAG_DOWNLOAD      = (1 << 7),
    GST_PLAY_FLAG_BUFFERING     = (1 << 8),
    GST_PLAY_FLAG_DEINTERLACE   = (1 << 9),
    GST_PLAY_FLAG_SOFT_COLORBALANCE = (1 << 10)
} GstPlayFlags;

Nix::MediaPlayer* BrowserPlatform::createMediaPlayer(Nix::MediaPlayerClient* client)
{
    return new MediaPlayer(client);
}

MediaPlayer::MediaPlayer(Nix::MediaPlayerClient* client)
    : Nix::MediaPlayer(client)
    , m_playBin(nullptr)
    , m_paused(true)
    , m_isLive(false)
    , m_seeking(false)
    , m_pendingSeek(false)
    , m_bufferingFinished(false)
    , m_playbackRate(1)
    , m_readyState(Nix::MediaPlayerClient::HaveNothing)
    , m_networkState(Nix::MediaPlayerClient::Empty)
{
}

MediaPlayer::~MediaPlayer()
{
    if (m_playBin) {
        gst_element_set_state(m_playBin, GST_STATE_NULL);
        gst_object_unref(m_playBin);
    }
}

void MediaPlayer::play()
{
    if (!m_paused)
        return;
    gst_element_set_state(m_playBin, GST_STATE_PLAYING);
    m_paused = false;
}

void MediaPlayer::pause()
{
    if (m_paused)
        return;
    gst_element_set_state(m_playBin, GST_STATE_PAUSED);
    m_paused = true;
}

float MediaPlayer::duration() const
{
    gint64 duration = GST_CLOCK_TIME_NONE;
    gst_element_query_duration(m_playBin, GST_FORMAT_TIME, &duration);
    if (GST_CLOCK_TIME_IS_VALID(duration))
        return static_cast<double>(duration) / GST_SECOND;
    return 0;
}

float MediaPlayer::currentTime() const
{
    gint64 current = GST_CLOCK_TIME_NONE;
    gst_element_query_position(m_playBin, GST_FORMAT_TIME, &current);
    if (GST_CLOCK_TIME_IS_VALID(current))
        return static_cast<double>(current) / GST_SECOND;
    return 0;
}

void MediaPlayer::seek(float time)
{
    if (!m_playBin)
        return;

    if (time == currentTime())
        return;

    if (m_isLive)
        return;

    m_seekTime = time;

    if (m_seeking) {
        m_pendingSeek = true;
        return;
    }

    GstState state;
    gst_element_get_state(m_playBin, &state, 0, 0);
    if (state != GST_STATE_PAUSED && state != GST_STATE_PLAYING)
        return;

    float seconds;
    float microSeconds = modff(time, &seconds) * 1000000;
    GTimeVal timeValue;
    timeValue.tv_sec = static_cast<glong>(seconds);
    timeValue.tv_usec = static_cast<glong>(roundf(microSeconds / 10000) * 10000);

    if (!gst_element_seek(m_playBin, m_playbackRate, GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
                          GST_SEEK_TYPE_SET, GST_TIMEVAL_TO_TIME(timeValue), GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
        return;
    }

    m_seeking = true;
}

bool MediaPlayer::seeking() const
{
    return m_seeking;
}

float MediaPlayer::maxTimeSeekable() const
{
    if (m_isLive)
        return numeric_limits<float>::infinity();

    return duration();
}

void MediaPlayer::setPlaybackRate(float playbackRate)
{
    m_playbackRate = playbackRate;
}

void MediaPlayer::setVolume(float volume)
{
    gst_stream_volume_set_volume(GST_STREAM_VOLUME(m_playBin), GST_STREAM_VOLUME_FORMAT_LINEAR, volume);
}

void MediaPlayer::setMuted(bool mute)
{
    gst_stream_volume_set_mute(GST_STREAM_VOLUME(m_playBin), mute);
}

bool MediaPlayer::isLiveStream() const
{
    return m_isLive;
}

void MediaPlayer::load(const char* url)
{
    gst_init_check(0, 0, 0);
    if (!m_playBin && !createPlayBin()) {
        // Could be a decode error as well, but just report a error is enough for now.
        m_playerClient->networkStateChanged(Nix::MediaPlayerClient::NetworkError);
        return;
    }

    g_object_set(m_playBin, "uri", url, NULL);

    gst_element_set_state(m_playBin, GST_STATE_PAUSED);
    setDownloadBuffering();
}

void MediaPlayer::onGstBusMessage(GstBus*, GstMessage* msg, MediaPlayer* self)
{
    GstElement* playBin = self->m_playBin;
    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
          GError *err;
          gchar *debug;
          gst_message_parse_error (msg, &err, &debug);
          GST_WARNING(err->message);
          g_error_free(err);
          g_free(debug);
          gst_element_set_state(playBin, GST_STATE_READY);
          break;
        }
    case GST_MESSAGE_EOS:
          gst_element_set_state(playBin, GST_STATE_READY);
          break;

    case GST_MESSAGE_BUFFERING: {
        if (self->m_isLive)
            return;

        int percent = 0;
        gst_message_parse_buffering(msg, &percent);
        if (percent < 100) {
            gst_element_set_state(playBin, GST_STATE_PAUSED);
        } else {
            // the duration is available now.
            self->m_playerClient->durationChanged();
            self->m_bufferingFinished = true;
            if (!self->m_paused)
               gst_element_set_state(playBin, GST_STATE_PLAYING);
        }
        break;
    }
    case GST_MESSAGE_CLOCK_LOST:
        if (!self->m_paused) {
            gst_element_set_state(playBin, GST_STATE_PAUSED);
            gst_element_set_state(playBin, GST_STATE_PLAYING);
        }
        break;
    case GST_MESSAGE_DURATION_CHANGED:
        self->m_playerClient->durationChanged();
        break;
    case GST_MESSAGE_STATE_CHANGED:
    case GST_MESSAGE_ASYNC_DONE:
        // Ignore state changes from internal elements. They are
        // forwarded to playbin anyway.
        if (GST_MESSAGE_SRC(msg) == reinterpret_cast<GstObject*>(self->m_playBin))
            self->updateStates();
        break;
    default:
        break;
    }
}

void MediaPlayer::updateStates()
{
    GstState state;
    GstState pending;

    GstStateChangeReturn ret = gst_element_get_state(m_playBin,
        &state, &pending, 250 * GST_NSECOND);

    Nix::MediaPlayerClient::ReadyState oldReadyState = m_readyState;
    Nix::MediaPlayerClient::NetworkState oldNetworkState = m_networkState;

    switch (ret) {
    case GST_STATE_CHANGE_FAILURE:
        GST_WARNING("Failed to change state");
        break;
    case GST_STATE_CHANGE_SUCCESS:
        m_isLive = false;

        switch (state) {
        case GST_STATE_READY:
            m_readyState = Nix::MediaPlayerClient::HaveMetadata;
            m_networkState = Nix::MediaPlayerClient::Empty;
            break;
        case GST_STATE_PAUSED:
        case GST_STATE_PLAYING:
            if (m_seeking) {
                m_seeking = false;
                m_playerClient->currentTimeChanged();
            }

            if (m_bufferingFinished) {
                m_readyState = Nix::MediaPlayerClient::HaveEnoughData;
                m_networkState = Nix::MediaPlayerClient::Loaded;
            } else {
                m_readyState = Nix::MediaPlayerClient::HaveCurrentData;
                m_networkState = Nix::MediaPlayerClient::Loading;
            }

            if (m_pendingSeek) {
                m_pendingSeek = false;
                seek(m_seekTime);
            }
            break;
        default:
            break;
        }
        break;
    case GST_STATE_CHANGE_NO_PREROLL:
        m_isLive = true;
        setDownloadBuffering();

        if (state == GST_STATE_PAUSED) {
            m_readyState = Nix::MediaPlayerClient::HaveEnoughData;
            m_networkState = Nix::MediaPlayerClient::Loading;
        }
        break;
    default:
        break;
    }

    if (oldReadyState != m_readyState)
        m_playerClient->readyStateChanged(m_readyState);

    if (oldNetworkState != m_networkState)
        m_playerClient->networkStateChanged(m_networkState);
}

bool MediaPlayer::createPlayBin()
{
    assert(!m_playBin);

    m_playBin = gst_element_factory_make("playbin", "play");
    if (!m_playBin)
        return false;

    // Audio sink
    GstElement* scale = gst_element_factory_make("scaletempo", 0);
    if (!scale) {
        GST_WARNING("Failed to create scaletempo");
        return false;
    }

    GstElement* convert = gst_element_factory_make("audioconvert", 0);
    GstElement* resample = gst_element_factory_make("audioresample", 0);
    GstElement* sink = gst_element_factory_make("autoaudiosink", 0);

    GstElement* audioSink = gst_bin_new("audio-sink");
    gst_bin_add_many(GST_BIN(audioSink), scale, convert, resample, sink, NULL);

    if (!gst_element_link_many(scale, convert, resample, sink, NULL)) {
        GST_WARNING("Failed to link audio sink elements");
        gst_object_unref(audioSink);
        return false;
    }

    GstPad* pad = gst_element_get_static_pad(scale, "sink");
    gst_element_add_pad(audioSink, gst_ghost_pad_new("sink", pad));

    g_object_set(m_playBin, "audio-sink", audioSink, NULL);
    gst_object_unref(pad);

    GstBus* bus = gst_element_get_bus(m_playBin);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message", G_CALLBACK(onGstBusMessage), this);
    gst_object_unref(bus);
    return true;
}

void MediaPlayer::setDownloadBuffering()
{
    assert(m_playBin);

    GstPlayFlags flags;
    g_object_get(m_playBin, "flags", &flags, NULL);
    // TODO add support for "auto" downloading...
    if (m_isLive)
        g_object_set(m_playBin, "flags", flags & ~GST_PLAY_FLAG_DOWNLOAD, NULL);
    else
        g_object_set(m_playBin, "flags", flags | GST_PLAY_FLAG_DOWNLOAD, NULL);
}
