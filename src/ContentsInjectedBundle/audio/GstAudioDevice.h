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

#ifndef GstAudioDevice_h
#define GstAudioDevice_h

#include <gst/gst.h>
#include <NixPlatform/Platform.h>

class GstAudioDevice : public Nix::AudioDevice {
public:
    GstAudioDevice(const char* inputDeviceId, size_t bufferSize, unsigned numberOfInputChannels,
        unsigned numberOfOutputChannels, double sampleRate, Nix::AudioDevice::RenderCallback* renderCallback);
    virtual ~GstAudioDevice();

    virtual void start() override;
    virtual void stop() override;

    virtual double sampleRate() override { return m_sampleRate; }
    bool providesLiveInput() { return m_providesLiveInput; }

private:
    void finishBuildingPipelineAfterWavParserPadReady(GstPad*);

    bool m_wavParserAvailable;
    bool m_audioSinkAvailable;
    GstElement* m_pipeline;
    GstElement* m_audioSink;
    double m_sampleRate;
    size_t m_bufferSize;
    bool m_providesLiveInput;
    char* m_inputDeviceId;
    Nix::AudioDevice::RenderCallback* m_renderCallback;
};

#endif
