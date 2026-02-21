#include "ApiClient.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>

ApiClient::ApiClient(QObject *parent)
    : QObject(parent), m_baseUrl("https://213.171.25.246") {
  qDebug() << Q_FUNC_INFO << "ApiClient created, base URL:" << m_baseUrl;

  // Allow self-signed certificates (dev/staging servers)
  connect(&m_nam, &QNetworkAccessManager::sslErrors, this,
          [](QNetworkReply *reply, const QList<QSslError> &errors) {
            qWarning() << Q_FUNC_INFO
                       << "[FIX] Ignoring SSL errors for self-signed cert:";
            for (const auto &e : errors)
              qWarning() << Q_FUNC_INFO << "  " << e.errorString();
            reply->ignoreSslErrors(errors);
          });
}

void ApiClient::setBaseUrl(const QString &url) {
  if (m_baseUrl == url)
    return;
  m_baseUrl = url;
  qDebug() << Q_FUNC_INFO << "Base URL changed to:" << url;
  emit baseUrlChanged();
}

QNetworkRequest ApiClient::buildRequest(const QString &path) const {
  QUrl url(m_baseUrl + path);
  QNetworkRequest req(url);
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!m_tokens.accessToken.isEmpty()) {
    req.setRawHeader("Authorization",
                     ("Bearer " + m_tokens.accessToken).toUtf8());
    qDebug() << Q_FUNC_INFO << "Request with auth to:" << url.toString();
  } else {
    qDebug() << Q_FUNC_INFO << "Request without auth to:" << url.toString();
  }
  return req;
}

void ApiClient::setTokens(const AuthTokens &tokens, const QString &username) {
  const bool wasAuth = authenticated();
  m_tokens = tokens;
  qDebug() << Q_FUNC_INFO
           << "Tokens set. accessToken empty:" << tokens.accessToken.isEmpty()
           << "expiresIn:" << tokens.expiresIn;
  if (!username.isEmpty()) {
    m_username = username;
    emit usernameChanged();
    qDebug() << Q_FUNC_INFO << "Username set to:" << m_username;
  }
  emit accessTokenChanged();
  if (wasAuth != authenticated()) {
    emit authenticatedChanged();
  }
}

AuthTokens ApiClient::parseTokens(const QJsonObject &obj) const {
  AuthTokens t;
  t.accessToken = obj.value("access_token").toString();
  t.refreshToken = obj.value("refresh_token").toString();
  t.expiresIn = obj.value("expires_in").toInt();
  t.refreshExpiresIn = obj.value("refresh_expires_in").toInt();
  qDebug() << Q_FUNC_INFO
           << "Parsed tokens, accessToken empty:" << t.accessToken.isEmpty()
           << "expiresIn:" << t.expiresIn;
  return t;
}

RoomInfo ApiClient::parseRoom(const QJsonObject &obj) const {
  RoomInfo r;
  r.id = obj.value("id").toString();
  r.name = obj.value("name").toString();
  r.createdAt =
      QDateTime::fromString(obj.value("created_at").toString(), Qt::ISODate);
  const QJsonArray mates = obj.value("roommates").toArray();
  for (const auto &v : mates)
    r.roommates << v.toString();
  qDebug() << Q_FUNC_INFO << "Parsed room id:" << r.id << "name:" << r.name
           << "roommates:" << r.roommates.size();
  return r;
}

QString ApiClient::extractError(QNetworkReply *reply) const {
  const QByteArray data = reply->readAll();
  qDebug() << Q_FUNC_INFO << "HTTP error:" << reply->error() << "status:"
           << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
           << "body:" << data;
  QJsonParseError pe;
  const QJsonDocument doc = QJsonDocument::fromJson(data, &pe);
  if (pe.error == QJsonParseError::NoError && doc.isObject()) {
    return doc.object().value("message").toString();
  }
  return reply->errorString();
}

// --- Auth ---

void ApiClient::login(const QString &username, const QString &password) {
  qDebug() << Q_FUNC_INFO << "Logging in as:" << username;
  QJsonObject body;
  body["username"] = username;
  body["password"] = password;

  QNetworkReply *reply =
      m_nam.post(buildRequest("/api/v1/auth/login"),
                 QJsonDocument(body).toJson(QJsonDocument::Compact));

  connect(reply, &QNetworkReply::finished, this, [this, reply, username]() {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      const QString msg = extractError(reply);
      qWarning() << Q_FUNC_INFO << "Login failed:" << msg;
      emit loginError(msg);
      return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
      qWarning() << Q_FUNC_INFO << "Login response is not a JSON object";
      emit loginError("Invalid server response");
      return;
    }
    const AuthTokens tokens = parseTokens(doc.object());
    setTokens(tokens, username);
    qDebug() << Q_FUNC_INFO << "Login success for:" << username;
    emit loginSuccess(tokens);
  });
}

