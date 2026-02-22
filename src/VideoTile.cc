#include "VideoTile.hh"

#include <QDebug>
#include <QMutexLocker>
#include <QPainter>

VideoTile::VideoTile(QQuickItem* parent) : QQuickPaintedItem(parent) {
  qDebug() << Q_FUNC_INFO << "VideoTile created";
  setRenderTarget(QQuickPaintedItem::FramebufferObject);
  setAntialiasing(false);
}

void VideoTile::setPeerId(const QString& id) {
  if (m_peerId == id) {
    return;
  }

  m_peerId = id;
  qDebug() << Q_FUNC_INFO << "peerId:" << id;
  emit peerIdChanged();
}

void VideoTile::setDisplayName(const QString& name) {
  if (m_displayName == name) {
    return;
  }

  m_displayName = name;
  qDebug() << Q_FUNC_INFO << "displayName:" << name;
  emit displayNameChanged();
}

void VideoTile::setMuted(bool m) {
  if (m_muted == m) {
    return;
  }
  m_muted = m;
  qDebug() << Q_FUNC_INFO << "muted:" << m << "peer:" << m_peerId;
  emit mutedChanged();
  update();
}

void VideoTile::setVideoEnabled(bool e) {
  if (m_videoEnabled == e) {
    return;
  }
  m_videoEnabled = e;
  qDebug() << Q_FUNC_INFO << "videoEnabled:" << e << "peer:" << m_peerId;
  emit videoEnabledChanged();
  update();
}

void VideoTile::setHasVideo(bool v) {
  if (m_hasVideo == v) {
    return;
  }
  m_hasVideo = v;
  qDebug() << Q_FUNC_INFO << "hasVideo:" << v << "peer:" << m_peerId;
  emit hasVideoChanged();
}

void VideoTile::onFrame(const QImage& frame) {
  if (frame.isNull()) {
    qWarning() << Q_FUNC_INFO << "Received null frame for peer:" << m_peerId;
    return;
  }

  {
    QMutexLocker lk(&m_frameMutex);
    m_currentFrame = frame;
  }

  if (!m_hasVideo) {
    setHasVideo(true);
  }
  update();  // triggers paint() on Qt render thread
}

void VideoTile::paint(QPainter* painter) {
  const QRectF bounds = boundingRect();

  // Background
  painter->fillRect(bounds, QColor(0x23, 0x27, 0x2A));  // dark Discord-like bg

  QImage frame;
  {
    QMutexLocker lk(&m_frameMutex);
    frame = m_currentFrame;
  }

  if (!frame.isNull() && m_videoEnabled) {
    // Scale-to-fit preserving aspect ratio
    const QSizeF frameSize = QSizeF(frame.size()).scaled(bounds.size(), Qt::KeepAspectRatio);
    const QRectF dest(bounds.x() + ((bounds.width() - frameSize.width()) / 2.0),
                      bounds.y() + ((bounds.height() - frameSize.height()) / 2.0),
                      frameSize.width(), frameSize.height());
    painter->drawImage(dest, frame);
  } else {
    // No video: draw avatar placeholder
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0x36, 0x39, 0x3F));
    const double r = qMin(bounds.width(), bounds.height()) * 0.3;
    painter->drawEllipse(bounds.center(), r, r);

    if (!m_displayName.isEmpty()) {
      painter->setPen(Qt::white);
      QFont f = painter->font();
      f.setPixelSize(qMax(12.0, r * 0.8));
      f.setBold(true);
      painter->setFont(f);
      painter->drawText(bounds, Qt::AlignCenter, m_displayName.left(2).toUpper());
    }
  }

  // Muted indicator
  if (m_muted) {
    const QRectF badge(bounds.right() - 28, bounds.bottom() - 28, 22, 22);
    painter->setBrush(QColor(0xED, 0x43, 0x45));  // red
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(badge);

    painter->setPen(Qt::white);
    QFont f = painter->font();
    f.setPixelSize(12);
    painter->setFont(f);
    painter->drawText(badge, Qt::AlignCenter, "M");
  }

  // Name overlay
  if (!m_displayName.isEmpty()) {
    const QRectF nameBar(bounds.x(), bounds.bottom() - 24, bounds.width(), 24);
    painter->fillRect(nameBar, QColor(0, 0, 0, 160));
    painter->setPen(Qt::white);
    QFont f = painter->font();
    f.setPixelSize(12);
    painter->setFont(f);
    painter->drawText(nameBar.adjusted(6, 0, -6, 0), Qt::AlignVCenter | Qt::AlignLeft,
                      m_displayName);
  }
}
