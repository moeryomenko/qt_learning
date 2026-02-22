#include "WebRtcPeerManager.hh"

#include <gst/app/gstappsrc.h>
#include <gst/sdp/sdp.h>
#include <unistd.h>

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioInput>
#include <QDebug>
#include <QGuiApplication>
#include <QImage>
#include <QMediaCaptureSession>
#include <QMetaObject>
#include <QScreen>
#include <QScreenCapture>
#include <QVideoFrame>
#include <QVideoSink>

#include "SignalingClient.hh"
#include "VideoTile.hh"

// ---------- helpers -----------------------------------------------------------

gboolean WebRtcPeerManager::busCallback(GstBus*, GstMessage* msg, gpointer user_data) {
  auto* self = static_cast<WebRtcPeerManager*>(user_data);
  if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
    GError* err = nullptr;
    gchar*  dbg = nullptr;
    gst_message_parse_error(msg, &err, &dbg);
    QString e = QString("GST ERROR: %1").arg(err ? err->message : "unknown");
    if (dbg)
      e += "\n" + QString(dbg);
    if (err)
      g_error_free(err);
    if (dbg)
      g_free(dbg);
    qWarning() << "[WebRtcPeerManager] Bus error:" << e;
    QMetaObject::invokeMethod(self, [self, e]() { emit self->error(e); }, Qt::QueuedConnection);
  } else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_STATE_CHANGED) {
    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(self->m_pipeline)) {
      GstState old_, new_, pending;
      gst_message_parse_state_changed(msg, &old_, &new_, &pending);
      qDebug() << "[WebRtcPeerManager] Pipeline state:" << gst_element_state_get_name(old_) << "->"
               << gst_element_state_get_name(new_);
    }
  }
  return TRUE;
}

// ---------- constructor/destructor -------------------------------------------
WebRtcPeerManager::WebRtcPeerManager(QObject* parent) : QObject(parent) {
  qDebug() << Q_FUNC_INFO << "WebRtcPeerManager created";
  gst_init(nullptr, nullptr);

  // Initialize audio format for microphone capture
  m_audioFormat.setSampleRate(48000);
  m_audioFormat.setChannelCount(2);
  m_audioFormat.setSampleFormat(QAudioFormat::Int16);
}
WebRtcPeerManager::~WebRtcPeerManager() {
  qDebug() << Q_FUNC_INFO << "Destroying";
  stop();
}

// ---------- public API -------------------------------------------------------

bool WebRtcPeerManager::startWithCamera() {
  qDebug() << Q_FUNC_INFO << "Starting with camera";
  // Save ALL deferred state before stop() — teardown() clears m_pendingPeers,
  // m_pendingOffers, and m_pendingIce. Save them now so they survive the teardown.
  QList<QString>          savedPeers  = m_pendingPeers + QList<QString>(m_peers.keys());
  QHash<QString, QString> savedOffers = m_pendingOffers;
  qDebug() << Q_FUNC_INFO << "[FIX] Saved" << savedPeers.size() << "peers and" << savedOffers.size()
           << "offers for reconnect";
  stop();
  m_pendingPeers = savedPeers;  // restore — teardown cleared it
  m_videoSource  = VideoSourceMode::Camera;
  emit videoSourceChanged();
  if (!buildPipeline())
    return false;
  gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
  m_running = true;
  emit runningChanged();

  // Flush pending peers
  const QList<QString> pending = m_pendingPeers;
  m_pendingPeers.clear();
  for (const QString& id : pending) {
    qDebug() << Q_FUNC_INFO << "[FIX] Flushing peer after camera start:" << id;
    ensurePeer(id);
  }

  // Replay offers that arrived before pipeline was ready (use savedOffers, not m_pendingOffers
  // which was cleared by teardown and never restored)
  for (auto it = savedOffers.cbegin(); it != savedOffers.cend(); ++it) {
    qDebug() << Q_FUNC_INFO << "[FIX] Replaying buffered offer from:" << it.key();
    onRemoteOffer(it.key(), it.value());
  }

  emit info("WebRTC pipeline started (camera)");
  return true;
}

bool WebRtcPeerManager::startWithScreen(int portalFd, uint videoNodeId) {
  qDebug() << Q_FUNC_INFO << "Starting with screen fd:" << portalFd << "node:" << videoNodeId;
  QList<QString>          savedPeers  = m_pendingPeers + QList<QString>(m_peers.keys());
  QHash<QString, QString> savedOffers = m_pendingOffers;
  qDebug() << Q_FUNC_INFO << "[FIX] Saved" << savedPeers.size() << "peers and" << savedOffers.size()
           << "offers for reconnect";
  stop();
  m_pendingPeers = savedPeers;
  m_portalFd     = portalFd;
  m_videoNodeId  = videoNodeId;
  m_videoSource  = VideoSourceMode::ScreenShare;
  emit videoSourceChanged();
  if (!buildPipeline())
    return false;
  gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
  m_running = true;
  emit runningChanged();

  const QList<QString> pending = m_pendingPeers;
  m_pendingPeers.clear();
  for (const QString& id : pending) {
    qDebug() << Q_FUNC_INFO << "[FIX] Flushing peer after screen start:" << id;
    ensurePeer(id);
  }

  for (auto it = savedOffers.cbegin(); it != savedOffers.cend(); ++it) {
    qDebug() << Q_FUNC_INFO << "[FIX] Replaying buffered offer from:" << it.key();
    onRemoteOffer(it.key(), it.value());
  }

  emit info("WebRTC pipeline started (screen share)");
  return true;
}

bool WebRtcPeerManager::startScreenShare() {
  qDebug() << Q_FUNC_INFO << "Starting screen share via QScreenCapture + appsrc";
  QList<QString>          savedPeers  = m_pendingPeers + QList<QString>(m_peers.keys());
  QHash<QString, QString> savedOffers = m_pendingOffers;
  qDebug() << Q_FUNC_INFO << "[FIX] Saved" << savedPeers.size() << "peers and" << savedOffers.size()
           << "offers for reconnect";
  stop();
  m_pendingPeers = savedPeers;
  m_videoSource  = VideoSourceMode::ScreenShare;
  emit videoSourceChanged();

  if (!buildPipeline())
    return false;

  // Setup Qt Multimedia screen capture
  m_screenCapture  = new QScreenCapture(this);
  m_captureSession = new QMediaCaptureSession(this);
  m_videoSink      = new QVideoSink(this);

  QScreen* screen = QGuiApplication::primaryScreen();
  m_screenCapture->setScreen(screen);
  qDebug() << Q_FUNC_INFO << "[FIX] Capturing screen:" << screen->name()
           << "size:" << screen->size();

  m_captureSession->setScreenCapture(m_screenCapture);
  m_captureSession->setVideoSink(m_videoSink);

  QObject::connect(m_videoSink, &QVideoSink::videoFrameChanged, this,
                   &WebRtcPeerManager::onVideoFrame, Qt::DirectConnection);

  m_screenCapture->setActive(true);
  qDebug() << Q_FUNC_INFO << "[FIX] QScreenCapture activated";

  gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
  m_running = true;
  emit runningChanged();

  // Flush peers that called ensurePeer before pipeline was ready
  const QList<QString> pending = m_pendingPeers;
  m_pendingPeers.clear();
  for (const QString& id : pending) {
    qDebug() << Q_FUNC_INFO << "[FIX] Flushing peer after screen share start:" << id;
    ensurePeer(id);
  }

  // Use savedOffers (captured before stop()), not m_pendingOffers which teardown() cleared
  for (auto it = savedOffers.cbegin(); it != savedOffers.cend(); ++it) {
    qDebug() << Q_FUNC_INFO << "[FIX] Replaying buffered offer from:" << it.key();
    onRemoteOffer(it.key(), it.value());
  }

  emit info("WebRTC pipeline started (screen share via QScreenCapture)");
  return true;
}

