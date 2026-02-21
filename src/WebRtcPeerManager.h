#pragma once

#include <gst/gst.h>
#include <gst/webrtc/webrtc.h>

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioInput>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVideoFrame>

#include "SignalingClient.h"  // for IceCandidate
#include "VideoFrameSink.h"

class VideoTile;
class QScreenCapture;
class QMediaCaptureSession;
class QVideoSink;

// Video capture source mode
enum class VideoSourceMode {
  Camera,       // autovideosrc / v4l2src
  ScreenShare,  // pipewiresrc via XDG Portal
};

class WebRtcPeerManager : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool running READ running NOTIFY runningChanged)
  Q_PROPERTY(int peerCount READ peerCount NOTIFY peerCountChanged)
  Q_PROPERTY(bool micMuted READ micMuted WRITE setMicMuted NOTIFY micMutedChanged)
  Q_PROPERTY(bool cameraMuted READ cameraMuted WRITE setCameraMuted NOTIFY cameraMutedChanged)
  Q_PROPERTY(VideoSourceMode videoSource READ videoSource NOTIFY videoSourceChanged)
  Q_PROPERTY(bool micAvailable READ micAvailable NOTIFY micAvailableChanged)

 public:
  explicit WebRtcPeerManager(QObject* parent = nullptr);
  ~WebRtcPeerManager() override;

  bool running() const {
    return m_running;
  }
  int peerCount() const {
    return m_peers.size();
  }

  bool micMuted() const {
    return m_micMuted;
  }
  Q_INVOKABLE void setMicMuted(bool m);

  bool cameraMuted() const {
    return m_cameraMuted;
  }
  Q_INVOKABLE void setCameraMuted(bool m);

  bool micAvailable() const {
    return m_micAvailable;
  }

  VideoSourceMode videoSource() const {
    return m_videoSource;
  }

  // Start with camera
  Q_INVOKABLE bool startWithCamera();
  // Start with screen share via Qt Multimedia (QScreenCapture + appsrc)
  Q_INVOKABLE bool startScreenShare();
  // Start with screen share (portal fd + node id) — kept for compatibility
  Q_INVOKABLE bool startWithScreen(int portalFd, uint videoNodeId);
  Q_INVOKABLE void stop();

  // Switch video source at runtime (rebuilds pipeline)
  Q_INVOKABLE void switchToCamera();
  Q_INVOKABLE void switchToScreen(int portalFd, uint videoNodeId);

  // Called by SignalingClient event handlers
  Q_INVOKABLE void ensurePeer(const QString& peerId);
  Q_INVOKABLE void dropPeer(const QString& peerId);

  // VideoFrameSink for a given peer (nullptr if not running)
  VideoFrameSink* sinkForPeer(const QString& peerId) const;
  VideoFrameSink* localSink() const {
    return m_localSink;
  }

  // QML bridge: connect a VideoTile (QObject*) to its VideoFrameSink by peer ID
  Q_INVOKABLE void connectVideoTile(const QString& peerId, QObject* tile);
  Q_INVOKABLE void connectLocalVideoTile(QObject* tile);

 signals:
  void runningChanged();
  void peerCountChanged();
  void micMutedChanged();
  void cameraMutedChanged();
  void videoSourceChanged();
  void micAvailableChanged();

  // Outbound signaling (connect to SignalingClient)
  void localOfferReady(QString peerId, QString sdp);
  void localAnswerReady(QString peerId, QString sdp);
  void localIceReady(QString peerId, IceCandidate ice);

  void error(QString message);
  void info(QString message);

 public slots:
  // Inbound signaling (connect from SignalingClient)
  void onRemoteOffer(const QString& peerId, const QString& sdp);
  void onRemoteAnswer(const QString& peerId, const QString& sdp);
  void onRemoteIce(const QString& peerId, const IceCandidate& ice);

 private slots:
  void onVideoFrame(const QVideoFrame& frame);
  void onAudioDataReady();

 private:
  struct Peer {
    QString         id;
    GstElement*     webrtcbin = nullptr;
    VideoFrameSink* sink      = nullptr;
  };

  bool buildPipeline();
  void teardown();
  void teardownPeer(Peer& p);

  bool addWebRtcBinForPeer(const QString& peerId);
  bool linkTeeToWebrtcBin(GstElement* tee, GstElement* webrtcbin, const QString& label);

  // sdpMid extraction from local SDP
  QString extractSdpMid(GstElement* webrtcbin, int mlineIndex) const;

  // GStreamer callbacks
  static gboolean busCallback(GstBus* bus, GstMessage* msg, gpointer user_data);
  static void     onNegotiationNeeded(GstElement* webrtcbin, gpointer user_data);
  static void     onIceCandidate(GstElement* webrtcbin, guint mline, gchar* candidate,
                                 gpointer user_data);
  static void     onOfferCreated(GstPromise* promise, gpointer user_data);
  static void     onAnswerCreated(GstPromise* promise, gpointer user_data);
  static void     onPadAdded(GstElement* webrtcbin, GstPad* pad, gpointer user_data);
  // notify::ice-connection-state / notify::connection-state — GObject property
  // notifications
  static void onIceConnectionStateChange(GObject* webrtcbin, GParamSpec* pspec, gpointer user_data);
  static void onConnectionStateChange(GObject* webrtcbin, GParamSpec* pspec, gpointer user_data);

  Peer* peerByWebrtcBin(GstElement* bin);

 private:
  bool            m_running      = false;
  bool            m_micMuted     = false;
  bool            m_cameraMuted  = false;
  bool            m_micAvailable = false;
  VideoSourceMode m_videoSource  = VideoSourceMode::Camera;

  // Portal screen share params (used when m_videoSource == ScreenShare via
  // portal)
  int  m_portalFd    = -1;
  uint m_videoNodeId = 0;

  // Qt Multimedia screen capture objects (screen share via QScreenCapture)
  QScreenCapture*       m_screenCapture  = nullptr;
  QMediaCaptureSession* m_captureSession = nullptr;
  QVideoSink*           m_videoSink      = nullptr;
  GstElement*           m_appsrc         = nullptr;  // non-owning, owned by pipeline

  // Qt Multimedia audio capture objects (microphone)
  QAudioInput* m_audioInput = nullptr;
  QAudioFormat m_audioFormat;
  QAudioDevice m_audioDevice;
  QIODevice*   m_audioIODevice = nullptr;

  GstElement* m_pipeline   = nullptr;
  GstElement* m_vtee       = nullptr;  // video tee after encode
  GstElement* m_atee       = nullptr;  // audio tee after encode
  GstElement* m_audioValve = nullptr;  // for mic mute
  GstElement* m_videoValve = nullptr;  // for camera/screen mute

  QHash<QString, Peer> m_peers;
  VideoFrameSink*      m_localSink = nullptr;  // local video preview

  // Peers that called ensurePeer before pipeline was ready
  QList<QString> m_pendingPeers;
  // ICE candidates that arrived before their peer was added
  QHash<QString, QList<IceCandidate>> m_pendingIce;
  // Remote offers that arrived before the pipeline was running (buffered for
  // later processing)
  QHash<QString, QString> m_pendingOffers;
};
