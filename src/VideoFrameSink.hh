#pragma once

#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include <QImage>
#include <QObject>
#include <QString>

// Thread-safe bridge from a GStreamer appsink to Qt signals.
// Lives on the Qt main thread; the on_new_sample callback runs on the GStreamer
// thread and posts frameReady via
// QMetaObject::invokeMethod(Qt::QueuedConnection).
class VideoFrameSink : public QObject {
  Q_OBJECT

  Q_PROPERTY(QString peerId READ peerId WRITE setPeerId NOTIFY peerIdChanged)

 public:
  explicit VideoFrameSink(QObject* parent = nullptr);
  ~VideoFrameSink() override;

  // Call this ONCE after construction to get the GstElement* to add to a
  // pipeline. Ownership stays with this object (ref held via gst_object_ref).
  [[nodiscard]] GstElement* element() const {
    return m_appsink;
  }

  [[nodiscard]] QString peerId() const {
    return m_peerId;
  }
  void setPeerId(const QString& id);

 signals:
  void peerIdChanged();
  // Emitted on the Qt main thread when a new video frame arrives.
  void frameReady(QImage frame);

 private:
  static GstFlowReturn on_new_sample(GstAppSink* sink, gpointer user_data);
  QImage               convertSample(GstSample* sample);

  GstElement* m_appsink = nullptr;
  QString     m_peerId;
};