void WebRtcPeerManager::stop() {
  qDebug() << Q_FUNC_INFO << "Stopping pipeline. peers:" << m_peers.size();
  teardown();
}

void WebRtcPeerManager::switchToCamera() {
  qDebug() << Q_FUNC_INFO << "Switching to camera";
  const bool wasRunning = m_running;
  stop();
  m_videoSource = VideoSourceMode::Camera;
  emit videoSourceChanged();
  if (wasRunning) {
    buildPipeline();
    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    m_running = true;
    emit runningChanged();
  }
}

void WebRtcPeerManager::switchToScreen(int portalFd, uint videoNodeId) {
  qDebug() << Q_FUNC_INFO << "Switching to screen fd:" << portalFd << "node:" << videoNodeId;
  const bool wasRunning = m_running;
  stop();
  m_portalFd    = portalFd;
  m_videoNodeId = videoNodeId;
  m_videoSource = VideoSourceMode::ScreenShare;
  emit videoSourceChanged();
  if (wasRunning) {
    buildPipeline();
    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    m_running = true;
    emit runningChanged();
  }
}

void WebRtcPeerManager::setMicMuted(bool m) {
  if (m_micMuted == m)
    return;
  m_micMuted = m;
  qDebug() << Q_FUNC_INFO << "micMuted:" << m;
  if (m_audioValve) {
    g_object_set(m_audioValve, "drop", m ? TRUE : FALSE, nullptr);
    qDebug() << Q_FUNC_INFO << "[FIX] audioValve drop set to:" << m;
  } else {
    qWarning()
        << Q_FUNC_INFO
        << "[FIX] Pipeline not running — mic mute state saved, will apply on next pipeline start";
  }
  emit micMutedChanged();
}

void WebRtcPeerManager::setCameraMuted(bool m) {
  if (m_cameraMuted == m)
    return;
  m_cameraMuted = m;
  qDebug() << Q_FUNC_INFO << "cameraMuted:" << m;
  if (m_videoValve)
    g_object_set(m_videoValve, "drop", m ? TRUE : FALSE, nullptr);
  emit cameraMutedChanged();
}

void WebRtcPeerManager::ensurePeer(const QString& peerId) {
  qDebug() << Q_FUNC_INFO << "ensurePeer:" << peerId;
  if (!m_pipeline || !m_vtee || !m_atee) {
    // Pipeline not ready yet — buffer peer for when it starts
    if (!m_pendingPeers.contains(peerId)) {
      qDebug() << Q_FUNC_INFO << "[FIX] Pipeline not running, buffering peer:" << peerId;
      m_pendingPeers << peerId;
    }
    return;
  }
  if (m_peers.contains(peerId)) {
    qDebug() << Q_FUNC_INFO << "Peer already exists:" << peerId;
    return;
  }
  addWebRtcBinForPeer(peerId);
}

void WebRtcPeerManager::dropPeer(const QString& peerId) {
  qDebug() << Q_FUNC_INFO << "dropPeer:" << peerId;
  if (!m_peers.contains(peerId))
    return;
  Peer p = m_peers.take(peerId);
  teardownPeer(p);
  emit peerCountChanged();
}

VideoFrameSink* WebRtcPeerManager::sinkForPeer(const QString& peerId) const {
  return m_peers.value(peerId).sink;
}

void WebRtcPeerManager::connectVideoTile(const QString& peerId, QObject* tile) {
  VideoFrameSink* sink = sinkForPeer(peerId);
  if (!sink || !tile) {
    qWarning() << Q_FUNC_INFO << "Cannot connect tile — sink or tile null for peer:" << peerId
               << "sink:" << sink << "tile:" << tile;
    return;
  }
  qDebug() << Q_FUNC_INFO << "Connecting VideoFrameSink to tile for peer:" << peerId;
  // Disconnect old connections first to avoid duplicates if called again after restart
  QObject::disconnect(sink, SIGNAL(frameReady(QImage)), tile, SLOT(onFrame(QImage)));
  QObject::connect(sink, SIGNAL(frameReady(QImage)), tile, SLOT(onFrame(QImage)));
}

void WebRtcPeerManager::connectLocalVideoTile(QObject* tile) {
  if (!m_localSink || !tile) {
    qWarning() << Q_FUNC_INFO << "Cannot connect local tile — localSink:" << m_localSink
               << "tile:" << tile;
    return;
  }
  qDebug() << Q_FUNC_INFO << "Connecting local VideoFrameSink to tile";
  QObject::disconnect(m_localSink, SIGNAL(frameReady(QImage)), tile, SLOT(onFrame(QImage)));
  QObject::connect(m_localSink, SIGNAL(frameReady(QImage)), tile, SLOT(onFrame(QImage)));
}

// ---------- inbound signaling ------------------------------------------------

