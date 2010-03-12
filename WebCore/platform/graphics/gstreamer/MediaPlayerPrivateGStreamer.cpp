/*
 * Copyright (C) 2007, 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Collabora Ltd.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (C) 2009, 2010 Igalia S.L
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * aint with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(VIDEO)

#include "MediaPlayerPrivateGStreamer.h"


#include "CString.h"
#include "DataSourceGStreamer.h"
#include "Document.h"
#include "Frame.h"
#include "FrameView.h"
#include "GOwnPtrGStreamer.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "KURL.h"
#include "MIMETypeRegistry.h"
#include "MediaPlayer.h"
#include "NotImplemented.h"
#include "ScrollView.h"
#include "SecurityOrigin.h"
#include "TimeRanges.h"
#include "VideoSinkGStreamer.h"
#include "WebKitWebSourceGStreamer.h"
#include "Widget.h"

#include <GOwnPtr.h>
#include <gst/gst.h>
#include <gst/interfaces/mixer.h>
#include <gst/interfaces/xoverlay.h>
#include <gst/video/video.h>
#include <limits>
#include <math.h>

// GstPlayFlags flags from playbin2. It is the policy of GStreamer to
// not publicly expose element-specific enums. That's why this
// GstPlayFlags enum has been copied here.
typedef enum {
    GST_PLAY_FLAG_VIDEO         = 0x00000001,
    GST_PLAY_FLAG_AUDIO         = 0x00000002,
    GST_PLAY_FLAG_TEXT          = 0x00000004,
    GST_PLAY_FLAG_VIS           = 0x00000008,
    GST_PLAY_FLAG_SOFT_VOLUME   = 0x00000010,
    GST_PLAY_FLAG_NATIVE_AUDIO  = 0x00000020,
    GST_PLAY_FLAG_NATIVE_VIDEO  = 0x00000040,
    GST_PLAY_FLAG_DOWNLOAD      = 0x00000080,
    GST_PLAY_FLAG_BUFFERING     = 0x000000100
} GstPlayFlags;

using namespace std;

namespace WebCore {

static int greatestCommonDivisor(int a, int b)
{
    while (b) {
        int temp = a;
        a = b;
        b = temp % b;
    }

    return ABS(a);
}

gboolean mediaPlayerPrivateMessageCallback(GstBus* bus, GstMessage* message, gpointer data)
{
    GOwnPtr<GError> err;
    GOwnPtr<gchar> debug;
    MediaPlayer::NetworkState error;
    MediaPlayerPrivate* mp = reinterpret_cast<MediaPlayerPrivate*>(data);
    bool issueError = true;
    bool attemptNextLocation = false;

    if (message->structure) {
        const gchar* messageTypeName = gst_structure_get_name(message->structure);

        // Redirect messages are sent from elements, like qtdemux, to
        // notify of the new location(s) of the media.
        if (!g_strcmp0(messageTypeName, "redirect")) {
            mp->mediaLocationChanged(message);
            return true;
        }
    }

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR:
        if (mp && mp->pipelineReset())
            break;
        gst_message_parse_error(message, &err.outPtr(), &debug.outPtr());
        LOG_VERBOSE(Media, "Error: %d, %s", err->code,  err->message);

        error = MediaPlayer::Empty;
        if (err->code == GST_STREAM_ERROR_CODEC_NOT_FOUND
            || err->code == GST_STREAM_ERROR_WRONG_TYPE
            || err->code == GST_STREAM_ERROR_FAILED
            || err->code == GST_CORE_ERROR_MISSING_PLUGIN
            || err->code == GST_RESOURCE_ERROR_NOT_FOUND)
            error = MediaPlayer::FormatError;
        else if (err->domain == GST_STREAM_ERROR) {
            error = MediaPlayer::DecodeError;
            attemptNextLocation = true;
        } else if (err->domain == GST_RESOURCE_ERROR)
            error = MediaPlayer::NetworkError;

        if (mp) {
            if (attemptNextLocation)
                issueError = !mp->loadNextLocation();
            if (issueError)
                mp->loadingFailed(error);
        }
        break;
    case GST_MESSAGE_EOS:
        LOG_VERBOSE(Media, "End of Stream");
        mp->didEnd();
        break;
    case GST_MESSAGE_STATE_CHANGED:
        mp->updateStates();
        break;
    case GST_MESSAGE_BUFFERING:
        mp->processBufferingStats(message);
        break;
    case GST_MESSAGE_DURATION:
        LOG_VERBOSE(Media, "Duration changed");
        mp->durationChanged();
        break;
    default:
        LOG_VERBOSE(Media, "Unhandled GStreamer message type: %s",
                    GST_MESSAGE_TYPE_NAME(message));
        break;
    }
    return true;
}

void mediaPlayerPrivateSourceChangedCallback(GObject *object, GParamSpec *pspec, gpointer data)
{
    MediaPlayerPrivate* mp = reinterpret_cast<MediaPlayerPrivate*>(data);
    GOwnPtr<GstElement> element;

    g_object_get(mp->m_playBin, "source", &element.outPtr(), NULL);
    gst_object_replace((GstObject**) &mp->m_source, (GstObject*) element.get());

    if (WEBKIT_IS_WEB_SRC(element.get())) {
        Frame* frame = mp->m_player->frameView() ? mp->m_player->frameView()->frame() : 0;

        if (frame)
            webKitWebSrcSetFrame(WEBKIT_WEB_SRC(element.get()), frame);
    }
}

void mediaPlayerPrivateVolumeChangedCallback(GObject *element, GParamSpec *pspec, gpointer data)
{
    // This is called when playbin receives the notify::volume signal.
    MediaPlayerPrivate* mp = reinterpret_cast<MediaPlayerPrivate*>(data);
    mp->volumeChanged();
}

gboolean notifyVolumeIdleCallback(gpointer data)
{
    MediaPlayerPrivate* mp = reinterpret_cast<MediaPlayerPrivate*>(data);
    mp->volumeChangedCallback();
    return FALSE;
}

void mediaPlayerPrivateMuteChangedCallback(GObject *element, GParamSpec *pspec, gpointer data)
{
    // This is called when playbin receives the notify::mute signal.
    MediaPlayerPrivate* mp = reinterpret_cast<MediaPlayerPrivate*>(data);
    mp->muteChanged();
}

gboolean notifyMuteIdleCallback(gpointer data)
{
    MediaPlayerPrivate* mp = reinterpret_cast<MediaPlayerPrivate*>(data);
    mp->muteChangedCallback();
    return FALSE;
}

gboolean bufferingTimeoutCallback(gpointer data)
{
    MediaPlayerPrivate* mp = reinterpret_cast<MediaPlayerPrivate*>(data);
    return mp->queryBufferingStats();
}

static float playbackPosition(GstElement* playbin)
{

    float ret = 0.0;

    GstQuery* query = gst_query_new_position(GST_FORMAT_TIME);
    if (!gst_element_query(playbin, query)) {
        LOG_VERBOSE(Media, "Position query failed...");
        gst_query_unref(query);
        return ret;
    }

    gint64 position;
    gst_query_parse_position(query, 0, &position);

    // Position is available only if the pipeline is not in GST_STATE_NULL or
    // GST_STATE_READY state.
    if (position !=  static_cast<gint64>(GST_CLOCK_TIME_NONE))
        ret = static_cast<float>(position) / static_cast<float>(GST_SECOND);

    LOG_VERBOSE(Media, "Position %" GST_TIME_FORMAT, GST_TIME_ARGS(position));

    gst_query_unref(query);

    return ret;
}


void mediaPlayerPrivateRepaintCallback(WebKitVideoSink*, GstBuffer *buffer, MediaPlayerPrivate* playerPrivate)
{
    g_return_if_fail(GST_IS_BUFFER(buffer));
    gst_buffer_replace(&playerPrivate->m_buffer, buffer);
    playerPrivate->repaint();
}

MediaPlayerPrivateInterface* MediaPlayerPrivate::create(MediaPlayer* player)
{
    return new MediaPlayerPrivate(player);
}

void MediaPlayerPrivate::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable())
        registrar(create, getSupportedTypes, supportsType);
}

static bool gstInitialized = false;

static bool doGstInit()
{
    // FIXME: We should pass the arguments from the command line
    if (!gstInitialized) {
        GOwnPtr<GError> error;
        gstInitialized = gst_init_check(0, 0, &error.outPtr());
        if (!gstInitialized) {
            LOG_VERBOSE(Media, "Could not initialize GStreamer: %s",
                        error ? error->message : "unknown error occurred");
        } else {
            gst_element_register(0, "webkitmediasrc", GST_RANK_PRIMARY,
                                 WEBKIT_TYPE_DATA_SRC);
            gst_element_register(0, "webkitwebsrc", GST_RANK_PRIMARY + 100,
                                 WEBKIT_TYPE_WEB_SRC);
        }

    }
    return gstInitialized;
}

bool MediaPlayerPrivate::isAvailable()
{
    if (!doGstInit())
        return false;

    GstElementFactory* factory = gst_element_factory_find("playbin2");
    if (factory) {
        gst_object_unref(GST_OBJECT(factory));
        return true;
    }
    return false;
}

MediaPlayerPrivate::MediaPlayerPrivate(MediaPlayer* player)
    : m_player(player)
    , m_playBin(0)
    , m_videoSink(0)
    , m_fpsSink(0)
    , m_source(0)
    , m_seekTime(0)
    , m_changingRate(false)
    , m_endTime(numeric_limits<float>::infinity())
    , m_networkState(MediaPlayer::Empty)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_isStreaming(false)
    , m_size(IntSize())
    , m_buffer(0)
    , m_mediaLocations(0)
    , m_mediaLocationCurrentIndex(0)
    , m_resetPipeline(false)
    , m_paused(true)
    , m_seeking(false)
    , m_buffering(false)
    , m_playbackRate(1)
    , m_errorOccured(false)
    , m_volumeIdleId(0)
    , m_mediaDuration(0)
    , m_muteIdleId(0)
    , m_startedBuffering(false)
    , m_fillTimeoutId(0)
    , m_maxTimeLoaded(0)
    , m_bufferingPercentage(0)
    , m_preload(MediaPlayer::Auto)
    , m_delayingLoad(false)
{
    if (doGstInit())
        createGSTPlayBin();
}

MediaPlayerPrivate::~MediaPlayerPrivate()
{
    if (m_fillTimeoutId) {
        g_source_remove(m_fillTimeoutId);
        m_fillTimeoutId = 0;
    }

    if (m_volumeIdleId) {
        g_source_remove(m_volumeIdleId);
        m_volumeIdleId = 0;
    }

    if (m_muteIdleId) {
        g_source_remove(m_muteIdleId);
        m_muteIdleId = 0;
    }

    if (m_buffer)
        gst_buffer_unref(m_buffer);
    m_buffer = 0;

    if (m_mediaLocations) {
        gst_structure_free(m_mediaLocations);
        m_mediaLocations = 0;
    }

    if (m_source) {
        gst_object_unref(m_source);
        m_source = 0;
    }

    if (m_playBin) {
        gst_element_set_state(m_playBin, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(m_playBin));
    }

    if (m_videoSink) {
        g_object_unref(m_videoSink);
        m_videoSink = 0;
    }

    if (m_fpsSink) {
        g_object_unref(m_fpsSink);
        m_fpsSink = 0;
    }
}

void MediaPlayerPrivate::load(const String& url)
{
    g_object_set(m_playBin, "uri", url.utf8().data(), NULL);

    if (m_preload == MediaPlayer::None) {
        m_delayingLoad = true;
        return;
    }

    commitLoad();
}

void MediaPlayerPrivate::commitLoad()
{
    // GStreamer needs to have the pipeline set to a paused state to
    // start providing anything useful.
    gst_element_set_state(m_playBin, GST_STATE_PAUSED);

    LOG_VERBOSE(Media, "Load %s", url.utf8().data());
    if (m_networkState != MediaPlayer::Loading) {
        m_networkState = MediaPlayer::Loading;
        m_player->networkStateChanged();
    }
    if (m_readyState != MediaPlayer::HaveNothing) {
        m_readyState = MediaPlayer::HaveNothing;
        m_player->readyStateChanged();
    }
}

bool MediaPlayerPrivate::changePipelineState(GstState newState)
{
    ASSERT(newState == GST_STATE_PLAYING || newState == GST_STATE_PAUSED);

    GstState currentState;
    GstState pending;

    gst_element_get_state(m_playBin, &currentState, &pending, 0);
    if (currentState != newState && pending != newState) {
        GstStateChangeReturn ret = gst_element_set_state(m_playBin, newState);
        GstState pausedOrPlaying = newState == GST_STATE_PLAYING ? GST_STATE_PAUSED : GST_STATE_PLAYING;
        if (currentState != pausedOrPlaying && ret == GST_STATE_CHANGE_FAILURE) {
            loadingFailed(MediaPlayer::Empty);
            return false;
        }
    }
    return true;
}

void MediaPlayerPrivate::prepareToPlay()
{
    if (m_delayingLoad) {
        m_delayingLoad = false;
        commitLoad();
    }
}

void MediaPlayerPrivate::play()
{
    if (changePipelineState(GST_STATE_PLAYING))
        LOG_VERBOSE(Media, "Play");
}

void MediaPlayerPrivate::pause()
{
    if (changePipelineState(GST_STATE_PAUSED))
        LOG_VERBOSE(Media, "Pause");
}

float MediaPlayerPrivate::duration() const
{
    if (!m_playBin)
        return 0.0;

    if (m_errorOccured)
        return 0.0;

    if (m_mediaDuration)
        return m_mediaDuration;

    GstFormat timeFormat = GST_FORMAT_TIME;
    gint64 timeLength = 0;

    if (!gst_element_query_duration(m_playBin, &timeFormat, &timeLength) || timeFormat != GST_FORMAT_TIME || static_cast<guint64>(timeLength) == GST_CLOCK_TIME_NONE) {
        LOG_VERBOSE(Media, "Time duration query failed.");
        return numeric_limits<float>::infinity();
    }

    LOG_VERBOSE(Media, "Duration: %" GST_TIME_FORMAT, GST_TIME_ARGS(timeLength));

    return (float) ((guint64) timeLength / 1000000000.0);
    // FIXME: handle 3.14.9.5 properly
}

float MediaPlayerPrivate::currentTime() const
{
    if (!m_playBin)
        return 0;

    if (m_errorOccured)
        return 0;

    if (m_seeking)
        return m_seekTime;

    return playbackPosition(m_playBin);

}

void MediaPlayerPrivate::seek(float time)
{
    // Avoid useless seeking.
    if (time == playbackPosition(m_playBin))
        return;

    if (!m_playBin)
        return;

    if (m_errorOccured)
        return;

    GstClockTime sec = (GstClockTime)(time * GST_SECOND);
    LOG_VERBOSE(Media, "Seek: %" GST_TIME_FORMAT, GST_TIME_ARGS(sec));
    if (!gst_element_seek(m_playBin, m_player->rate(),
            GST_FORMAT_TIME,
            (GstSeekFlags)(GST_SEEK_FLAG_FLUSH),
            GST_SEEK_TYPE_SET, sec,
            GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
        LOG_VERBOSE(Media, "Seek to %f failed", time);
    else {
        m_seeking = true;
        m_seekTime = sec;
    }
}

void MediaPlayerPrivate::startEndPointTimerIfNeeded()
{
    notImplemented();
}

void MediaPlayerPrivate::cancelSeek()
{
    notImplemented();
}

void MediaPlayerPrivate::endPointTimerFired(Timer<MediaPlayerPrivate>*)
{
    notImplemented();
}

bool MediaPlayerPrivate::paused() const
{
    return m_paused;
}

bool MediaPlayerPrivate::seeking() const
{
    return m_seeking;
}

// Returns the size of the video
IntSize MediaPlayerPrivate::naturalSize() const
{
    if (!hasVideo())
        return IntSize();

    GstPad* pad = gst_element_get_static_pad(m_videoSink, "sink");
    if (!pad)
        return IntSize();

    int width = 0, height = 0;
    GstCaps* caps = GST_PAD_CAPS(pad);
    int pixelAspectRatioNumerator, pixelAspectRatioDenominator;
    int displayWidth, displayHeight, displayAspectRatioGCD;
    int originalWidth = 0, originalHeight = 0;

    // TODO: handle possible clean aperture data. See
    // https://bugzilla.gnome.org/show_bug.cgi?id=596571
    // TODO: handle possible transformation matrix. See
    // https://bugzilla.gnome.org/show_bug.cgi?id=596326

    // Get the video PAR and original size.
    if (!GST_IS_CAPS(caps) || !gst_caps_is_fixed(caps)
        || !gst_video_format_parse_caps(caps, 0, &originalWidth, &originalHeight)
        || !gst_video_parse_caps_pixel_aspect_ratio(caps, &pixelAspectRatioNumerator,
                                                    &pixelAspectRatioDenominator)) {
        gst_object_unref(GST_OBJECT(pad));
        return IntSize();
    }

    gst_object_unref(GST_OBJECT(pad));

    LOG_VERBOSE(Media, "Original video size: %dx%d", originalWidth, originalHeight);
    LOG_VERBOSE(Media, "Pixel aspect ratio: %d/%d", pixelAspectRatioNumerator, pixelAspectRatioDenominator);

    // Calculate DAR based on PAR and video size.
    displayWidth = originalWidth * pixelAspectRatioNumerator;
    displayHeight = originalHeight * pixelAspectRatioDenominator;

    // Divide display width and height by their GCD to avoid possible overflows.
    displayAspectRatioGCD = greatestCommonDivisor(displayWidth, displayHeight);
    displayWidth /= displayAspectRatioGCD;
    displayHeight /= displayAspectRatioGCD;

    // Apply DAR to original video size. This is the same behavior as in xvimagesink's setcaps function.
    if (!(originalHeight % displayHeight)) {
        LOG_VERBOSE(Media, "Keeping video original height");
        width = gst_util_uint64_scale_int(originalHeight, displayWidth, displayHeight);
        height = originalHeight;
    } else if (!(originalWidth % displayWidth)) {
        LOG_VERBOSE(Media, "Keeping video original width");
        height = gst_util_uint64_scale_int(originalWidth, displayHeight, displayWidth);
        width = originalWidth;
    } else {
        LOG_VERBOSE(Media, "Approximating while keeping original video height");
        width = gst_util_uint64_scale_int(originalHeight, displayWidth, displayHeight);
        height = originalHeight;
    }

    LOG_VERBOSE(Media, "Natural size: %dx%d", width, height);
    return IntSize(width, height);
}

bool MediaPlayerPrivate::hasVideo() const
{
    gint currentVideo = -1;
    if (m_playBin)
        g_object_get(m_playBin, "current-video", &currentVideo, NULL);
    return currentVideo > -1;
}

bool MediaPlayerPrivate::hasAudio() const
{
    gint currentAudio = -1;
    if (m_playBin)
        g_object_get(m_playBin, "current-audio", &currentAudio, NULL);
    return currentAudio > -1;
}

void MediaPlayerPrivate::setVolume(float volume)
{
    if (!m_playBin)
        return;

    g_object_set(m_playBin, "volume", static_cast<double>(volume), NULL);
}

void MediaPlayerPrivate::volumeChangedCallback()
{
    double volume;
    g_object_get(m_playBin, "volume", &volume, NULL);
    m_player->volumeChanged(static_cast<float>(volume));
}

void MediaPlayerPrivate::volumeChanged()
{
    if (m_volumeIdleId)
        g_source_remove(m_volumeIdleId);
    m_volumeIdleId = g_idle_add((GSourceFunc) notifyVolumeIdleCallback, this);
}

void MediaPlayerPrivate::setRate(float rate)
{
    // Avoid useless playback rate update.
    if (m_playbackRate == rate)
        return;

    GstState state;
    GstState pending;

    gst_element_get_state(m_playBin, &state, &pending, 0);
    if ((state != GST_STATE_PLAYING && state != GST_STATE_PAUSED)
        || (pending == GST_STATE_PAUSED))
        return;

    if (m_isStreaming)
        return;

    m_playbackRate = rate;
    m_changingRate = true;
    float currentPosition = playbackPosition(m_playBin) * GST_SECOND;
    GstSeekFlags flags = (GstSeekFlags)(GST_SEEK_FLAG_FLUSH);
    gint64 start, end;
    bool mute = false;

    LOG_VERBOSE(Media, "Set Rate to %f", rate);
    if (rate >= 0) {
        // Mute the sound if the playback rate is too extreme.
        // TODO: in other cases we should perform pitch adjustments.
        mute = (bool) (rate < 0.8 || rate > 2);
        start = currentPosition;
        end = GST_CLOCK_TIME_NONE;
    } else {
        start = 0;
        mute = true;

        // If we are at beginning of media, start from the end to
        // avoid immediate EOS.
        if (currentPosition <= 0)
            end = duration() * GST_SECOND;
        else
            end = currentPosition;
    }

    LOG_VERBOSE(Media, "Need to mute audio: %d", (int) mute);

    if (!gst_element_seek(m_playBin, rate, GST_FORMAT_TIME, flags,
                          GST_SEEK_TYPE_SET, start,
                          GST_SEEK_TYPE_SET, end))
        LOG_VERBOSE(Media, "Set rate to %f failed", rate);
    else
        g_object_set(m_playBin, "mute", mute, NULL);
}

MediaPlayer::NetworkState MediaPlayerPrivate::networkState() const
{
    return m_networkState;
}

MediaPlayer::ReadyState MediaPlayerPrivate::readyState() const
{
    return m_readyState;
}

PassRefPtr<TimeRanges> MediaPlayerPrivate::buffered() const
{
    RefPtr<TimeRanges> timeRanges = TimeRanges::create();
    float loaded = maxTimeLoaded();
    if (!m_errorOccured && !m_isStreaming && loaded > 0)
        timeRanges->add(0, loaded);
    return timeRanges.release();
}

void MediaPlayerPrivate::processBufferingStats(GstMessage* message)
{
    // This is the immediate buffering that needs to happen so we have
    // enough to play right now.
    m_buffering = true;
    const GstStructure *structure = gst_message_get_structure(message);
    gst_structure_get_int(structure, "buffer-percent", &m_bufferingPercentage);

    LOG_VERBOSE(Media, "[Buffering] Buffering: %d%%.", m_bufferingPercentage);

    GstBufferingMode mode;
    gst_message_parse_buffering_stats(message, &mode, 0, 0, 0);
    if (mode != GST_BUFFERING_DOWNLOAD) {
        updateStates();
        return;
    }

    // This is on-disk buffering, that allows us to download much more
    // than needed for right now.
    if (!m_startedBuffering) {
        LOG_VERBOSE(Media, "[Buffering] Starting on-disk buffering.");

        m_startedBuffering = true;

        if (m_fillTimeoutId > 0)
            g_source_remove(m_fillTimeoutId);

        m_fillTimeoutId = g_timeout_add(200, (GSourceFunc) bufferingTimeoutCallback, this);
    }
}

bool MediaPlayerPrivate::queryBufferingStats()
{
    GstQuery* query = gst_query_new_buffering(GST_FORMAT_PERCENT);

    if (!gst_element_query(m_playBin, query)) {
        gst_query_unref(query);
        return TRUE;
    }

    gint64 start, stop;
    gdouble fillStatus = 100.0;

    gst_query_parse_buffering_range(query, 0, &start, &stop, 0);
    gst_query_unref(query);

    if (stop != -1)
        fillStatus = 100.0 * stop / GST_FORMAT_PERCENT_MAX;

    LOG_VERBOSE(Media, "[Buffering] Download buffer filled up to %f%%", fillStatus);

    if (!m_mediaDuration)
        durationChanged();

    // Update maxTimeLoaded only if the media duration is
    // available. Otherwise we can't compute it.
    if (m_mediaDuration) {
        m_maxTimeLoaded = static_cast<float>((fillStatus * m_mediaDuration) / 100.0);
        LOG_VERBOSE(Media, "[Buffering] Updated maxTimeLoaded: %f", m_maxTimeLoaded);
    }

    if (fillStatus != 100.0) {
        updateStates();
        return TRUE;
    }

    // Media is now fully loaded. It will play even if network
    // connection is cut. Buffering is done, remove the fill source
    // from the main loop.
    m_fillTimeoutId = 0;
    m_startedBuffering = false;
    updateStates();
    return FALSE;
}

float MediaPlayerPrivate::maxTimeSeekable() const
{
    if (m_errorOccured)
        return 0.0;

    LOG_VERBOSE(Media, "maxTimeSeekable");
    // infinite duration means live stream
    if (isinf(duration()))
        return 0.0;

    return maxTimeLoaded();
}

float MediaPlayerPrivate::maxTimeLoaded() const
{
    if (m_errorOccured)
        return 0.0;

    float loaded = m_maxTimeLoaded;
    if (!loaded && !m_fillTimeoutId)
        loaded = duration();
    LOG_VERBOSE(Media, "maxTimeLoaded: %f", loaded);
    return loaded;
}

unsigned MediaPlayerPrivate::bytesLoaded() const
{
    if (!m_playBin)
        return 0;

    if (!m_mediaDuration)
        return 0;

    unsigned loaded = totalBytes() * maxTimeLoaded() / m_mediaDuration;
    LOG_VERBOSE(Media, "bytesLoaded: %d", loaded);
    return loaded;
}

unsigned MediaPlayerPrivate::totalBytes() const
{
    if (!m_source)
        return 0;

    if (m_errorOccured)
        return 0;

    GstFormat fmt = GST_FORMAT_BYTES;
    gint64 length = 0;
    gst_element_query_duration(m_source, &fmt, &length);
    LOG_VERBOSE(Media, "totalBytes %" G_GINT64_FORMAT, length);

    return length;
}

void MediaPlayerPrivate::cancelLoad()
{
    if (m_networkState < MediaPlayer::Loading || m_networkState == MediaPlayer::Loaded)
        return;

    if (m_playBin)
        gst_element_set_state(m_playBin, GST_STATE_NULL);
}

void MediaPlayerPrivate::updateStates()
{
    if (!m_playBin)
        return;

    if (m_errorOccured)
        return;

    MediaPlayer::NetworkState oldNetworkState = m_networkState;
    MediaPlayer::ReadyState oldReadyState = m_readyState;
    GstState state;
    GstState pending;

    GstStateChangeReturn ret = gst_element_get_state(m_playBin,
        &state, &pending, 250 * GST_NSECOND);

    bool shouldUpdateAfterSeek = false;
    switch (ret) {
    case GST_STATE_CHANGE_SUCCESS:
        LOG_VERBOSE(Media, "State: %s, pending: %s",
            gst_element_state_get_name(state),
            gst_element_state_get_name(pending));

        m_resetPipeline = state <= GST_STATE_READY;

        // Try to figure out ready and network states.
        if (state == GST_STATE_READY) {
            m_readyState = MediaPlayer::HaveNothing;
            m_networkState = MediaPlayer::Empty;
        } else if (maxTimeLoaded() == duration()) {
            m_networkState = MediaPlayer::Loaded;
            m_readyState = MediaPlayer::HaveEnoughData;
        } else {
            m_readyState = currentTime() < maxTimeLoaded() ? MediaPlayer::HaveFutureData : MediaPlayer::HaveCurrentData;
            m_networkState = MediaPlayer::Loading;
        }

        if (m_buffering && state != GST_STATE_READY) {
            m_readyState = MediaPlayer::HaveCurrentData;
            m_networkState = MediaPlayer::Loading;
        }

        // Now let's try to get the states in more detail using
        // information from GStreamer, while we sync states where
        // needed.
        if (state == GST_STATE_PAUSED) {
            if (m_buffering && m_bufferingPercentage == 100) {
                m_buffering = false;
                m_bufferingPercentage = 0;
                m_readyState = MediaPlayer::HaveEnoughData;

                LOG_VERBOSE(Media, "[Buffering] Complete.");

                if (!m_paused) {
                    LOG_VERBOSE(Media, "[Buffering] Restarting playback.");
                    gst_element_set_state(m_playBin, GST_STATE_PLAYING);
                }
            } else if (!m_buffering)
                m_paused = true;
        } else if (state == GST_STATE_PLAYING) {
            m_readyState = MediaPlayer::HaveEnoughData;
            m_paused = false;

            if (!m_mediaDuration) {
                float newDuration = duration();
                if (!isinf(newDuration))
                    m_mediaDuration = newDuration;
            }

            if (m_buffering) {
                m_readyState = MediaPlayer::HaveCurrentData;
                m_networkState = MediaPlayer::Loading;

                LOG_VERBOSE(Media, "[Buffering] Pausing stream for buffering.");

                gst_element_set_state(m_playBin, GST_STATE_PAUSED);
            }
        } else
            m_paused = true;

        // Is on-disk buffering in progress?

        if (m_fillTimeoutId)
            m_networkState = MediaPlayer::Loading;

        if (m_changingRate) {
            m_player->rateChanged();
            m_changingRate = false;
        }

        if (m_seeking) {
            shouldUpdateAfterSeek = true;
            m_seeking = false;
        }

        break;
    case GST_STATE_CHANGE_ASYNC:
        LOG_VERBOSE(Media, "Async: State: %s, pending: %s",
            gst_element_state_get_name(state),
            gst_element_state_get_name(pending));
        // Change in progress

        if (!m_isStreaming)
            return;

        // Resume playback if a seek was performed in a live pipeline.
        if (m_seeking) {
            shouldUpdateAfterSeek = true;
            m_seeking = false;
            if (m_paused)
                gst_element_set_state(m_playBin, GST_STATE_PLAYING);
        }
        break;
    case GST_STATE_CHANGE_FAILURE:
        LOG_VERBOSE(Media, "Failure: State: %s, pending: %s",
            gst_element_state_get_name(state),
            gst_element_state_get_name(pending));
        // Change failed
        return;
    case GST_STATE_CHANGE_NO_PREROLL:
        LOG_VERBOSE(Media, "No preroll: State: %s, pending: %s",
            gst_element_state_get_name(state),
            gst_element_state_get_name(pending));

        if (state == GST_STATE_READY)
            m_readyState = MediaPlayer::HaveNothing;
        else if (state == GST_STATE_PAUSED) {
            m_readyState = MediaPlayer::HaveEnoughData;
            m_paused = true;
            // Live pipelines go in PAUSED without prerolling.
            m_isStreaming = true;
        } else if (state == GST_STATE_PLAYING)
            m_paused = false;

        if (m_seeking) {
            shouldUpdateAfterSeek = true;
            m_seeking = false;
            if (!m_paused)
                gst_element_set_state(m_playBin, GST_STATE_PLAYING);
        } else if (!m_paused)
            gst_element_set_state(m_playBin, GST_STATE_PLAYING);

        m_networkState = MediaPlayer::Loading;
        break;
    default:
        LOG_VERBOSE(Media, "Else : %d", ret);
        break;
    }

    if (seeking())
        m_readyState = MediaPlayer::HaveNothing;

    if (shouldUpdateAfterSeek)
        timeChanged();

    if (m_networkState != oldNetworkState) {
        LOG_VERBOSE(Media, "Network State Changed from %u to %u",
            oldNetworkState, m_networkState);
        m_player->networkStateChanged();
    }
    if (m_readyState != oldReadyState) {
        LOG_VERBOSE(Media, "Ready State Changed from %u to %u",
            oldReadyState, m_readyState);
        m_player->readyStateChanged();
    }
}

void MediaPlayerPrivate::mediaLocationChanged(GstMessage* message)
{
    if (m_mediaLocations)
        gst_structure_free(m_mediaLocations);

    if (message->structure) {
        // This structure can contain:
        // - both a new-location string and embedded locations structure
        // - or only a new-location string.
        m_mediaLocations = gst_structure_copy(message->structure);
        const GValue* locations = gst_structure_get_value(m_mediaLocations, "locations");

        if (locations)
            m_mediaLocationCurrentIndex = static_cast<int>(gst_value_list_get_size(locations)) -1;

        loadNextLocation();
    }
}

bool MediaPlayerPrivate::loadNextLocation()
{
    if (!m_mediaLocations)
        return false;

    const GValue* locations = gst_structure_get_value(m_mediaLocations, "locations");
    const gchar* newLocation = 0;

    if (!locations) {
        // Fallback on new-location string.
        newLocation = gst_structure_get_string(m_mediaLocations, "new-location");
        if (!newLocation)
            return false;
    }

    if (!newLocation) {
        if (m_mediaLocationCurrentIndex < 0) {
            m_mediaLocations = 0;
            return false;
        }

        const GValue* location = gst_value_list_get_value(locations,
                                                          m_mediaLocationCurrentIndex);
        const GstStructure* structure = gst_value_get_structure(location);

        if (!structure) {
            m_mediaLocationCurrentIndex--;
            return false;
        }

        newLocation = gst_structure_get_string(structure, "new-location");
    }

    if (newLocation) {
        // Found a candidate. new-location is not always an absolute url
        // though. We need to take the base of the current url and
        // append the value of new-location to it.

        gchar* currentLocation = 0;
        g_object_get(m_playBin, "uri", &currentLocation, NULL);

        KURL currentUrl(KURL(), currentLocation);
        g_free(currentLocation);

        KURL newUrl;

        if (gst_uri_is_valid(newLocation))
            newUrl = KURL(KURL(), newLocation);
        else
            newUrl = KURL(KURL(), currentUrl.baseAsString() + newLocation);

        RefPtr<SecurityOrigin> securityOrigin = SecurityOrigin::create(currentUrl);
        if (securityOrigin->canRequest(newUrl)) {
            LOG_VERBOSE(Media, "New media url: %s", newUrl.string().utf8().data());

            // Reset player states.
            m_networkState = MediaPlayer::Loading;
            m_player->networkStateChanged();
            m_readyState = MediaPlayer::HaveNothing;
            m_player->readyStateChanged();

            // Reset pipeline state.
            m_resetPipeline = true;
            gst_element_set_state(m_playBin, GST_STATE_READY);

            GstState state;
            gst_element_get_state(m_playBin, &state, 0, 0);
            if (state <= GST_STATE_READY) {
                // Set the new uri and start playing.
                g_object_set(m_playBin, "uri", newUrl.string().utf8().data(), NULL);
                gst_element_set_state(m_playBin, GST_STATE_PLAYING);
                return true;
            }
        }
    }
    m_mediaLocationCurrentIndex--;
    return false;

}

void MediaPlayerPrivate::loadStateChanged()
{
    updateStates();
}

void MediaPlayerPrivate::sizeChanged()
{
    notImplemented();
}

void MediaPlayerPrivate::timeChanged()
{
    updateStates();
    m_player->timeChanged();
}

void MediaPlayerPrivate::didEnd()
{
    // EOS was reached but in case of reverse playback the position is
    // not always 0. So to not confuse the HTMLMediaElement we
    // synchronize position and duration values.
    float now = currentTime();
    if (now > 0) {
        m_mediaDuration = now;
        m_player->durationChanged();
    }

    gst_element_set_state(m_playBin, GST_STATE_PAUSED);

    timeChanged();
}

void MediaPlayerPrivate::durationChanged()
{
    // Reset cached media duration
    m_mediaDuration = 0;

    // And re-cache it if possible.
    float newDuration = duration();
    if (!isinf(newDuration))
        m_mediaDuration = newDuration;

    m_player->durationChanged();
}

bool MediaPlayerPrivate::supportsMuting() const
{
    return true;
}

void MediaPlayerPrivate::setMuted(bool muted)
{
    if (!m_playBin)
        return;

    g_object_set(m_playBin, "mute", muted, NULL);
}

void MediaPlayerPrivate::muteChangedCallback()
{
    gboolean muted;
    g_object_get(m_playBin, "mute", &muted, NULL);
    m_player->muteChanged(static_cast<bool>(muted));
}

void MediaPlayerPrivate::muteChanged()
{
    if (m_muteIdleId)
        g_source_remove(m_muteIdleId);

    m_muteIdleId = g_idle_add((GSourceFunc) notifyMuteIdleCallback, this);
}

void MediaPlayerPrivate::loadingFailed(MediaPlayer::NetworkState error)
{
    m_errorOccured = true;
    if (m_networkState != error) {
        m_networkState = error;
        m_player->networkStateChanged();
    }
    if (m_readyState != MediaPlayer::HaveNothing) {
        m_readyState = MediaPlayer::HaveNothing;
        m_player->readyStateChanged();
    }
}

void MediaPlayerPrivate::setSize(const IntSize& size)
{
    m_size = size;
}

void MediaPlayerPrivate::setVisible(bool visible)
{
}

void MediaPlayerPrivate::repaint()
{
    m_player->repaint();
}

void MediaPlayerPrivate::paint(GraphicsContext* context, const IntRect& rect)
{
    if (context->paintingDisabled())
        return;

    if (!m_player->visible())
        return;
    if (!m_buffer)
        return;

    int width = 0, height = 0;
    GstCaps* caps = gst_buffer_get_caps(m_buffer);
    GstVideoFormat format;

    if (!gst_video_format_parse_caps(caps, &format, &width, &height)) {
      gst_caps_unref(caps);
      return;
    }

    cairo_format_t cairoFormat;
    if (format == GST_VIDEO_FORMAT_ARGB || format == GST_VIDEO_FORMAT_BGRA)
        cairoFormat = CAIRO_FORMAT_ARGB32;
    else
        cairoFormat = CAIRO_FORMAT_RGB24;

    cairo_t* cr = context->platformContext();
    cairo_surface_t* src = cairo_image_surface_create_for_data(GST_BUFFER_DATA(m_buffer),
                                                               cairoFormat,
                                                               width, height,
                                                               4 * width);

    cairo_save(cr);

    // translate and scale the context to correct size
    cairo_translate(cr, rect.x(), rect.y());
    cairo_scale(cr, static_cast<double>(rect.width()) / width, static_cast<double>(rect.height()) / height);

    // And paint it.
    cairo_set_source_surface(cr, src, 0, 0);
    cairo_pattern_set_extend(cairo_get_source(cr), CAIRO_EXTEND_PAD);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    cairo_restore(cr);

    cairo_surface_destroy(src);
    gst_caps_unref(caps);
}

static HashSet<String> mimeTypeCache()
{

    doGstInit();

    static HashSet<String> cache;
    static bool typeListInitialized = false;

    if (!typeListInitialized) {
        // Build a whitelist of mime-types known to be supported by
        // GStreamer.
        HashSet<String> handledApplicationSubtypes;
        handledApplicationSubtypes.add(String("ogg"));
        handledApplicationSubtypes.add(String("x-3gp"));
        handledApplicationSubtypes.add(String("vnd.rn-realmedia"));
        handledApplicationSubtypes.add(String("x-pn-realaudio"));

        GList* factories = gst_type_find_factory_get_list();
        for (GList* iterator = factories; iterator; iterator = iterator->next) {
            GstTypeFindFactory* factory = GST_TYPE_FIND_FACTORY(iterator->data);
            GstCaps* caps = gst_type_find_factory_get_caps(factory);

            if (!caps)
                continue;

            for (guint structureIndex = 0; structureIndex < gst_caps_get_size(caps); structureIndex++) {
                GstStructure* structure = gst_caps_get_structure(caps, structureIndex);
                const gchar* name = gst_structure_get_name(structure);
                bool cached = false;

                // These formats are supported by GStreamer, but not
                // correctly advertised.
                if (g_str_equal(name, "video/x-h264")
                    || g_str_equal(name, "audio/x-m4a")) {
                    cache.add(String("video/mp4"));
                    cache.add(String("audio/aac"));
                    cached = true;
                }

                if (g_str_equal(name, "video/x-theora")) {
                    cache.add(String("video/ogg"));
                    cached = true;
                }

                if (g_str_equal(name, "audio/x-vorbis")) {
                    cache.add(String("audio/ogg"));
                    cached = true;
                }

                if (g_str_equal(name, "audio/x-wav")) {
                    cache.add(String("audio/wav"));
                    cached = true;
                }

                if (g_str_equal(name, "audio/mpeg")) {
                    cache.add(String(name));
                    cached = true;

                    // This is what we are handling:
                    // mpegversion=(int)1, layer=(int)[ 1, 3 ]
                    gint mpegVersion = 0;
                    if (gst_structure_get_int(structure, "mpegversion", &mpegVersion) && (mpegVersion == 1)) {
                        const GValue* layer = gst_structure_get_value(structure, "layer");
                        if (G_VALUE_TYPE(layer) == GST_TYPE_INT_RANGE) {
                            gint minLayer = gst_value_get_int_range_min(layer);
                            gint maxLayer = gst_value_get_int_range_max(layer);
                            if (minLayer <= 1 && 1 <= maxLayer)
                                cache.add(String("audio/mp1"));
                            if (minLayer <= 2 && 2 <= maxLayer)
                                cache.add(String("audio/mp2"));
                            if (minLayer <= 3 && 3 <= maxLayer)
                                cache.add(String("audio/mp3"));
                        }
                    }
                }

                if (!cached) {
                    // GStreamer plugins can be capable of supporting
                    // types which WebKit supports by default. In that
                    // case, we should not consider these types
                    // supportable by GStreamer.  Examples of what
                    // GStreamer can support but should not be added:
                    // text/plain, text/html, image/jpeg,
                    // application/xml
                    gchar** mimetype = g_strsplit(name, "/", 2);
                    if (g_str_equal(mimetype[0], "audio")
                        || g_str_equal(mimetype[0], "video")
                        || (g_str_equal(mimetype[0], "application")
                            && handledApplicationSubtypes.contains(String(mimetype[1]))))
                        cache.add(String(name));

                    g_strfreev(mimetype);
                }
            }
        }

        gst_plugin_feature_list_free(factories);
        typeListInitialized = true;
    }

    return cache;
}

void MediaPlayerPrivate::getSupportedTypes(HashSet<String>& types)
{
    types = mimeTypeCache();
}

MediaPlayer::SupportsType MediaPlayerPrivate::supportsType(const String& type, const String& codecs)
{
    if (type.isNull() || type.isEmpty())
        return MediaPlayer::IsNotSupported;

    // spec says we should not return "probably" if the codecs string is empty
    if (mimeTypeCache().contains(type))
        return codecs.isEmpty() ? MediaPlayer::MayBeSupported : MediaPlayer::IsSupported;
    return MediaPlayer::IsNotSupported;
}

bool MediaPlayerPrivate::hasSingleSecurityOrigin() const
{
    return true;
}

bool MediaPlayerPrivate::supportsFullscreen() const
{
    return true;
}

void MediaPlayerPrivate::setPreload(MediaPlayer::Preload preload)
{
    ASSERT(m_playBin);

    m_preload = preload;

    GstPlayFlags flags;
    g_object_get(m_playBin, "flags", &flags, NULL);
    if (preload == MediaPlayer::None)
        g_object_set(m_playBin, "flags", flags & ~GST_PLAY_FLAG_DOWNLOAD, NULL);
    else
        g_object_set(m_playBin, "flags", flags | GST_PLAY_FLAG_DOWNLOAD, NULL);

    if (m_delayingLoad && m_preload != MediaPlayer::None) {
        m_delayingLoad = false;
        commitLoad();
    }
}

void MediaPlayerPrivate::createGSTPlayBin()
{
    ASSERT(!m_playBin);
    m_playBin = gst_element_factory_make("playbin2", "play");

    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_playBin));
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message", G_CALLBACK(mediaPlayerPrivateMessageCallback), this);
    gst_object_unref(bus);

    g_signal_connect(m_playBin, "notify::volume", G_CALLBACK(mediaPlayerPrivateVolumeChangedCallback), this);
    g_signal_connect(m_playBin, "notify::source", G_CALLBACK(mediaPlayerPrivateSourceChangedCallback), this);
    g_signal_connect(m_playBin, "notify::mute", G_CALLBACK(mediaPlayerPrivateMuteChangedCallback), this);

    m_videoSink = webkit_video_sink_new();

    g_object_ref_sink(m_videoSink);

    WTFLogChannel* channel = getChannelFromName("Media");
    if (channel->state == WTFLogChannelOn) {
        m_fpsSink = gst_element_factory_make("fpsdisplaysink", "sink");
        if (g_object_class_find_property(G_OBJECT_GET_CLASS(m_fpsSink), "video-sink")) {
            g_object_set(m_fpsSink, "video-sink", m_videoSink, NULL);
            g_object_ref_sink(m_fpsSink);
            g_object_set(m_playBin, "video-sink", m_fpsSink, NULL);
        } else {
            m_fpsSink = 0;
            g_object_set(m_playBin, "video-sink", m_videoSink, NULL);
            LOG_VERBOSE(Media, "Can't display FPS statistics, you need gst-plugins-bad >= 0.10.18");
        }
    } else
        g_object_set(m_playBin, "video-sink", m_videoSink, NULL);

    g_signal_connect(m_videoSink, "repaint-requested", G_CALLBACK(mediaPlayerPrivateRepaintCallback), this);
}

}

#endif
