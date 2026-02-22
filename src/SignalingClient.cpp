#include "SignalingClient.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSslError>
#include <cmath>

SignalingClient::SignalingClient(QObject* parent) : QObject(parent) {
  qDebug() << Q_FUNC_INFO << "SignalingClient created";

  connect(&m_ws, &QWebSocket::connected, this, &SignalingClient::onConnected);
  connect(&m_ws, &QWebSocket::disconnected, this, &SignalingClient::onDisconnected);
  connect(&m_ws, &QWebSocket::textMessageReceived, this, &SignalingClient::onTextMessage);
  connect(&m_ws, &QWebSocket::errorOccurred, this, &SignalingClient::onError);

  // Allow self-signed certificates (dev/staging servers)
  connect(&m_ws, &QWebSocket::sslErrors, this, [this](const QList<QSslError>& errors) {
    qWarning() << Q_FUNC_INFO << "[FIX] Ignoring SSL errors for self-signed cert:";
    for (const auto& e : errors)
      qWarning() << Q_FUNC_INFO << "  " << e.errorString();
    m_ws.ignoreSslErrors(errors);
  });

  m_reconnectTimer.setSingleShot(true);
  connect(&m_reconnectTimer, &QTimer::timeout, this, &SignalingClient::onReconnectTimeout);
}

void SignalingClient::connectToServer(const QString& wsBaseUrl, const QString& token) {
  qDebug() << Q_FUNC_INFO << "Connecting to" << wsBaseUrl << "with token empty:" << token.isEmpty();
  m_serverUrl = wsBaseUrl;
  m_token     = token;
  resetReconnect();

  QString url = wsBaseUrl + "/api/v1/ws";
  if (!token.isEmpty())
    url += "?token=" + token;

  qDebug() << Q_FUNC_INFO << "Opening WebSocket:" << url;
  m_ws.open(QUrl(url));
}

void SignalingClient::disconnectFromServer() {
  qDebug() << Q_FUNC_INFO << "Disconnecting from server";
  m_reconnectTimer.stop();
  resetReconnect();
  m_pendingRoom.clear();
  m_pendingUsername.clear();
  m_ws.close();
}

void SignalingClient::join(const QString& roomId, const QString& username) {
  qDebug() << Q_FUNC_INFO << "Joining room:" << roomId << "as:" << username;
  m_pendingRoom     = roomId;
  m_pendingUsername = username;

  if (!m_connected) {
    qWarning() << Q_FUNC_INFO << "Not connected — join will be sent after connection";
    return;
  }

  QJsonObject msg;
  msg["type"]     = "join";
  msg["room"]     = roomId;
  msg["username"] = username;
  sendJson(msg);
}

void SignalingClient::leave() {
  qDebug() << Q_FUNC_INFO << "Leaving room:" << m_currentRoom;
  if (m_currentRoom.isEmpty()) {
    qWarning() << Q_FUNC_INFO << "Not in a room, ignoring leave";
    return;
  }
  QJsonObject msg;
  msg["type"] = "leave";
  msg["room"] = m_currentRoom;
  sendJson(msg);

  m_currentRoom.clear();
  m_selfId.clear();
  m_peers.clear();
  m_peerNames.clear();
  emit currentRoomChanged();
  emit selfIdChanged();
  emit peersChanged();
}

void SignalingClient::sendOffer(const QString& toPeerId, const QString& sdp) {
  qDebug() << Q_FUNC_INFO << "Sending offer to peer:" << toPeerId << "sdp length:" << sdp.length();
  QJsonObject payload;
  payload["sdp"]  = sdp;
  payload["type"] = "offer";

  QJsonObject msg;
  msg["type"]    = "offer";
  msg["to"]      = toPeerId;
  msg["payload"] = payload;
  sendJson(msg);
}