void WebRtcPeerManager::onRemoteOffer(const QString& peerId, const QString& sdp) {
  qDebug() << Q_FUNC_INFO << "Remote offer from:" << peerId;
  if (!m_pipeline) {
    // Buffer the offer — pipeline not started yet (user hasn't clicked camera/screen share).
    // Will be drained after pipeline starts, just like ICE candidates are buffered.
    qDebug() << Q_FUNC_INFO << "[FIX] Buffering offer from peer:" << peerId
             << "(pipeline not running yet)";
    m_pendingOffers[peerId] = sdp;
    return;
  }
  ensurePeer(peerId);
  if (!m_peers.contains(peerId))
    return;
  Peer& p = m_peers[peerId];

  // Set remote description
  GstSDPMessage* sdpMsg = nullptr;
  gst_sdp_message_new(&sdpMsg);
  const QByteArray utf8 = sdp.toUtf8();
  if (gst_sdp_message_parse_buffer((guint8*)utf8.constData(), utf8.size(), sdpMsg) != GST_SDP_OK) {
    qWarning() << Q_FUNC_INFO << "Failed to parse remote SDP offer";
    gst_sdp_message_free(sdpMsg);
    return;
  }
  GstWebRTCSessionDescription* desc =
      gst_webrtc_session_description_new(GST_WEBRTC_SDP_TYPE_OFFER, sdpMsg);

  GstPromise* pr = gst_promise_new();
  g_signal_emit_by_name(p.webrtcbin, "set-remote-description", desc, pr);
  gst_promise_interrupt(pr);
  gst_promise_unref(pr);
  gst_webrtc_session_description_free(desc);

  // Create answer
  struct Ctx {
    WebRtcPeerManager* self;
    QString            peerId;
    GstElement*        webrtcbin;
  };
  auto* ctx = new Ctx{this, peerId, p.webrtcbin};

  GstPromise* answerPromise = gst_promise_new_with_change_func(
      &WebRtcPeerManager::onAnswerCreated, ctx, [](gpointer p) { delete static_cast<Ctx*>(p); });

  g_signal_emit_by_name(p.webrtcbin, "create-answer", nullptr, answerPromise);
  qDebug() << Q_FUNC_INFO << "Answer creation requested for peer:" << peerId;
}

void WebRtcPeerManager::onRemoteAnswer(const QString& peerId, const QString& sdp) {
  qDebug() << Q_FUNC_INFO << "Remote answer from:" << peerId;
  if (!m_peers.contains(peerId)) {
    qWarning() << Q_FUNC_INFO << "Unknown peer:" << peerId;
    return;
  }
  Peer& p = m_peers[peerId];

  GstSDPMessage* sdpMsg = nullptr;
  gst_sdp_message_new(&sdpMsg);
  const QByteArray utf8 = sdp.toUtf8();
  if (gst_sdp_message_parse_buffer((guint8*)utf8.constData(), utf8.size(), sdpMsg) != GST_SDP_OK) {
    qWarning() << Q_FUNC_INFO << "Failed to parse remote SDP answer from:" << peerId;
    gst_sdp_message_free(sdpMsg);
    return;
  }
  GstWebRTCSessionDescription* desc =
      gst_webrtc_session_description_new(GST_WEBRTC_SDP_TYPE_ANSWER, sdpMsg);

  GstPromise* pr = gst_promise_new();
  g_signal_emit_by_name(p.webrtcbin, "set-remote-description", desc, pr);
  gst_promise_interrupt(pr);
  gst_promise_unref(pr);
  gst_webrtc_session_description_free(desc);
  qDebug() << Q_FUNC_INFO << "Remote answer applied for peer:" << peerId;
}

void WebRtcPeerManager::onRemoteIce(const QString& peerId, const IceCandidate& ice) {
  qDebug() << Q_FUNC_INFO << "Remote ICE from:" << peerId << "sdpMid:" << ice.sdpMid
           << "mline:" << ice.sdpMLineIndex;
  if (!m_peers.contains(peerId)) {
    qDebug() << Q_FUNC_INFO << "[FIX] Buffering ICE for unknown peer:" << peerId;
    m_pendingIce[peerId] << ice;
    return;
  }
  Peer&            p = m_peers[peerId];
  const QByteArray c = ice.candidate.toUtf8();
  g_signal_emit_by_name(p.webrtcbin, "add-ice-candidate", (guint)ice.sdpMLineIndex, c.constData());
}

// ---------- Qt Multimedia frame callback -------------------------------------

void WebRtcPeerManager::onVideoFrame(const QVideoFrame& frame) {
  if (!m_appsrc || !m_running || !frame.isValid())
    return;

  // Convert to RGBA8888 for consistent GStreamer caps
  QImage img = frame.toImage();
  if (img.isNull()) {
    qWarning() << "[WebRtcPeerManager] onVideoFrame: toImage returned null";
    return;
  }
  if (img.format() != QImage::Format_RGBA8888)
    img = img.convertToFormat(QImage::Format_RGBA8888);

  const qsizetype byteCount = img.sizeInBytes();
  GstBuffer*      buf       = gst_buffer_new_allocate(nullptr, byteCount, nullptr);

  GstMapInfo map;
  if (!gst_buffer_map(buf, &map, GST_MAP_WRITE)) {
    qWarning() << "[WebRtcPeerManager] onVideoFrame: gst_buffer_map failed";
    gst_buffer_unref(buf);
    return;
  }
  memcpy(map.data, img.constBits(), byteCount);
  gst_buffer_unmap(buf, &map);

  // appsrc takes ownership of buf regardless of return value
  const GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(m_appsrc), buf);
  if (ret != GST_FLOW_OK) {
    qWarning() << "[WebRtcPeerManager] onVideoFrame: push_buffer returned:" << ret;
  }
}

void WebRtcPeerManager::onAudioDataReady() {
  if (!m_running || !m_audioIODevice)
    return;

  // Read available audio data
  QByteArray data = m_audioIODevice->readAll();
  if (data.isEmpty())
    return;

  // For now, we'll just log that audio data is available
  // In a full implementation, this would push data to GStreamer pipeline
  qDebug() << "[WebRtcPeerManager] Audio data ready, size:" << data.size();
}

// ---------- pipeline build/teardown ------------------------------------------

