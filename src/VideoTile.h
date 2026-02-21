#pragma once

#include <QImage>
#include <QMutex>
#include <QQuickPaintedItem>
#include <QString>

// QML component that displays a video stream from a VideoFrameSink.
// Register as QML type "VideoTile" in "pages" URI.
//
// Usage in QML:
//   VideoTile {
//       peerId: "abc123"
//       width: 320; height: 240
//   }
//   // C++: connect VideoFrameSink::frameReady to VideoTile::onFrame
class VideoTile : public QQuickPaintedItem {
  Q_OBJECT

  Q_PROPERTY(QString peerId READ peerId WRITE setPeerId NOTIFY peerIdChanged)
  Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY
                 displayNameChanged)
  Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
  Q_PROPERTY(bool videoEnabled READ videoEnabled WRITE setVideoEnabled NOTIFY
                 videoEnabledChanged)
  Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)

public:
  explicit VideoTile(QQuickItem *parent = nullptr);

  QString peerId() const { return m_peerId; }
  void setPeerId(const QString &id);

  QString displayName() const { return m_displayName; }
  void setDisplayName(const QString &name);

  bool muted() const { return m_muted; }
  void setMuted(bool m);

  bool videoEnabled() const { return m_videoEnabled; }
  void setVideoEnabled(bool e);

  bool hasVideo() const { return m_hasVideo; }

  // Thread-safe: can be called from any thread
  void paint(QPainter *painter) override;

public slots:
  // Connect VideoFrameSink::frameReady here
  void onFrame(const QImage &frame);

signals:
  void peerIdChanged();
  void displayNameChanged();
  void mutedChanged();
  void videoEnabledChanged();
  void hasVideoChanged();

private:
  void setHasVideo(bool v);

private:
  QString m_peerId;
  QString m_displayName;
  bool m_muted = false;
  bool m_videoEnabled = true;
  bool m_hasVideo = false;

  QMutex m_frameMutex;
  QImage m_currentFrame;
};
