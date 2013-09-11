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

#include "MediaStreamCenter.h"
#include <NixPlatform/MediaStreamCenter.h>

#include <cstdio>
#include <glib.h>
#include <gst/gst.h>

MediaStreamCenter::MediaStreamCenter()
{
    printf("[%s] %p\n", __PRETTY_FUNCTION__, this);
}

MediaStreamCenter::~MediaStreamCenter()
{
    printf("[%s] %p\n", __PRETTY_FUNCTION__, this);
}

void MediaStreamCenter::queryMediaStreamsSources(Nix::MediaStreamSourcesQueryClient& queryClient)
{
    printf("[%s] %p\n", __PRETTY_FUNCTION__, this);
    Nix::Vector<Nix::MediaStreamSource> audioSources;
    Nix::Vector<Nix::MediaStreamSource> videoSources;

    //*
    if (queryClient.audio()) {
        GstElement* audioSrc = gst_element_factory_make("autoaudiosrc", "autosrc");
        if (audioSrc) {
            audioSources = Nix::Vector<Nix::MediaStreamSource>((size_t) 1);

            char* deviceId = gst_element_get_name(gst_element_get_factory(audioSrc));
            char buffer[100];
            sprintf(buffer, "%s;default", deviceId);

            printf("[%s] %p %s\n", __PRETTY_FUNCTION__, this, buffer);

            audioSources[0].initialize(Nix::String("autosrc"), Nix::MediaStreamSource::TypeAudio, "default");
            audioSources[0].setDeviceId(Nix::String("autoaudiosrc;default"));
        }
    }
    //*/

    if (queryClient.video()) {
        // Not supported.
    }
    queryClient.didCompleteQuery(audioSources, videoSources);
}