bool WebRtcPeerManager::buildPipeline() {
  qDebug() << Q_FUNC_INFO << "Building pipeline. videoSource:"
           << (m_videoSource == VideoSourceMode::Camera ? "camera" : "screen");

  m_pipeline = gst_pipeline_new("webrtc-chat");
  if (!m_pipeline) {
    emit error("Failed to create GStreamer pipeline");
    return false;
  }

  // Check webrtcbin availability early — it is created dynamically in addWebRtcBinForPeer,
  // not in the static element table below, so missing gst-plugins-bad would only surface
  // later with a cryptic "Failed to create webrtcbin" message.
  {
    GstElementFactory* wbf = gst_element_factory_find("webrtcbin");
    if (!wbf) {
      qWarning() << Q_FUNC_INFO << "[FIX] webrtcbin factory NOT in GStreamer registry!"
                 << "gst-plugins-bad is required.";
      qWarning() << Q_FUNC_INFO << "[FIX] Arch Linux:  sudo pacman -S gst-plugins-bad";
      qWarning() << Q_FUNC_INFO << "[FIX] Verify:      gst-inspect-1.0 webrtcbin";
      emit error(
          "WebRTC requires gst-plugins-bad — Arch: sudo pacman -S gst-plugins-bad\n"
          "Verify with: gst-inspect-1.0 webrtcbin");
      gst_object_unref(m_pipeline);
      m_pipeline = nullptr;
      return false;
    }
    gst_object_unref(wbf);  // just checking availability, don't keep the ref
  }

  // --- VIDEO SOURCE ---
  GstElement* vsrc = nullptr;
  m_appsrc         = nullptr;

  if (m_videoSource == VideoSourceMode::Camera) {
    vsrc = gst_element_factory_make("autovideosrc", "vsrc");
    if (!vsrc)
      vsrc = gst_element_factory_make("v4l2src", "vsrc");
    if (!vsrc) {
      qWarning() << Q_FUNC_INFO
                 << "[FIX] autovideosrc/v4l2src not found — no camera source available";
      emit error("Camera source not available (autovideosrc / v4l2src)");
      gst_object_unref(m_pipeline);
      m_pipeline = nullptr;
      return false;
    }
    qDebug() << Q_FUNC_INFO << "[FIX] Camera source created";
  } else {
    // Screen share via Qt Multimedia — use appsrc, frames pushed from onVideoFrame()
    vsrc = gst_element_factory_make("appsrc", "vsrc");
    if (!vsrc) {
      qWarning() << Q_FUNC_INFO << "[FIX] appsrc not found (GStreamer base missing?)";
      emit error("Screen share requires GStreamer appsrc (gst-plugins-base)");
      gst_object_unref(m_pipeline);
      m_pipeline = nullptr;
      return false;
    }
    m_appsrc = vsrc;

    // Get screen dimensions for caps
    QScreen*  screen = QGuiApplication::primaryScreen();
    const int w      = screen ? screen->size().width() : 1920;
    const int h      = screen ? screen->size().height() : 1080;

    // RGBA caps matching what onVideoFrame() pushes via QImage::Format_RGBA8888
    const QByteArray capsStr =
        QString("video/x-raw,format=RGBA,width=%1,height=%2,framerate=0/1").arg(w).arg(h).toUtf8();
    GstCaps* appsrcCaps = gst_caps_from_string(capsStr.constData());
    g_object_set(vsrc, "caps", appsrcCaps, "is-live", TRUE, "format", GST_FORMAT_TIME,
                 "do-timestamp", TRUE, nullptr);
    gst_caps_unref(appsrcCaps);
    qDebug() << Q_FUNC_INFO << "[FIX] appsrc configured:" << capsStr << "screen" << w << "x" << h;
  }

  GstElement* vconv  = gst_element_factory_make("videoconvert", "vconv");
  GstElement* vscale = gst_element_factory_make("videoscale", "vscale");
  GstElement* vcaps  = gst_element_factory_make("capsfilter", "vcaps");
  GstElement* venc   = gst_element_factory_make("vp8enc", "venc");
  GstElement* vpay   = gst_element_factory_make("rtpvp8pay", "vpay");
  m_videoValve       = gst_element_factory_make("valve", "vvalve");
  m_vtee             = gst_element_factory_make("tee", "vtee");

  // Local preview sink
  m_localSink = new VideoFrameSink(this);
  m_localSink->setPeerId("local");
  GstElement* lpreview = m_localSink->element();
  if (lpreview)
    gst_object_ref(lpreview);  // pipeline will own a ref

  // --- AUDIO SOURCE ---
  // Always use autoaudiosrc (microphone). pipewiresrc for audio without a specific
  // node starts streaming immediately and errors with GST_FLOW_NOT_LINKED when no
  // peers are connected yet. autoaudiosrc is more tolerant of this startup race.
  GstElement* asrc = gst_element_factory_make("autoaudiosrc", "asrc");
  if (!asrc)
    asrc = gst_element_factory_make("pipewiresrc", "asrc");
  GstElement* aconv = gst_element_factory_make("audioconvert", "aconv");
  GstElement* ares  = gst_element_factory_make("audioresample", "ares");
  GstElement* acaps = gst_element_factory_make("capsfilter", "acaps");
  GstElement* aenc  = gst_element_factory_make("opusenc", "aenc");
  GstElement* apay  = gst_element_factory_make("rtpopuspay", "apay");
  m_audioValve      = gst_element_factory_make("valve", "avalve");
  m_atee            = gst_element_factory_make("tee", "atee");

  // Check required elements BEFORE calling g_object_set on any of them
  // (g_object_set on a null pointer causes GLib-GObject-CRITICAL assertions)
  {
    struct {
      const char* name;
      const char* pkg;
      GstElement* el;
    } checks[] = {
        {"videoconvert", "gst-plugins-base", vconv},
        {"videoscale", "gst-plugins-base", vscale},
        {"capsfilter", "gst-plugins-base", vcaps},
        {"vp8enc", "gst-plugins-good (VP8/libvpx) — Arch: sudo pacman -S gst-plugins-good", venc},
        {"rtpvp8pay", "gst-plugins-good (RTP VP8 payloader)", vpay},
        {"valve", "gst-plugins-good", m_videoValve},
        {"tee", "gst-plugins-base", m_vtee},
        {"autoaudiosrc", "gst-plugins-good", asrc},
        {"audioconvert", "gst-plugins-base", aconv},
        {"audioresample", "gst-plugins-base", ares},
        {"capsfilter(a)", "gst-plugins-base", acaps},
        {"opusenc",
         "gst-plugins-base (NOTE: gst-plugins-base-libs is NOT enough) — Arch: sudo pacman -S gst-plugins-base; verify: gst-inspect-1.0 opusenc",
         aenc},
        {"rtpopuspay",
         "gst-plugins-good (RTP Opus payloader) — Arch: sudo pacman -S gst-plugins-good; verify: gst-inspect-1.0 rtpopuspay",
         apay},
        {"valve(a)", "gst-plugins-good", m_audioValve},
        {"tee(a)", "gst-plugins-base", m_atee},
    };
    bool missing = false;
    for (auto& c : checks) {
      if (!c.el) {
        qWarning() << Q_FUNC_INFO << "[FIX] Missing element:" << c.name << "— install:" << c.pkg;
        missing = true;
      }
    }
    if (missing) {
      emit error("Missing required GStreamer elements — check logs for install instructions");
      gst_object_unref(m_pipeline);
      m_pipeline = nullptr;
      m_vtee = m_atee = m_audioValve = m_videoValve = nullptr;
      return false;
    }
  }

  // Allow tees to operate with zero connected src pads during startup.
  // Screen share (appsrc) starts pushing frames immediately, and audio also starts
  // immediately. Without this, GST_FLOW_NOT_LINKED propagates back as a fatal error
  // before any peer has connected and created tee branches.
  g_object_set(m_vtee, "allow-not-linked", TRUE, nullptr);
  g_object_set(m_atee, "allow-not-linked", TRUE, nullptr);
  qDebug() << Q_FUNC_INFO << "[FIX] tee allow-not-linked=TRUE set on vtee and atee";

  // All elements confirmed non-null — safe to configure properties
  // Video caps: 720p. For screen share (variable framerate appsrc) omit framerate
  // constraint to avoid backwards-propagation that causes caps negotiation failure.
  GstCaps* vCaps = (m_videoSource == VideoSourceMode::ScreenShare)
                       ? gst_caps_from_string("video/x-raw,width=1280,height=720")
                       : gst_caps_from_string("video/x-raw,width=1280,height=720,framerate=30/1");
  qDebug() << Q_FUNC_INFO << "[FIX] downstream capsfilter:"
           << (m_videoSource == VideoSourceMode::ScreenShare ? "no framerate" : "30fps");
  g_object_set(vcaps, "caps", vCaps, nullptr);
  gst_caps_unref(vCaps);

  // VP8 low-latency tuning
  g_object_set(venc, "deadline", 1, "cpu-used", 8, "threads", 4, "error-resilient", TRUE, nullptr);
  g_object_set(vpay, "pt", 96, nullptr);

  GstCaps* aCaps = gst_caps_from_string("audio/x-raw,rate=48000,channels=2");
  g_object_set(acaps, "caps", aCaps, nullptr);
  gst_caps_unref(aCaps);

  g_object_set(aenc, "bitrate", 64000, "inband-fec", TRUE, nullptr);
  g_object_set(apay, "pt", 111, nullptr);

  // Apply current mute state
  g_object_set(m_audioValve, "drop", m_micMuted ? TRUE : FALSE, nullptr);
  g_object_set(m_videoValve, "drop", m_cameraMuted ? TRUE : FALSE, nullptr);

  // Add all elements
  if (lpreview) {
    gst_bin_add_many(GST_BIN(m_pipeline), vsrc, vconv, vscale, vcaps, m_videoValve, venc, vpay,
                     m_vtee, asrc, aconv, ares, acaps, m_audioValve, aenc, apay, m_atee, lpreview,
                     nullptr);
  } else {
    gst_bin_add_many(GST_BIN(m_pipeline), vsrc, vconv, vscale, vcaps, m_videoValve, venc, vpay,
                     m_vtee, asrc, aconv, ares, acaps, m_audioValve, aenc, apay, m_atee, nullptr);
  }

  // Link video chain
  if (!gst_element_link_many(vsrc, vconv, vscale, vcaps, m_videoValve, venc, vpay, m_vtee,
                             nullptr)) {
    qWarning() << Q_FUNC_INFO << "Failed to link video chain";
    emit error("Failed to link video chain");
    teardown();
    return false;
  }

  // Link audio chain
  if (!gst_element_link_many(asrc, aconv, ares, acaps, m_audioValve, aenc, apay, m_atee, nullptr)) {
    qWarning() << Q_FUNC_INFO << "Failed to link audio chain";
    emit error("Failed to link audio chain");
    teardown();
    return false;
  }

  // Connect local preview to video tee
  if (lpreview) {
    GstPad* teeSrc  = gst_element_request_pad_simple(m_vtee, "src_%u");
    GstPad* sinkPad = gst_element_get_static_pad(lpreview, "sink");
    if (teeSrc && sinkPad) {
      const auto res = gst_pad_link(teeSrc, sinkPad);
      qDebug() << Q_FUNC_INFO << "Local preview tee link result:" << res;
    }
    if (teeSrc)
      gst_object_unref(teeSrc);
    if (sinkPad)
      gst_object_unref(sinkPad);
  }

  // Setup bus watch
  GstBus* bus = gst_element_get_bus(m_pipeline);
  gst_bus_add_watch(bus, busCallback, this);
  gst_object_unref(bus);

  qDebug() << Q_FUNC_INFO << "Pipeline built successfully";
  return true;
}

