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

#include "FFTGStreamer.h"
#include "PlatformClient.h"

#include <glib.h>
G_BEGIN_DECLS
#include <gst/fft/gstfft.h>
#include <gst/fft/gstfftf32.h>
G_END_DECLS

#include <cstring>

using namespace WebKit;

WebKit::WebFFTFrame* PlatformClient::createFFTFrame(unsigned fftSize)
{
    return new FFTGStreamer(fftSize);
}

WebKit::WebFFTFrame* PlatformClient::createFFTFrame(const WebKit::WebFFTFrame* frame)
{
    return new FFTGStreamer(*frame);
}

unsigned frequencyDomainSize(unsigned fftSize)
{
    return fftSize / 2 + 1;
}

FFTGStreamer::FFTGStreamer(unsigned fftSize)
    : m_fftSize(fftSize)
    , m_frequencyDomainSize(frequencyDomainSize(m_fftSize))
{
    m_complexData = new GstFFTF32Complex[m_frequencyDomainSize];
    m_realData = new float[m_frequencyDomainSize];
    m_imagData = new float[m_frequencyDomainSize];

    int fftLength = gst_fft_next_fast_length(m_fftSize);
    m_forward = gst_fft_f32_new(fftLength, FALSE);
    m_inverse = gst_fft_f32_new(fftLength, TRUE);
}

FFTGStreamer::FFTGStreamer(const WebFFTFrame& frame)
    : m_fftSize(0)
{
    const FFTGStreamer* other_frame = static_cast<const FFTGStreamer*>(&frame);
    m_fftSize = other_frame->m_fftSize;
    m_frequencyDomainSize = other_frame->m_frequencyDomainSize;

    m_complexData = new GstFFTF32Complex[m_frequencyDomainSize];
    memcpy(m_complexData, other_frame->m_complexData, sizeof(GstFFTF32Complex)*m_frequencyDomainSize);

    m_realData = new float[m_frequencyDomainSize];
    memcpy(m_realData, other_frame->m_realData, sizeof(float)*m_frequencyDomainSize);

    m_imagData = new float[m_frequencyDomainSize];
    memcpy(m_imagData, other_frame->m_imagData, sizeof(float)*m_frequencyDomainSize);

    int fftLength = gst_fft_next_fast_length(m_fftSize);
    m_forward = gst_fft_f32_new(fftLength, FALSE);
    m_inverse = gst_fft_f32_new(fftLength, TRUE);
}

FFTGStreamer::~FFTGStreamer()
{
    delete m_complexData;
    delete m_realData;
    delete m_imagData;

    if (m_forward)
        gst_fft_f32_free(m_forward);

    if (m_inverse)
        gst_fft_f32_free(m_inverse);
}

void FFTGStreamer::doFFT(const float* data)
{
    gst_fft_f32_fft(m_forward, data, m_complexData);

    updatePlanarData();
}

void FFTGStreamer::doInverseFFT(float* data)
{
    updateComplexData();
    gst_fft_f32_inverse_fft(m_inverse, m_complexData, data);
}

void FFTGStreamer::multiply(const WebFFTFrame& frame)
{
    // Multiply both frames element-wise. In the ideal case we'd implement this using
    // optimized SSE2 or ARM instructions.

    const FFTGStreamer* other_frame = static_cast<const FFTGStreamer*>(&frame);

    float* real1P = realData();
    float* imag1P = imagData();

    const float* real2P = other_frame->realData();
    const float* imag2P = other_frame->imagData();

    const unsigned framesToProcess = m_frequencyDomainSize;

    for (int i = 0; i < framesToProcess; ++i) {
        float realResult = real1P[i] * real2P[i] - imag1P[i] * imag2P[i];
        float imagResult = real1P[i] * imag2P[i] + imag1P[i] * real2P[i];

        real1P[i] = realResult;
        imag1P[i] = imagResult;
    }
}

unsigned FFTGStreamer::frequencyDomainSampleCount() const
{
    return m_frequencyDomainSize;
}

float* FFTGStreamer::realData() const
{
    return m_realData;
}

float* FFTGStreamer::imagData() const
{
    return m_imagData;
}

void FFTGStreamer::updatePlanarData()
{
    for (int i = 0; i < m_frequencyDomainSize; ++i) {
        m_realData[i] = m_complexData[i].r;
        m_imagData[i] = m_complexData[i].i;
    }
}

void FFTGStreamer::updateComplexData()
{
    for (int i = 0; i < m_frequencyDomainSize; ++i) {
        m_complexData[i].r = m_realData[i];
        m_complexData[i].i = m_imagData[i];
    }
}
