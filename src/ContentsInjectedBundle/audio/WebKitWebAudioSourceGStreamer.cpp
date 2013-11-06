/*
 *  Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
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

#include "WebKitWebAudioSourceGStreamer.h"
#include "AudioLiveInputPipeline.h"

#include <cstdlib>
#include <vector>

#include <gst/pbutils/pbutils.h>
#include <gst/audio/audio.h>
#include <NixPlatform/Platform.h>

typedef struct _WebKitWebAudioSrcClass   WebKitWebAudioSrcClass;
typedef struct _WebKitWebAudioSourcePrivate WebKitWebAudioSourcePrivate;

struct _WebKitWebAudioSrc {
    GstBin parent;

    WebKitWebAudioSourcePrivate* priv;
};

struct _WebKitWebAudioSrcClass {
    GstBinClass parentClass;
};

struct _WebKitWebAudioSourcePrivate {
    gfloat sampleRate;
    Nix::AudioDevice::RenderCallback* handler;
    guint framesToPull;

    guint numberOfOutputChannels;
    guint numberOfInputChannels;
    gboolean isLive;
    AudioLiveInputPipeline* inputPipeline;

    GstElement* interleave;
    GstElement* wavEncoder;

    GstTask* task;
    GRecMutex mutex;
    GSList* pads; // List of queue sink pads. One queue for each planar audio channel.
    GstPad* sourcePad; // src pad of the element, interleaved wav data is pushed to it.
};

enum {
    PROP_RATE = 1,
    PROP_OUTPUT_CHANNELS,
    PROP_INPUT_CHANNELS,
    PROP_IS_LIVE,
    PROP_HANDLER,
    PROP_FRAMES
};

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  GST_STATIC_CAPS("audio/x-wav"));

GST_DEBUG_CATEGORY_STATIC(webkit_web_audio_src_debug);
#define GST_CAT_DEFAULT webkit_web_audio_src_debug

static void webKitWebAudioSrcConstructed(GObject*);
static void webKitWebAudioSrcFinalize(GObject*);
static void webKitWebAudioSrcSetProperty(GObject*, guint propertyId, const GValue*, GParamSpec*);
static void webKitWebAudioSrcGetProperty(GObject*, guint propertyId, GValue*, GParamSpec*);
static GstStateChangeReturn webKitWebAudioSrcChangeState(GstElement*, GstStateChange);
static void webKitWebAudioSrcLoop(WebKitWebAudioSrc*);
static gboolean webKitWebAudioSrcQuery(GstPad* pad, GstObject* parent, GstQuery* query);

static GstCaps* getGStreamerMonoAudioCaps(float sampleRate)
{
    return gst_caps_new_simple("audio/x-raw", "rate", G_TYPE_INT, static_cast<int>(sampleRate),
                               "channels", G_TYPE_INT, 1,
                               "format", G_TYPE_STRING, gst_audio_format_to_string(GST_AUDIO_FORMAT_F32),
                               "layout", G_TYPE_STRING, "non-interleaved", NULL);
}

// XXX: From AudioBus.h (we don't have it), neeeded for the next function. Put this in a better place?
enum {
    ChannelLeft = 0,
    ChannelRight = 1,
    ChannelCenter = 2, // center and mono are the same
    ChannelMono = 2,
    ChannelLFE = 3,
    ChannelSurroundLeft = 4,
    ChannelSurroundRight = 5,
};

// XXX: The whole next function can handle both gstreamer-1.0 and 0.10, which is why it has an #ifdef
// in the middle of it. It was enclosed in this 'outer' #ifdef because it was not needed in the
// 0.10 build and hence there was an unused function warning.
static GstAudioChannelPosition webKitWebAudioGStreamerChannelPosition(int channelIndex)
{
    GstAudioChannelPosition position = GST_AUDIO_CHANNEL_POSITION_NONE;

    switch (channelIndex) {
    case ChannelLeft:
        position = GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
        break;
    case ChannelRight:
        position = GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
        break;
    case ChannelCenter:
        position = GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER;
        break;
    case ChannelLFE:
        position = GST_AUDIO_CHANNEL_POSITION_LFE1;
        break;
    case ChannelSurroundLeft:
        position = GST_AUDIO_CHANNEL_POSITION_REAR_LEFT;
        break;
    case ChannelSurroundRight:
        position = GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT;
        break;
    default:
        break;
    };

    return position;
}

#define webkit_web_audio_src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(WebKitWebAudioSrc, webkit_web_audio_src, GST_TYPE_BIN, GST_DEBUG_CATEGORY_INIT(webkit_web_audio_src_debug, \
                            "webkitwebaudiosrc", \
                            0, \
                            "webaudiosrc element"));

static void webkit_web_audio_src_class_init(WebKitWebAudioSrcClass* webKitWebAudioSrcClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(webKitWebAudioSrcClass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(webKitWebAudioSrcClass);

    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));
    gst_element_class_set_details_simple(elementClass,
                                         "WebKit WebAudio source element",
                                         "Source",
                                         "Handles WebAudio data from WebCore",
                                         "Philippe Normand <pnormand@igalia.com>");

    objectClass->constructed = webKitWebAudioSrcConstructed;
    objectClass->finalize = webKitWebAudioSrcFinalize;
    elementClass->change_state = webKitWebAudioSrcChangeState;

    objectClass->set_property = webKitWebAudioSrcSetProperty;
    objectClass->get_property = webKitWebAudioSrcGetProperty;

    GParamFlags flags = static_cast<GParamFlags>(G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    g_object_class_install_property(objectClass,
                                    PROP_RATE,
                                    g_param_spec_float("rate", "rate",
                                                       "Sample rate", G_MINDOUBLE, G_MAXDOUBLE,
                                                       44100.0, flags));

    g_object_class_install_property(objectClass,
                                    PROP_OUTPUT_CHANNELS,
                                    g_param_spec_uint("output-channels", "output-channels",
                                                      "Number of output audio channels",
                                                      1, G_MAXUINT8, 128, flags));

    g_object_class_install_property(objectClass,
                                    PROP_INPUT_CHANNELS,
                                    g_param_spec_uint("input-channels", "input-channels",
                                                      "Number of input audio channels",
                                                      0, G_MAXUINT8, 128, flags));

    g_object_class_install_property(objectClass,
                                    PROP_IS_LIVE,
                                    g_param_spec_boolean("is-live", "is-live",
                                                      "Configures live/non-live mode",
                                                      false, flags));

    g_object_class_install_property(objectClass,
                                    PROP_HANDLER,
                                    g_param_spec_pointer("handler", "handler",
                                                         "Handler", flags));

    g_object_class_install_property(objectClass,
                                    PROP_FRAMES,
                                    g_param_spec_uint("frames", "frames",
                                                      "Number of audio frames to pull at each iteration",
                                                      0, G_MAXUINT8, 128, flags));

    g_type_class_add_private(webKitWebAudioSrcClass, sizeof(WebKitWebAudioSourcePrivate));
}

static GstPad* webkitGstGhostPadFromStaticTemplate(GstStaticPadTemplate* staticPadTemplate, const gchar* name, GstPad* target)
{
    GstPad* pad;
    GstPadTemplate* padTemplate = gst_static_pad_template_get(staticPadTemplate);

    if (target)
        pad = gst_ghost_pad_new_from_template(name, target, padTemplate);
    else
        pad = gst_ghost_pad_new_no_target_from_template(name, padTemplate);

    gst_object_unref(padTemplate);
    return pad;
}

static void webkit_web_audio_src_init(WebKitWebAudioSrc* src)
{
    WebKitWebAudioSourcePrivate* priv = G_TYPE_INSTANCE_GET_PRIVATE(src, WEBKIT_TYPE_WEB_AUDIO_SRC, WebKitWebAudioSourcePrivate);
    src->priv = priv;
    new (priv) WebKitWebAudioSourcePrivate();

    priv->sourcePad = webkitGstGhostPadFromStaticTemplate(&srcTemplate, "src", 0);
    gst_pad_set_query_function(priv->sourcePad, webKitWebAudioSrcQuery);
    gst_element_add_pad(GST_ELEMENT(src), priv->sourcePad);

    priv->handler = 0;

    g_rec_mutex_init(&priv->mutex);
    priv->task = gst_task_new(reinterpret_cast<GstTaskFunction>(webKitWebAudioSrcLoop), src, 0);
    gst_task_set_lock(priv->task, &priv->mutex);
}

static void webKitWebAudioSrcConstructed(GObject* object)
{
    WebKitWebAudioSrc* src = WEBKIT_WEB_AUDIO_SRC(object);
    WebKitWebAudioSourcePrivate* priv = src->priv;

    priv->inputPipeline = (priv->isLive) ? new AudioLiveInputPipeline(priv->sampleRate) : nullptr;
    priv->interleave = gst_element_factory_make("interleave", 0);
    priv->wavEncoder = gst_element_factory_make("wavenc", 0);

    if (!priv->interleave) {
        GST_ERROR_OBJECT(src, "Failed to create interleave");
        return;
    }

    if (!priv->wavEncoder) {
        GST_ERROR_OBJECT(src, "Failed to create wavenc");
        return;
    }

    gst_bin_add_many(GST_BIN(src), priv->interleave, priv->wavEncoder, NULL);
    gst_element_link_pads_full(priv->interleave, "src", priv->wavEncoder, "sink", GST_PAD_LINK_CHECK_NOTHING);

    // For each channel of the bus create a new upstream branch for interleave, like:
    // queue ! capsfilter ! audioconvert. which is plugged to a new interleave request sinkpad.
    for (unsigned channelIndex = 0; channelIndex < priv->numberOfOutputChannels; ++channelIndex) {
        GstElement* queue = gst_element_factory_make("queue", 0);
        GstElement* capsfilter = gst_element_factory_make("capsfilter", 0);
        GstElement* audioconvert = gst_element_factory_make("audioconvert", 0);

        GstCaps* monoCaps = getGStreamerMonoAudioCaps(priv->sampleRate);

        GstAudioInfo info;
        gst_audio_info_from_caps(&info, monoCaps);
        GST_AUDIO_INFO_POSITION(&info, 0) = webKitWebAudioGStreamerChannelPosition(channelIndex);
        GstCaps* caps = gst_audio_info_to_caps(&info);
        g_object_set(capsfilter, "caps", caps, NULL);

        // Configure the queue for minimal latency.
        g_object_set(queue, "max-size-buffers", static_cast<guint>(1), NULL);

        GstPad* pad = gst_element_get_static_pad(queue, "sink");
        priv->pads = g_slist_prepend(priv->pads, pad);

        gst_bin_add_many(GST_BIN(src), queue, capsfilter, audioconvert, NULL);
        gst_element_link_pads_full(queue, "src", capsfilter, "sink", GST_PAD_LINK_CHECK_NOTHING);
        gst_element_link_pads_full(capsfilter, "src", audioconvert, "sink", GST_PAD_LINK_CHECK_NOTHING);
        gst_element_link_pads_full(audioconvert, "src", priv->interleave, 0, GST_PAD_LINK_CHECK_NOTHING);

    }
    priv->pads = g_slist_reverse(priv->pads);

    // wavenc's src pad is the only visible pad of our element.
    GstPad* targetPad = gst_element_get_static_pad(priv->wavEncoder, "src");
    gst_ghost_pad_set_target(GST_GHOST_PAD(priv->sourcePad), targetPad);
}

static void webKitWebAudioSrcFinalize(GObject* object)
{
    WebKitWebAudioSrc* src = WEBKIT_WEB_AUDIO_SRC(object);
    WebKitWebAudioSourcePrivate* priv = src->priv;

    if (priv->inputPipeline)
        delete priv->inputPipeline;

    g_rec_mutex_clear(&priv->mutex);
    g_slist_free_full(priv->pads, reinterpret_cast<GDestroyNotify>(gst_object_unref));

    priv->~WebKitWebAudioSourcePrivate();
    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, ((GObject* )(src)));
}

static void webKitWebAudioSrcSetProperty(GObject* object, guint propertyId, const GValue* value, GParamSpec* pspec)
{
    WebKitWebAudioSrc* src = WEBKIT_WEB_AUDIO_SRC(object);
    WebKitWebAudioSourcePrivate* priv = src->priv;

    switch (propertyId) {
    case PROP_RATE:
        priv->sampleRate = g_value_get_float(value);
        break;
    case PROP_OUTPUT_CHANNELS:
        priv->numberOfOutputChannels = g_value_get_uint(value);
        break;
    case PROP_INPUT_CHANNELS:
        priv->numberOfInputChannels = g_value_get_uint(value);
        break;
    case PROP_IS_LIVE:
        priv->isLive = g_value_get_boolean(value);;
        break;
    case PROP_HANDLER:
        priv->handler = static_cast<Nix::AudioDevice::RenderCallback*>(g_value_get_pointer(value));
        break;
    case PROP_FRAMES:
        priv->framesToPull = g_value_get_uint(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyId, pspec);
        break;
    }
}

static void webKitWebAudioSrcGetProperty(GObject* object, guint propertyId, GValue* value, GParamSpec* pspec)
{
    WebKitWebAudioSrc* src = WEBKIT_WEB_AUDIO_SRC(object);
    WebKitWebAudioSourcePrivate* priv = src->priv;

    switch (propertyId) {
    case PROP_RATE:
        g_value_set_float(value, priv->sampleRate);
        break;
    case PROP_OUTPUT_CHANNELS:
        g_value_set_uint(value, priv->numberOfOutputChannels);
        break;
    case PROP_INPUT_CHANNELS:
        g_value_set_uint(value, priv->numberOfInputChannels);
        break;
    case PROP_IS_LIVE:
        g_value_set_boolean(value, priv->isLive);
        break;
    case PROP_HANDLER:
        g_value_set_pointer(value, priv->handler);
        break;
    case PROP_FRAMES:
        g_value_set_uint(value, priv->framesToPull);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyId, pspec);
        break;
    }
}

static inline float* mapBuffer(GstBuffer* buffer) {
    float * ret = nullptr;
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_READ);
    ret = reinterpret_cast<float*>(info.data);
    gst_buffer_unmap(buffer, &info);
    return ret;
}

static void webKitWebAudioSrcLoop(WebKitWebAudioSrc* src)
{
    WebKitWebAudioSourcePrivate* priv = src->priv;

    if (!priv->handler) {
        GST_WARNING_OBJECT(src, "No handler, ignoring.");
        return;
    }

    GSList* inputBufferList = nullptr;
    std::vector<float*> audioDataVector((size_t) priv->numberOfOutputChannels);
    std::vector<float*> sourceDataVector;

    if (priv->isLive) {
        // Collect input data.
        if (!priv->inputPipeline->isReady()) {
            GST_WARNING_OBJECT(src, "Live-input not yet ready!");
            return;
        }

        sourceDataVector.resize(priv->numberOfInputChannels);
        guint inputChannels = priv->inputPipeline->pullChannelBuffers(&inputBufferList);
        if (inputChannels < priv->numberOfInputChannels) {
            GST_WARNING("Couldn't get input buffers for all requested channels!");
            return;
        }

        GSList* inbufIt = inputBufferList;
        GstBuffer* inbuf;

        for(int i = 0; inbufIt != nullptr; ++i, inbufIt = g_slist_next(inbufIt)) {
            inbuf = static_cast<GstBuffer*>(inbufIt->data);
            sourceDataVector[i] = mapBuffer(inbuf);
        }
    }

    guint bufferSize = priv->framesToPull * sizeof(float);
    GSList* channelBufferList = nullptr;
    GstBuffer* channelBuffer;
    for (int i = priv->numberOfOutputChannels - 1; i >= 0; --i) {
        channelBuffer = gst_buffer_new_and_alloc(bufferSize);
        channelBufferList = g_slist_prepend(channelBufferList, channelBuffer);
        audioDataVector[i] = mapBuffer(channelBuffer);
    }

    priv->handler->render(sourceDataVector, audioDataVector, priv->framesToPull);

    for (int i = priv->numberOfOutputChannels - 1; i >= 0; --i) {
        GstPad* pad = static_cast<GstPad*>(g_slist_nth_data(priv->pads, i));
        channelBuffer = static_cast<GstBuffer*>(g_slist_nth_data(channelBufferList, i));

        GstFlowReturn ret = gst_pad_chain(pad, channelBuffer);
        if (ret != GST_FLOW_OK) {
            GST_ELEMENT_ERROR(src, CORE, PAD, ("Internal WebAudioSrc error"),
                ("Failed to push buffer on %s", GST_DEBUG_PAD_NAME(pad)));
        }
    }

    g_slist_free(channelBufferList);
    if (priv->isLive && inputBufferList)
        g_slist_free_full(inputBufferList, reinterpret_cast<GDestroyNotify>(gst_buffer_unref));
}

static GstStateChangeReturn webKitWebAudioSrcChangeState(GstElement* element, GstStateChange transition)
{
    GstStateChangeReturn returnValue = GST_STATE_CHANGE_SUCCESS;
    WebKitWebAudioSrc* src = WEBKIT_WEB_AUDIO_SRC(element);

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        if (!src->priv->interleave) {
            gst_element_post_message(element, gst_missing_element_message_new(element, "interleave"));
            GST_ELEMENT_ERROR(src, CORE, MISSING_PLUGIN, (0), ("no interleave"));
            return GST_STATE_CHANGE_FAILURE;
        }
        if (!src->priv->wavEncoder) {
            gst_element_post_message(element, gst_missing_element_message_new(element, "wavenc"));
            GST_ELEMENT_ERROR(src, CORE, MISSING_PLUGIN, (0), ("no wavenc"));
            return GST_STATE_CHANGE_FAILURE;
        }
        break;
    default:
        break;
    }

    returnValue = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    if (returnValue == GST_STATE_CHANGE_FAILURE) {
        GST_WARNING_OBJECT(src, "State change failed");
        return returnValue;
    }

    switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        GST_DEBUG_OBJECT(src, "PAUSED->PLAYING");
        if (src->priv->isLive) {
            if (!gst_task_start(src->priv->task))
                returnValue = GST_STATE_CHANGE_FAILURE;
        }
        break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        GST_DEBUG_OBJECT(src, "READY->PAUSED");
        // When processing live-input it starts the gsttask when
        // moving from PAUSED to PLAYING and returns NO_PREROLL.
        if (src->priv->isLive)
            returnValue = GST_STATE_CHANGE_NO_PREROLL;
        else {
            if (!gst_task_start(src->priv->task))
                returnValue = GST_STATE_CHANGE_FAILURE;
        }
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        GST_DEBUG_OBJECT(src, "PAUSED->READY");
        if (!gst_task_join(src->priv->task))
            returnValue = GST_STATE_CHANGE_FAILURE;
        break;
    default:
        break;
    }

    return returnValue;
}

static gboolean webKitWebAudioSrcQuery(GstPad* pad, GstObject* parent, GstQuery* query)
{
    WebKitWebAudioSrc* src = WEBKIT_WEB_AUDIO_SRC(parent);
    gboolean ret;

    if (!src->priv->isLive)
        return gst_pad_query_default(pad, parent, query);

    switch (GST_QUERY_TYPE (query)) {
      case GST_QUERY_LATENCY:
        GST_DEBUG_OBJECT(src, "Processing latency query");
        ret = src->priv->inputPipeline->sendQuery(query);
        break;
      default:
        GST_DEBUG_OBJECT(src, "Processing query");
        ret = gst_pad_query_default(pad, parent, query);
        break;
    }
    return ret;
}