void WebRtcPeerManager::teardown() {
  qDebug() << Q_FUNC_INFO << "Tearing down. peers:" << m_peers.size();

  for (auto it = m_peers.begin(); it != m_peers.end(); ++it) {
    teardownPeer(it.value());
  }
  m_peers.clear();

  // Stop Qt Multimedia screen capture before tearing down GStreamer pipeline
  if (m_screenCapture) {
    qDebug() << Q_FUNC_INFO << "[FIX] Stopping QScreenCapture";
    m_screenCapture->setActive(false);
    delete m_screenCapture;
    m_screenCapture = nullptr;
  }
  delete m_captureSession;
  m_captureSession = nullptr;
  delete m_videoSink;
  m_videoSink = nullptr;
  m_appsrc    = nullptr;  // owned by pipeline, will be freed below

  if (m_pipeline) {
    gst_element_set_state(m_pipeline, GST_STATE_NULL);
    gst_object_unref(m_pipeline);
    m_pipeline = nullptr;
  }

  delete m_localSink;
  m_localSink = nullptr;

  m_vtee = m_atee = m_audioValve = m_videoValve = nullptr;
  m_pendingPeers.clear();
  m_pendingIce.clear();
  m_pendingOffers.clear();

  if (m_running) {
    m_running = false;
    emit runningChanged();
  }
  emit peerCountChanged();
  qDebug() << Q_FUNC_INFO << "Teardown complete";
}

void WebRtcPeerManager::teardownPeer(Peer& p) {
  qDebug() << Q_FUNC_INFO << "Tearing down peer:" << p.id;
  if (p.webrtcbin && m_pipeline) {
    gst_element_set_state(p.webrtcbin, GST_STATE_NULL);
    gst_bin_remove(GST_BIN(m_pipeline), p.webrtcbin);
  }
  delete p.sink;
  p.sink      = nullptr;
  p.webrtcbin = nullptr;
}