void SignalingClient::sendAnswer(const QString& toPeerId, const QString& sdp) {
  qDebug() << Q_FUNC_INFO << "Sending answer to peer:" << toPeerId << "sdp length:" << sdp.length();
  QJsonObject payload;
  payload["sdp"]  = sdp;
  payload["type"] = "answer";

  QJsonObject msg;
  msg["type"]    = "answer";
  msg["to"]      = toPeerId;
  msg["payload"] = payload;
  sendJson(msg);
}

void SignalingClient::sendIce(const QString& toPeerId, const IceCandidate& ice) {
  qDebug() << Q_FUNC_INFO << "Sending ICE to peer:" << toPeerId << "sdpMid:" << ice.sdpMid
           << "mline:" << ice.sdpMLineIndex;
  QJsonObject payload;
  payload["candidate"]     = ice.candidate;
  payload["sdpMid"]        = ice.sdpMid;
  payload["sdpMLineIndex"] = ice.sdpMLineIndex;

  QJsonObject msg;
  msg["type"]    = "ice";
  msg["to"]      = toPeerId;
  msg["payload"] = payload;
  sendJson(msg);
}

// --- Private slots ---

void SignalingClient::onConnected() {
  qDebug() << Q_FUNC_INFO << "WebSocket connected";
  m_connected         = true;
  m_reconnectAttempts = 0;
  emit connectedChanged();

  // Rejoin room if we had one pending
  if (!m_pendingRoom.isEmpty()) {
    qDebug() << Q_FUNC_INFO << "Auto-joining pending room:" << m_pendingRoom;
    join(m_pendingRoom, m_pendingUsername);
  }
}

void SignalingClient::onDisconnected() {
  qDebug() << Q_FUNC_INFO << "WebSocket disconnected. Was in room:" << m_currentRoom;
  m_connected = false;
  emit connectedChanged();

  if (!m_currentRoom.isEmpty()) {
    m_pendingRoom     = m_currentRoom;
    m_pendingUsername = m_peerNames.value(m_selfId);
    m_currentRoom.clear();
    emit currentRoomChanged();
  }

  // Clear peer state
  m_selfId.clear();
  m_peers.clear();
  m_peerNames.clear();
  emit selfIdChanged();
  emit peersChanged();

  scheduleReconnect();
}

void SignalingClient::onTextMessage(const QString& msg) {
  qDebug() << Q_FUNC_INFO << "Received message, length:" << msg.length();
  QJsonParseError     pe;
  const QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &pe);
  if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
    qWarning() << Q_FUNC_INFO << "Failed to parse message:" << pe.errorString()
               << "raw:" << msg.left(200);
    return;
  }
  handleMessage(doc.object());
}

void SignalingClient::onError(QAbstractSocket::SocketError err) {
  const QString errStr = m_ws.errorString();
  qWarning() << Q_FUNC_INFO << "WebSocket error:" << err << errStr;
  setError(errStr);
}

void SignalingClient::onReconnectTimeout() {
  if (m_reconnectAttempts >= kMaxReconnectAttempts) {
    qWarning() << Q_FUNC_INFO << "Max reconnect attempts reached, giving up";
    setError("Connection lost after " + QString::number(kMaxReconnectAttempts) + " retries");
    return;
  }

  qDebug() << Q_FUNC_INFO << "Reconnect attempt" << (m_reconnectAttempts + 1) << "/"
           << kMaxReconnectAttempts;

  QString url = m_serverUrl + "/api/v1/ws";
  if (!m_token.isEmpty())
    url += "?token=" + m_token;
  m_ws.open(QUrl(url));
}

// --- Private helpers ---

void SignalingClient::setError(const QString& msg) {
  qWarning() << Q_FUNC_INFO << "Error:" << msg;
  m_lastError = msg;
  emit lastErrorChanged();
}

void SignalingClient::sendJson(const QJsonObject& obj) {
  if (!m_connected) {
    qWarning() << Q_FUNC_INFO << "Cannot send, not connected. type:" << obj["type"].toString();
    return;
  }
  const QString text = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
  qDebug() << Q_FUNC_INFO << "Sending:" << text.left(200);
  m_ws.sendTextMessage(text);
}