void ApiClient::registerUser(const QString &username, const QString &password) {
  qDebug() << Q_FUNC_INFO << "Registering user:" << username;
  QJsonObject body;
  body["username"] = username;
  body["password"] = password;

  QNetworkReply *reply =
      m_nam.post(buildRequest("/api/v1/auth/register"),
                 QJsonDocument(body).toJson(QJsonDocument::Compact));

  connect(reply, &QNetworkReply::finished, this, [this, reply, username]() {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      const QString msg = extractError(reply);
      qWarning() << Q_FUNC_INFO << "Register failed:" << msg;
      emit registerError(msg);
      return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
      qWarning() << Q_FUNC_INFO << "Register response is not a JSON object";
      emit registerError("Invalid server response");
      return;
    }
    const AuthTokens tokens = parseTokens(doc.object());
    setTokens(tokens, username);
    qDebug() << Q_FUNC_INFO << "Register success for:" << username;
    emit registerSuccess(tokens);
  });
}

void ApiClient::refreshTokens() {
  qDebug() << Q_FUNC_INFO << "Refreshing tokens";
  if (m_tokens.refreshToken.isEmpty()) {
    qWarning() << Q_FUNC_INFO << "No refresh token available";
    emit refreshError("No refresh token");
    return;
  }
  QJsonObject body;
  body["refresh_token"] = m_tokens.refreshToken;

  QNetworkReply *reply =
      m_nam.post(buildRequest("/api/v1/auth/refresh"),
                 QJsonDocument(body).toJson(QJsonDocument::Compact));

  connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      const QString msg = extractError(reply);
      qWarning() << Q_FUNC_INFO << "Token refresh failed:" << msg;
      emit refreshError(msg);
      return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
      qWarning() << Q_FUNC_INFO << "Refresh response is not a JSON object";
      emit refreshError("Invalid server response");
      return;
    }
    const AuthTokens tokens = parseTokens(doc.object());
    setTokens(tokens);
    qDebug() << Q_FUNC_INFO << "Token refresh success";
    emit refreshSuccess(tokens);
  });
}

void ApiClient::logout() {
  qDebug() << Q_FUNC_INFO << "Logging out user:" << m_username;
  if (authenticated()) {
    QNetworkReply *reply =
        m_nam.post(buildRequest("/api/v1/auth/logout"), QByteArray{});
    connect(reply, &QNetworkReply::finished, reply,
            &QNetworkReply::deleteLater);
  }
  setTokens({});
  m_username.clear();
  emit usernameChanged();
}

// --- Rooms ---

void ApiClient::listRooms() {
  qDebug() << Q_FUNC_INFO << "Listing rooms";
  QNetworkReply *reply = m_nam.get(buildRequest("/api/v1/rooms"));

  connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      const QString msg = extractError(reply);
      qWarning() << Q_FUNC_INFO << "List rooms failed:" << msg;
      emit apiError(msg);
      return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isArray()) {
      qWarning() << Q_FUNC_INFO << "List rooms response is not a JSON array";
      emit apiError("Invalid server response");
      return;
    }
    QVariantList varRooms;
    for (const auto &v : doc.array()) {
      if (v.isObject()) {
        const RoomInfo r = parseRoom(v.toObject());
        QVariantMap m;
        m["id"] = r.id;
        m["name"] = r.name;
        m["createdAt"] = r.createdAt;
        m["roommates"] = QVariant::fromValue(r.roommates);
        varRooms << m;
      }
    }
    qDebug() << Q_FUNC_INFO << "[FIX] Emitting" << varRooms.size()
             << "rooms as QVariantList";
    emit roomsReceived(varRooms);
  });
}

void ApiClient::createRoom(const QString &name) {
  qDebug() << Q_FUNC_INFO << "Creating room:" << name;
  QJsonObject body;
  body["name"] = name;

  QNetworkReply *reply =
      m_nam.post(buildRequest("/api/v1/rooms"),
                 QJsonDocument(body).toJson(QJsonDocument::Compact));

  connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      const QString msg = extractError(reply);
      qWarning() << Q_FUNC_INFO << "Create room failed:" << msg;
      emit apiError(msg);
      return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
      qWarning() << Q_FUNC_INFO << "Create room response is not a JSON object";
      emit apiError("Invalid server response");
      return;
    }
    const RoomInfo room = parseRoom(doc.object());
    QVariantMap varRoom;
    varRoom["id"] = room.id;
    varRoom["name"] = room.name;
    varRoom["createdAt"] = room.createdAt;
    varRoom["roommates"] = QVariant::fromValue(room.roommates);
    qDebug() << Q_FUNC_INFO
             << "[FIX] Emitting created room as QVariantMap id:" << room.id
             << "name:" << room.name;
    emit roomCreated(varRoom);
  });
}

void ApiClient::getRoomParticipants(const QString &roomId) {
  qDebug() << Q_FUNC_INFO << "Getting participants for room:" << roomId;
  QNetworkReply *reply =
      m_nam.get(buildRequest("/api/v1/rooms/" + roomId + "/participants"));

  connect(reply, &QNetworkReply::finished, this, [this, reply, roomId]() {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      qWarning() << Q_FUNC_INFO << "Get participants failed for room:" << roomId
                 << extractError(reply);
      emit apiError("Failed to get participants");
      return;
    }
    qDebug() << Q_FUNC_INFO << "Participants fetched for room:" << roomId;
    // Parsed by caller if needed; emit raw for now
  });
}
