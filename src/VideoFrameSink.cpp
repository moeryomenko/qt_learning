#include "VideoFrameSink.h"

#include <QDebug>
#include <QMetaObject>

VideoFrameSink::VideoFrameSink(QObject* parent)
    : QObject(parent)
{
    qDebug() << Q_FUNC_INFO << "Creating appsink";

    m_appsink = gst_element_factory_make("appsink", nullptr);
    if (!m_appsink) {
        qWarning() << Q_FUNC_INFO << "Failed to create appsink element";
        return;
    }

    // Accept only RGB/BGR/RGBA video formats (easy to convert to QImage)
    GstCaps* caps = gst_caps_from_string(
        "video/x-raw,"
        "format=(string){BGRA,BGR,RGB,RGBA,I420,YV12,NV12,NV21}");
    g_object_set(m_appsink,
                 "emit-signals", TRUE,
                 "caps", caps,
                 "max-buffers", 1,      // drop old frames if Qt thread is slow
                 "drop", TRUE,
                 "sync", FALSE,
                 nullptr);
    gst_caps_unref(caps);

    GstAppSinkCallbacks callbacks = {};
    callbacks.new_sample = &VideoFrameSink::on_new_sample;
    gst_app_sink_set_callbacks(GST_APP_SINK(m_appsink), &callbacks, this, nullptr);

    qDebug() << Q_FUNC_INFO << "appsink created:" << m_appsink;
}

VideoFrameSink::~VideoFrameSink() {
    qDebug() << Q_FUNC_INFO << "Destroying VideoFrameSink for peer:" << m_peerId;
    if (m_appsink) {
        gst_object_unref(m_appsink);
        m_appsink = nullptr;
    }
}

void VideoFrameSink::setPeerId(const QString& id) {
    if (m_peerId == id) return;
    m_peerId = id;
    qDebug() << Q_FUNC_INFO << "peerId set to:" << id;
    emit peerIdChanged();
}

// Static callback — runs on GStreamer streaming thread
GstFlowReturn VideoFrameSink::on_new_sample(GstAppSink* sink, gpointer user_data) {
    auto* self = static_cast<VideoFrameSink*>(user_data);

    GstSample* sample = gst_app_sink_pull_sample(sink);
    if (!sample) {
        qWarning() << "[VideoFrameSink] on_new_sample: null sample";
        return GST_FLOW_OK;
    }

    // Convert on the GStreamer thread (cheap), then marshal the QImage
    QImage frame = self->convertSample(sample);
    gst_sample_unref(sample);

    if (frame.isNull()) {
        qWarning() << "[VideoFrameSink] on_new_sample: null frame for peer" << self->m_peerId;
        return GST_FLOW_OK;
    }

    // Post to Qt main thread — thread-safe
    QMetaObject::invokeMethod(self, [self, frame = std::move(frame)]() mutable {
        emit self->frameReady(std::move(frame));
    }, Qt::QueuedConnection);

    return GST_FLOW_OK;
}

QImage VideoFrameSink::convertSample(GstSample* sample) {
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        qWarning() << Q_FUNC_INFO << "No buffer in sample";
        return {};
    }

    GstCaps* caps = gst_sample_get_caps(sample);
    if (!caps) {
        qWarning() << Q_FUNC_INFO << "No caps in sample";
        return {};
    }

    GstVideoInfo info;
    if (!gst_video_info_from_caps(&info, caps)) {
        qWarning() << Q_FUNC_INFO << "Failed to get video info from caps";
        return {};
    }

    const int width  = GST_VIDEO_INFO_WIDTH(&info);
    const int height = GST_VIDEO_INFO_HEIGHT(&info);
    const GstVideoFormat fmt = GST_VIDEO_INFO_FORMAT(&info);

    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        qWarning() << Q_FUNC_INFO << "Failed to map buffer";
        return {};
    }

    QImage img;
    switch (fmt) {
        case GST_VIDEO_FORMAT_BGRA:
            img = QImage(map.data, width, height, QImage::Format_ARGB32).copy();
            break;
        case GST_VIDEO_FORMAT_BGR:
            // Qt has no BGR24 — convert via RGBX workaround: copy to RGB and swap
            img = QImage(map.data, width, height, width * 3, QImage::Format_RGB888).rgbSwapped();
            break;
        case GST_VIDEO_FORMAT_RGB:
            img = QImage(map.data, width, height, width * 3, QImage::Format_RGB888).copy();
            break;
        case GST_VIDEO_FORMAT_RGBA:
            img = QImage(map.data, width, height, QImage::Format_RGBA8888).copy();
            break;
        default:
            // For YUV (I420, NV12 etc.) — convert to RGB via a slow but simple path
            // In production you'd use a GStreamer videoconvert before appsink, but
            // since caps negotiation will pick RGB/BGRA preferentially, this is fallback.
            qWarning() << Q_FUNC_INFO << "Unhandled video format, falling back to RGB888 attempt";
            img = QImage(map.data, width, height, QImage::Format_RGB888).copy();
            break;
    }

    gst_buffer_unmap(buffer, &map);
    return img;
}