void SignalingClient::scheduleReconnect() {
  if (m_serverUrl.isEmpty())
    return;
  if (m_reconnectAttempts >= kMaxReconnectAttempts)
    return;

  // Exponential backoff: 1s, 2s, 4s, 8s, 16s (capped at 30s)
  const int delayMs =
      qMin(kBaseReconnectMs * static_cast<int>(std::pow(2, m_reconnectAttempts)), 30000);
  m_reconnectAttempts++;
  qDebug() << Q_FUNC_INFO << "Scheduling reconnect in" << delayMs << "ms"
           << "(attempt" << m_reconnectAttempts << ")";
  m_reconnectTimer.start(delayMs);
}

void SignalingClient::resetReconnect() {
  m_reconnectAttempts = 0;
  m_reconnectTimer.stop();
}

void SignalingClient::handleMessage(const QJsonObject& obj) {
  const QString     type     = obj.value("type").toString();
  const QString     from     = obj.value("from").toString();
  const QString     username = obj.value("username").toString();
  const QJsonObject payload  = obj.value("payload").toObject();

  qDebug() << Q_FUNC_INFO << "Handling message type:" << type << "from:" << from;

  if (type == "joined") {
    // Server assigned us our clientId via `from`
    m_selfId = from;
    emit selfIdChanged();
    qDebug() << Q_FUNC_INFO << "Joined room. selfId:" << m_selfId;

    // Parse room_mates map: {clientId: username, ...}
    const QJsonObject roomMates = payload.value("room_mates").toObject();
    m_peers.clear();
    m_peerNames.clear();
    for (auto it = roomMates.begin(); it != roomMates.end(); ++it) {
      const QString peerId   = it.key();
      const QString peerName = it.value().toString();
      m_peers << peerId;
      m_peerNames[peerId] = peerName;
      qDebug() << Q_FUNC_INFO << "Existing peer:" << peerId << "name:" << peerName;
    }
    emit peersChanged();

    if (m_currentRoom != m_pendingRoom) {
      m_currentRoom = m_pendingRoom;
      emit currentRoomChanged();
    }
    return;
  }

  if (type == "peer-joined") {
    qDebug() << Q_FUNC_INFO << "Peer joined:" << from << "username:" << username;
    if (!from.isEmpty() && !m_peers.contains(from)) {
      m_peers << from;
      m_peerNames[from] = username;
      emit peersChanged();
      emit peerJoined(from, username);
    }
    return;
  }

  if (type == "leave") {
    qDebug() << Q_FUNC_INFO << "Peer left:" << from;
    m_peers.removeAll(from);
    m_peerNames.remove(from);
    emit peersChanged();
    emit peerLeft(from);
    return;
  }

  if (type == "offer") {
    const QString sdp = payload.value("sdp").toString();
    qDebug() << Q_FUNC_INFO << "Received offer from:" << from << "sdp length:" << sdp.length();
    emit remoteOffer(from, sdp);
    return;
  }

  if (type == "answer") {
    const QString sdp = payload.value("sdp").toString();
    qDebug() << Q_FUNC_INFO << "Received answer from:" << from << "sdp length:" << sdp.length();
    emit remoteAnswer(from, sdp);
    return;
  }

  if (type == "ice") {
    IceCandidate ice;
    ice.candidate     = payload.value("candidate").toString();
    ice.sdpMid        = payload.value("sdpMid").toString();
    ice.sdpMLineIndex = payload.value("sdpMLineIndex").toInt();
    qDebug() << Q_FUNC_INFO << "Received ICE from:" << from << "sdpMid:" << ice.sdpMid
             << "mline:" << ice.sdpMLineIndex;
    emit remoteIce(from, ice);
    return;
  }

  qWarning() << Q_FUNC_INFO << "Unknown message type:" << type;
}
