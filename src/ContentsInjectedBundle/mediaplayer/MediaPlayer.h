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

#ifndef MediaPlayer_h
#define MediaPlayer_h

#include <NixPlatform/MediaPlayer.h>
#include <gst/gst.h>

class MediaPlayer : public Nix::MediaPlayer
{
public:
    MediaPlayer(Nix::MediaPlayerClient*);
    virtual ~MediaPlayer();
    virtual void play() override;
    virtual void pause() override;
    virtual float duration() const override;
    virtual float currentTime() const override;
    virtual void seek(float) override;
    virtual void setVolume(float) override;
    virtual void setMuted(bool) override;
    virtual void load(const char* url) override;
    virtual bool seeking() const override;
    virtual float maxTimeSeekable() const override;
    virtual void setPlaybackRate(float) override;

private:
    GstElement* m_playBin;
    bool m_paused;
    bool m_isLive;
    bool m_seeking;
    bool m_pendingSeek;
    float m_playbackRate;
    float m_seekTime;

    bool createPlayBin();
    void setDownloadBuffering();
    void updateStates();

    static void onGstBusMessage(GstBus*, GstMessage*, MediaPlayer*);
};

#endif // MediaPlayer_h
