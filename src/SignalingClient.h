#pragma once
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>

// ICE candidate with full info needed by Go server
struct IceCandidate {
  QString candidate;
  QString sdpMid;
  int sdpMLineIndex = 0;
};

class SignalingClient : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
  Q_PROPERTY(QString selfId READ selfId NOTIFY selfIdChanged)
  Q_PROPERTY(QStringList peers READ peers NOTIFY peersChanged)
  Q_PROPERTY(QString currentRoom READ currentRoom NOTIFY currentRoomChanged)
  Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
  explicit SignalingClient(QObject *parent = nullptr);

  bool connected() const { return m_connected; }
  QString selfId() const { return m_selfId; }
  QStringList peers() const { return m_peers; }
  QString currentRoom() const { return m_currentRoom; }
  QString lastError() const { return m_lastError; }

  // Map of peerId -> username for currently connected peers
  QMap<QString, QString> peerNames() const { return m_peerNames; }
  Q_INVOKABLE QString peerName(const QString &peerId) const {
    return m_peerNames.value(peerId, peerId);
  }

  Q_INVOKABLE void connectToServer(const QString &wsBaseUrl,
                                   const QString &token);
  Q_INVOKABLE void disconnectFromServer();

  Q_INVOKABLE void join(const QString &roomId, const QString &username);
  Q_INVOKABLE void leave();

  Q_INVOKABLE void sendOffer(const QString &toPeerId, const QString &sdp);
  Q_INVOKABLE void sendAnswer(const QString &toPeerId, const QString &sdp);
  Q_INVOKABLE void sendIce(const QString &toPeerId, const IceCandidate &ice);

signals:
  void connectedChanged();
  void selfIdChanged();
  void peersChanged();
  void currentRoomChanged();
  void lastErrorChanged();

  void peerJoined(QString peerId, QString username);
  void peerLeft(QString peerId);

  void remoteOffer(QString fromPeerId, QString sdp);
  void remoteAnswer(QString fromPeerId, QString sdp);
  void remoteIce(QString fromPeerId, IceCandidate ice);

private slots:
  void onConnected();
  void onDisconnected();
  void onTextMessage(const QString &msg);
  void onError(QAbstractSocket::SocketError err);
  void onReconnectTimeout();

private:
  void setError(const QString &msg);
  void sendJson(const QJsonObject &obj);
  void handleMessage(const QJsonObject &obj);
  void scheduleReconnect();
  void resetReconnect();

private:
  QWebSocket m_ws;
  QTimer m_reconnectTimer;

  QString m_serverUrl; // without token
  QString m_token;
  QString m_pendingRoom;
  QString m_pendingUsername;

  bool m_connected = false;
  QString m_selfId;
  QStringList m_peers;
  QMap<QString, QString> m_peerNames; // peerId -> username
  QString m_currentRoom;
  QString m_lastError;

  int m_reconnectAttempts = 0;
  static constexpr int kMaxReconnectAttempts = 5;
  static constexpr int kBaseReconnectMs = 1000;
};
