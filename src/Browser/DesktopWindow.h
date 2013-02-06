/*
 * Copyright (C) 2012-2013 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DesktopWindow_h
#define DesktopWindow_h

#include <WebKit2/WKGeometry.h>
#include <NIXEvents.h>

class DesktopWindowClient
{
public:
    virtual void onWindowExpose() = 0;
    virtual void onKeyPress(NIXKeyEvent*) = 0;
    virtual void onKeyRelease(NIXKeyEvent*) = 0;
    virtual void onMousePress(NIXMouseEvent*) = 0;
    virtual void onMouseRelease(NIXMouseEvent*) = 0;
    virtual void onMouseMove(NIXMouseEvent*) = 0;
    virtual void onMouseWheel(NIXWheelEvent*) = 0;

    virtual void onWindowSizeChange(WKSize) = 0;
    virtual void onWindowClose() = 0;
};

class DesktopWindow
{
public:
    virtual ~DesktopWindow();

    static DesktopWindow* create(DesktopWindowClient* client, int width, int height);

    WKSize size() const { return m_size; }

    virtual void makeCurrent() = 0;
    virtual void swapBuffers() = 0;
protected:
    DesktopWindowClient* m_client;
    WKSize m_size;

    DesktopWindow(DesktopWindowClient* client, int width, int height);
};

#endif
