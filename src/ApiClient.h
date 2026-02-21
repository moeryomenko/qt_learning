#pragma once

#include <QDateTime>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QUrl>
#include <QVariant>

// Represents a room from the server
struct RoomInfo {
  QString id;
  QString name;
  QDateTime createdAt;
  QStringList roommates;
};

// Holds JWT tokens
struct AuthTokens {
  QString accessToken;
  QString refreshToken;
  int expiresIn = 0;
  int refreshExpiresIn = 0;
};

class ApiClient : public QObject {
  Q_OBJECT

  Q_PROPERTY(
      QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
  Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
  Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
  Q_PROPERTY(QString accessToken READ accessToken NOTIFY accessTokenChanged)

public:
  explicit ApiClient(QObject *parent = nullptr);

  QString baseUrl() const { return m_baseUrl; }
  void setBaseUrl(const QString &url);

  bool authenticated() const { return !m_tokens.accessToken.isEmpty(); }
  QString username() const { return m_username; }
  QString accessToken() const { return m_tokens.accessToken; }
  QString refreshToken() const { return m_tokens.refreshToken; }

  Q_INVOKABLE void login(const QString &username, const QString &password);
  Q_INVOKABLE void registerUser(const QString &username,
                                const QString &password);
  Q_INVOKABLE void refreshTokens();
  Q_INVOKABLE void logout();

  Q_INVOKABLE void listRooms();
  Q_INVOKABLE void createRoom(const QString &name);
  Q_INVOKABLE void getRoomParticipants(const QString &roomId);

signals:
  void baseUrlChanged();
  void authenticatedChanged();
  void usernameChanged();
  void accessTokenChanged();

  void loginSuccess(const AuthTokens &tokens);
  void loginError(const QString &message);

  void registerSuccess(const AuthTokens &tokens);
  void registerError(const QString &message);

  void refreshSuccess(const AuthTokens &tokens);
  void refreshError(const QString &message);

  // QVariantList of QVariantMap {id, name, createdAt, roommates} —
  // QML-accessible
  void roomsReceived(const QVariantList &rooms);
  void roomCreated(const QVariantMap &room);
  void apiError(const QString &message);

private:
  QNetworkRequest buildRequest(const QString &path) const;
  void setTokens(const AuthTokens &tokens, const QString &username = {});
  void handleAuthReply(QNetworkReply *reply, bool isRegister = false);
  AuthTokens parseTokens(const QJsonObject &obj) const;
  RoomInfo parseRoom(const QJsonObject &obj) const;
  QString extractError(QNetworkReply *reply) const;

private:
  QNetworkAccessManager m_nam;
  QString m_baseUrl;
  QString m_username;
  AuthTokens m_tokens;
};
