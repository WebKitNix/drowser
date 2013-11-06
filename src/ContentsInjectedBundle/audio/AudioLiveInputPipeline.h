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

#ifndef AudioLiveInputPipeline_h
#define AudioLiveInputPipeline_h

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/audio/audio.h>
#include <gst/pbutils/pbutils.h>

class AudioLiveInputPipeline {
public:
    typedef void (*InputReadyCallback)();

    AudioLiveInputPipeline(float sampleRate);
    ~AudioLiveInputPipeline();

    GstStateChangeReturn start();
    gboolean sendQuery(GstQuery* query);
    bool isReady() { return m_ready; }

    guint pullChannelBuffers(GSList** bufferList);

    void handleNewDeinterleavePad(GstPad*);
    void deinterleavePadsConfigured();
    GstFlowReturn handlePullRequest(GstAppSink* sink);

private:
    void buildInputPipeline();
    GstBuffer* pullNewBuffer(GstAppSink* sink);

    GstElement* m_pipeline;
    GstElement* m_source;
    GstElement* m_deInterleave;
    GSList* m_sinkList;
    float m_sampleRate;
    bool m_ready;
};

#endif //AudioLiveInputPipeline_h