bool WebRtcPeerManager::addWebRtcBinForPeer(const QString& peerId) {
  qDebug() << Q_FUNC_INFO << "Adding webrtcbin for peer:" << peerId;

  GstElement* wb = gst_element_factory_make("webrtcbin", ("wb_" + peerId).toUtf8().constData());
  if (!wb) {
    qWarning() << Q_FUNC_INFO << "Failed to create webrtcbin for peer:" << peerId;
    emit error("Failed to create webrtcbin");
    return false;
  }

  g_object_set(wb, "bundle-policy", GST_WEBRTC_BUNDLE_POLICY_MAX_BUNDLE, "latency", 0,
               "stun-server", "stun://stun.l.google.com:19302", nullptr);
  qDebug() << Q_FUNC_INFO << "[FIX] webrtcbin stun-server set: stun://stun.l.google.com:19302";

  gst_bin_add(GST_BIN(m_pipeline), wb);
  gst_element_sync_state_with_parent(wb);

  if (!linkTeeToWebrtcBin(m_vtee, wb, "video_" + peerId)
      || !linkTeeToWebrtcBin(m_atee, wb, "audio_" + peerId)) {
    qWarning() << Q_FUNC_INFO << "Failed to link tee to webrtcbin for peer:" << peerId;
    gst_element_set_state(wb, GST_STATE_NULL);
    gst_bin_remove(GST_BIN(m_pipeline), wb);
    emit error("Failed to connect peer " + peerId + " to pipeline");
    return false;
  }

  // Create remote video sink.
  // gst_bin_add sinks the floating ref, transferring pipeline ownership of the element.
  // VideoFrameSink::~VideoFrameSink() also calls gst_object_unref, so we must add an
  // extra ref here (same pattern used for lpreview in buildPipeline). Without this,
  // teardownPeer's "delete p.sink" drops refcount to 0 while the appsink is still in
  // the pipeline (linked to decode chain), causing GStreamer-CRITICAL on teardown.
  VideoFrameSink* sink = new VideoFrameSink(this);
  sink->setPeerId(peerId);
  if (sink->element()) {
    gst_object_ref(
        sink->element());  // VideoFrameSink keeps this ref; pipeline gets its own via gst_bin_add
  }
  gst_bin_add(GST_BIN(m_pipeline), sink->element());
  gst_element_sync_state_with_parent(sink->element());
  qDebug() << Q_FUNC_INFO
           << "[FIX] Peer appsink added with extra ref — refcount balanced for teardown";

  // Connect pad-added for incoming remote streams
  g_signal_connect(wb, "pad-added", G_CALLBACK(WebRtcPeerManager::onPadAdded), this);

  // Connect negotiation-needed → create offer
  g_signal_connect(wb, "on-negotiation-needed", G_CALLBACK(WebRtcPeerManager::onNegotiationNeeded),
                   this);

  // Connect ICE candidate
  g_signal_connect(wb, "on-ice-candidate", G_CALLBACK(WebRtcPeerManager::onIceCandidate), this);

  // Monitor ICE and DTLS connection-state via GObject property notifications.
  // webrtcbin exposes these as read-only properties, NOT dedicated signals.
  // The correct GLib mechanism is notify::property-name, not a standalone signal.
  g_signal_connect(wb, "notify::ice-connection-state",
                   G_CALLBACK(WebRtcPeerManager::onIceConnectionStateChange), this);
  g_signal_connect(wb, "notify::connection-state",
                   G_CALLBACK(WebRtcPeerManager::onConnectionStateChange), this);

  Peer p;
  p.id        = peerId;
  p.webrtcbin = wb;
  p.sink      = sink;
  m_peers.insert(peerId, p);
  emit peerCountChanged();

  qDebug() << Q_FUNC_INFO << "Peer added successfully:" << peerId
           << "total peers:" << m_peers.size();

  // Drain any ICE candidates that arrived before this peer was added
  if (m_pendingIce.contains(peerId)) {
    const QList<IceCandidate> pending = m_pendingIce.take(peerId);
    qDebug() << Q_FUNC_INFO << "[FIX] Draining" << pending.size()
             << "buffered ICE candidates for peer:" << peerId;
    Peer& addedPeer = m_peers[peerId];
    for (const IceCandidate& ice : pending) {
      const QByteArray c = ice.candidate.toUtf8();
      g_signal_emit_by_name(addedPeer.webrtcbin, "add-ice-candidate", (guint)ice.sdpMLineIndex,
                            c.constData());
    }
  }

  return true;
}

bool WebRtcPeerManager::linkTeeToWebrtcBin(GstElement* tee, GstElement* webrtcbin,
                                           const QString& label) {
  GstElement* q = gst_element_factory_make("queue", ("q_" + label).toUtf8().constData());
  if (!q) {
    qWarning() << Q_FUNC_INFO << "Failed to create queue for:" << label;
    return false;
  }
  gst_bin_add(GST_BIN(m_pipeline), q);
  gst_element_sync_state_with_parent(q);

  GstPad* teeSrc = gst_element_request_pad_simple(tee, "src_%u");
  GstPad* qSink  = gst_element_get_static_pad(q, "sink");

  if (!teeSrc || !qSink || gst_pad_link(teeSrc, qSink) != GST_PAD_LINK_OK) {
    qWarning() << Q_FUNC_INFO << "Failed to link tee->queue for:" << label;
    if (teeSrc)
      gst_object_unref(teeSrc);
    if (qSink)
      gst_object_unref(qSink);
    return false;
  }
  gst_object_unref(teeSrc);
  gst_object_unref(qSink);

  GstPad* qSrc   = gst_element_get_static_pad(q, "src");
  GstPad* wbSink = gst_element_request_pad_simple(webrtcbin, "sink_%u");

  if (!qSrc || !wbSink || gst_pad_link(qSrc, wbSink) != GST_PAD_LINK_OK) {
    qWarning() << Q_FUNC_INFO << "Failed to link queue->webrtcbin for:" << label;
    if (qSrc)
      gst_object_unref(qSrc);
    if (wbSink)
      gst_object_unref(wbSink);
    return false;
  }
  gst_object_unref(qSrc);
  gst_object_unref(wbSink);

  qDebug() << Q_FUNC_INFO << "Linked tee->queue->webrtcbin for:" << label;
  return true;
}

// ---------- sdpMid extraction ------------------------------------------------

QString WebRtcPeerManager::extractSdpMid(GstElement* webrtcbin, int mlineIndex) const {
  GstWebRTCSessionDescription* localDesc = nullptr;
  g_object_get(webrtcbin, "local-description", &localDesc, nullptr);
  if (!localDesc) {
    qWarning() << Q_FUNC_INFO << "No local-description yet, using mline as string";
    return QString::number(mlineIndex);
  }

  QString     mid;
  const guint count = gst_sdp_message_medias_len(localDesc->sdp);
  if ((guint)mlineIndex < count) {
    const GstSDPMedia* media = gst_sdp_message_get_media(localDesc->sdp, (guint)mlineIndex);
    const char*        val   = gst_sdp_media_get_attribute_val(media, "mid");
    if (val)
      mid = QString::fromUtf8(val);
  }

  gst_webrtc_session_description_free(localDesc);

  if (mid.isEmpty())
    mid = QString::number(mlineIndex);
  qDebug() << Q_FUNC_INFO << "mlineIndex:" << mlineIndex << "-> sdpMid:" << mid;
  return mid;
}

// ---------- GStreamer callbacks -----------------------------------------------

