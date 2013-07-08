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

#include "UserMediaClient.h"
#include <NixPlatform/UserMediaRequest.h>

#include <cstdio>

UserMediaClient::UserMediaClient()
{
    printf("[%s] %p\n", __PRETTY_FUNCTION__, this);
}

UserMediaClient::~UserMediaClient()
{
    printf("[%s] %p\n", __PRETTY_FUNCTION__, this);
}

void UserMediaClient::pageDestroyed()
{
    printf("[%s] %p\n", __PRETTY_FUNCTION__, this);
    delete this;
}

void UserMediaClient::requestUserMedia(const Nix::UserMediaRequest& request, const Nix::Vector<Nix::MediaStreamSource>& audioSources, const Nix::Vector<Nix::MediaStreamSource>& videoSources)
{
    printf("[%s] %p -- audio: %d, video: %d\n", __PRETTY_FUNCTION__, this, request.audio(), request.video());
}

void UserMediaClient::cancelUserMediaRequest(const Nix::UserMediaRequest& request)
{
    printf("[%s] %p -- audio: %d, video: %d\n", __PRETTY_FUNCTION__, this, request.audio(), request.video());
}