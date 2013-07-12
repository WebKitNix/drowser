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

#ifndef FFTGStreamer_h
#define FFTGStreamer_h

#include <NixPlatform/Platform.h>
#include <NixPlatform/FFTFrame.h>

#include <glib.h>
G_BEGIN_DECLS
#include <gst/fft/gstfft.h>
#include <gst/fft/gstfftf32.h>
G_END_DECLS

class FFTGStreamer : public Nix::FFTFrame {
public:
    FFTGStreamer(unsigned fftSize);
    FFTGStreamer(const FFTFrame& frame);
    ~FFTGStreamer();

    virtual void doFFT(const float* data);
    virtual void doInverseFFT(float* data);
    virtual void multiply(const FFTFrame& frame); // multiplies ourself with frame : effectively operator*=()

    virtual unsigned frequencyDomainSampleCount() const;
    virtual float* realData() const;
    virtual float* imagData() const;

private:

    void updatePlanarData();
    void updateComplexData();

    unsigned m_fftSize;
    unsigned m_frequencyDomainSize;
    GstFFTF32* m_forward;
    GstFFTF32* m_inverse;

    GstFFTF32Complex* m_complexData;
    float* m_realData; // Using float while inside WebKit we'd use ArrayFloat
    float* m_imagData;
};

#endif