WebRtcPeerManager::Peer* WebRtcPeerManager::peerByWebrtcBin(GstElement* bin) {
  for (auto it = m_peers.begin(); it != m_peers.end(); ++it) {
    if (it->webrtcbin == bin)
      return &it.value();
  }
  return nullptr;
}

void WebRtcPeerManager::onNegotiationNeeded(GstElement* webrtcbin, gpointer user_data) {
  auto* self = static_cast<WebRtcPeerManager*>(user_data);
  Peer* p    = self->peerByWebrtcBin(webrtcbin);
  if (!p) {
    qWarning() << "[WebRtcPeerManager] onNegotiationNeeded: unknown webrtcbin";
    return;
  }
  qDebug() << "[WebRtcPeerManager] onNegotiationNeeded for peer:" << p->id;

  struct Ctx {
    WebRtcPeerManager* self;
    QString            peerId;
    GstElement*        wb;
  };
  auto* ctx = new Ctx{self, p->id, webrtcbin};

  GstPromise* promise = gst_promise_new_with_change_func(
      &WebRtcPeerManager::onOfferCreated, ctx, [](gpointer p) { delete static_cast<Ctx*>(p); });

  g_signal_emit_by_name(webrtcbin, "create-offer", nullptr, promise);
}

void WebRtcPeerManager::onOfferCreated(GstPromise* promise, gpointer user_data) {
  struct Ctx {
    WebRtcPeerManager* self;
    QString            peerId;
    GstElement*        wb;
  };
  auto* ctx = static_cast<Ctx*>(user_data);

  const GstStructure*          reply = gst_promise_get_reply(promise);
  GstWebRTCSessionDescription* offer = nullptr;
  gst_structure_get(reply, "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, nullptr);
  gst_promise_unref(promise);

  if (!offer) {
    qWarning() << "[WebRtcPeerManager] onOfferCreated: null offer for peer:" << ctx->peerId;
    return;
  }

  // Set local description
  GstPromise* pr = gst_promise_new();
  g_signal_emit_by_name(ctx->wb, "set-local-description", offer, pr);
  gst_promise_interrupt(pr);
  gst_promise_unref(pr);

  gchar*        sdpText = gst_sdp_message_as_text(offer->sdp);
  const QString sdp     = QString::fromUtf8(sdpText);
  g_free(sdpText);
  gst_webrtc_session_description_free(offer);

  const QString peerId = ctx->peerId;
  qDebug() << "[WebRtcPeerManager] Offer created for peer:" << peerId
           << "sdp length:" << sdp.length();

  QMetaObject::invokeMethod(
      ctx->self, [self = ctx->self, peerId, sdp]() { emit self->localOfferReady(peerId, sdp); },
      Qt::QueuedConnection);
}

void WebRtcPeerManager::onAnswerCreated(GstPromise* promise, gpointer user_data) {
  struct Ctx {
    WebRtcPeerManager* self;
    QString            peerId;
    GstElement*        wb;
  };
  auto* ctx = static_cast<Ctx*>(user_data);

  const GstStructure*          reply  = gst_promise_get_reply(promise);
  GstWebRTCSessionDescription* answer = nullptr;
  gst_structure_get(reply, "answer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answer, nullptr);
  gst_promise_unref(promise);

  if (!answer) {
    qWarning() << "[WebRtcPeerManager] onAnswerCreated: null answer for peer:" << ctx->peerId;
    return;
  }

  GstPromise* pr = gst_promise_new();
  g_signal_emit_by_name(ctx->wb, "set-local-description", answer, pr);
  gst_promise_interrupt(pr);
  gst_promise_unref(pr);

  gchar*        sdpText = gst_sdp_message_as_text(answer->sdp);
  const QString sdp     = QString::fromUtf8(sdpText);
  g_free(sdpText);
  gst_webrtc_session_description_free(answer);

  const QString peerId = ctx->peerId;
  qDebug() << "[WebRtcPeerManager] Answer created for peer:" << peerId;

  QMetaObject::invokeMethod(
      ctx->self, [self = ctx->self, peerId, sdp]() { emit self->localAnswerReady(peerId, sdp); },
      Qt::QueuedConnection);
}

void WebRtcPeerManager::onIceCandidate(GstElement* webrtcbin, guint mline, gchar* candidate,
                                       gpointer user_data) {
  auto* self = static_cast<WebRtcPeerManager*>(user_data);
  Peer* p    = self->peerByWebrtcBin(webrtcbin);
  if (!p)
    return;

  const QString cand   = QString::fromUtf8(candidate);
  const QString sdpMid = self->extractSdpMid(webrtcbin, (int)mline);

  IceCandidate ice;
  ice.candidate     = cand;
  ice.sdpMid        = sdpMid;
  ice.sdpMLineIndex = (int)mline;

  qDebug() << "[WebRtcPeerManager] ICE candidate for peer:" << p->id << "sdpMid:" << sdpMid
           << "mline:" << mline;

  const QString peerId = p->id;
  QMetaObject::invokeMethod(
      self, [self, peerId, ice]() { emit self->localIceReady(peerId, ice); }, Qt::QueuedConnection);
}

void WebRtcPeerManager::onPadAdded(GstElement* webrtcbin, GstPad* pad, gpointer user_data) {
  auto* self = static_cast<WebRtcPeerManager*>(user_data);
  Peer* p    = self->peerByWebrtcBin(webrtcbin);

  const gchar* padName = GST_PAD_NAME(pad);
  qDebug() << "[WebRtcPeerManager] pad-added on webrtcbin pad:" << padName
           << "peer:" << (p ? p->id : "unknown");

  if (GST_PAD_DIRECTION(pad) != GST_PAD_SRC)
    return;

  GstCaps* caps = gst_pad_get_current_caps(pad);
  if (!caps)
    caps = gst_pad_query_caps(pad, nullptr);
  if (!caps) {
    qWarning() << "[WebRtcPeerManager] Cannot get caps for pad:" << padName;
    return;
  }

  const gchar* mediaType = gst_structure_get_name(gst_caps_get_structure(caps, 0));
  qDebug() << "[WebRtcPeerManager] Incoming media type:" << mediaType;
  gst_caps_unref(caps);

  // Only handle video — audio playback handled by autoaudiosink
  if (!g_str_has_prefix(mediaType, "application/x-rtp"))
    return;

  // Build: decodebin -> videoconvert -> videoscale -> appsink
  GstElement* decodebin = gst_element_factory_make("decodebin", nullptr);
  GstElement* vconv2    = gst_element_factory_make("videoconvert", nullptr);
  GstElement* vscale2   = gst_element_factory_make("videoscale", nullptr);

  VideoFrameSink* sink      = p ? p->sink : nullptr;
  GstElement*     appsinkEl = sink ? sink->element() : nullptr;

  if (!decodebin || !vconv2 || !vscale2 || !appsinkEl) {
    qWarning() << "[WebRtcPeerManager] Missing decode elements" << decodebin << vconv2 << vscale2
               << appsinkEl;
    if (decodebin)
      gst_object_unref(decodebin);
    if (vconv2)
      gst_object_unref(vconv2);
    if (vscale2)
      gst_object_unref(vscale2);
    return;
  }

  gst_bin_add_many(GST_BIN(self->m_pipeline), decodebin, vconv2, vscale2, nullptr);
  gst_element_sync_state_with_parent(decodebin);
  gst_element_sync_state_with_parent(vconv2);
  gst_element_sync_state_with_parent(vscale2);

  // decodebin's src is dynamic — connect via pad-added
  // Note: use a named typedef to avoid brace-init inside g_signal_connect macro args
  struct DecodeCtx {
    GstElement* vconv;
    GstElement* vscale;
    GstElement* appsink;
  };
  auto* ctx2 = new DecodeCtx{vconv2, vscale2, appsinkEl};

  g_signal_connect(
      decodebin, "pad-added", G_CALLBACK(+[](GstElement*, GstPad* decodePad, gpointer ud) {
        auto* c = static_cast<DecodeCtx*>(ud);

        GstCaps* pcaps = gst_pad_get_current_caps(decodePad);
        if (!pcaps)
          pcaps = gst_pad_query_caps(decodePad, nullptr);
        bool isVideo = false;
        if (pcaps) {
          isVideo =
              g_str_has_prefix(gst_structure_get_name(gst_caps_get_structure(pcaps, 0)), "video/");
          gst_caps_unref(pcaps);
        }

        if (isVideo) {
          GstPad* sinkPad = gst_element_get_static_pad(c->vconv, "sink");
          if (sinkPad && gst_pad_link(decodePad, sinkPad) == GST_PAD_LINK_OK) {
            gst_element_link_many(c->vconv, c->vscale, c->appsink, nullptr);
            qDebug() << "[WebRtcPeerManager] Linked decode->vconv->vscale->appsink";
          }
          if (sinkPad)
            gst_object_unref(sinkPad);
        }
        delete c;
      }),
      ctx2);

  // Link webrtcbin src pad -> decodebin
  GstPad* decoderSink = gst_element_get_static_pad(decodebin, "sink");
  if (decoderSink) {
    gst_pad_link(pad, decoderSink);
    gst_object_unref(decoderSink);
  }

  qDebug() << "[WebRtcPeerManager] Receive pipeline connected for peer:" << (p ? p->id : "unknown");
}

void WebRtcPeerManager::onIceConnectionStateChange(GObject* webrtcbin, GParamSpec* /*pspec*/,
                                                   gpointer user_data) {
  // notify::ice-connection-state fires on the GStreamer streaming thread.
  // Read the property immediately (on this thread), then marshal string to Qt main thread.
  auto*         self   = static_cast<WebRtcPeerManager*>(user_data);
  Peer*         p      = self->peerByWebrtcBin(GST_ELEMENT(webrtcbin));
  const QString peerId = p ? p->id : QStringLiteral("unknown");

  GstWebRTCICEConnectionState state = GST_WEBRTC_ICE_CONNECTION_STATE_NEW;
  g_object_get(webrtcbin, "ice-connection-state", &state, nullptr);

  const char* stateStr = "unknown";
  switch (state) {
    case GST_WEBRTC_ICE_CONNECTION_STATE_NEW:
      stateStr = "new";
      break;
    case GST_WEBRTC_ICE_CONNECTION_STATE_CHECKING:
      stateStr = "checking";
      break;
    case GST_WEBRTC_ICE_CONNECTION_STATE_CONNECTED:
      stateStr = "connected";
      break;
    case GST_WEBRTC_ICE_CONNECTION_STATE_COMPLETED:
      stateStr = "completed";
      break;
    case GST_WEBRTC_ICE_CONNECTION_STATE_FAILED:
      stateStr = "FAILED";
      break;
    case GST_WEBRTC_ICE_CONNECTION_STATE_DISCONNECTED:
      stateStr = "disconnected";
      break;
    case GST_WEBRTC_ICE_CONNECTION_STATE_CLOSED:
      stateStr = "closed";
      break;
    default:
      break;
  }

  QString stateQStr = QString::fromUtf8(stateStr);
  QMetaObject::invokeMethod(
      self,
      [self, peerId, stateQStr]() {
        qDebug() << "[WebRtcPeerManager] ICE connection state →" << stateQStr << "peer:" << peerId;
        if (stateQStr == "FAILED") {
          qWarning() << "[WebRtcPeerManager] ICE FAILED for peer:" << peerId
                     << "— check STUN/TURN config and network connectivity";
        }
      },
      Qt::QueuedConnection);
}

void WebRtcPeerManager::onConnectionStateChange(GObject* webrtcbin, GParamSpec* /*pspec*/,
                                                gpointer user_data) {
  auto*         self   = static_cast<WebRtcPeerManager*>(user_data);
  Peer*         p      = self->peerByWebrtcBin(GST_ELEMENT(webrtcbin));
  const QString peerId = p ? p->id : QStringLiteral("unknown");

  GstWebRTCPeerConnectionState state = GST_WEBRTC_PEER_CONNECTION_STATE_NEW;
  g_object_get(webrtcbin, "connection-state", &state, nullptr);

  const char* stateStr = "unknown";
  switch (state) {
    case GST_WEBRTC_PEER_CONNECTION_STATE_NEW:
      stateStr = "new";
      break;
    case GST_WEBRTC_PEER_CONNECTION_STATE_CONNECTING:
      stateStr = "connecting";
      break;
    case GST_WEBRTC_PEER_CONNECTION_STATE_CONNECTED:
      stateStr = "connected";
      break;
    case GST_WEBRTC_PEER_CONNECTION_STATE_DISCONNECTED:
      stateStr = "disconnected";
      break;
    case GST_WEBRTC_PEER_CONNECTION_STATE_FAILED:
      stateStr = "FAILED";
      break;
    case GST_WEBRTC_PEER_CONNECTION_STATE_CLOSED:
      stateStr = "closed";
      break;
    default:
      break;
  }

  QString stateQStr = QString::fromUtf8(stateStr);
  QMetaObject::invokeMethod(
      self,
      [self, peerId, stateQStr]() {
        qDebug() << "[WebRtcPeerManager] DTLS/peer connection state →" << stateQStr
                 << "peer:" << peerId;
        if (stateQStr == "FAILED") {
          qWarning() << "[WebRtcPeerManager] DTLS connection FAILED for peer:" << peerId
                     << "— likely certificate or DTLS negotiation issue";
        }
      },
      Qt::QueuedConnection);
}
