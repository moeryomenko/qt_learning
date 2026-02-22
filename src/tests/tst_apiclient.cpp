#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest/QtTest>

// -----------------------------------------------------------------------
// Unit tests for ApiClient JSON response parsing.
// We test the parsing logic in isolation (no network needed).
// -----------------------------------------------------------------------

class TstApiClient : public QObject {
  Q_OBJECT

 private:
  // Helpers that mirror ApiClient's internal parsing logic

  struct AuthTokens {
    QString accessToken, refreshToken;
    int     expiresIn = 0, refreshExpiresIn = 0;
  };

  struct RoomInfo {
    QString id, name;
  };

  static AuthTokens parseTokens(const QJsonObject& obj) {
    AuthTokens t;
    t.accessToken      = obj.value("access_token").toString();
    t.refreshToken     = obj.value("refresh_token").toString();
    t.expiresIn        = obj.value("expires_in").toInt();
    t.refreshExpiresIn = obj.value("refresh_expires_in").toInt();
    return t;
  }

  static RoomInfo parseRoom(const QJsonObject& obj) {
    RoomInfo r;
    r.id   = obj.value("id").toString();
    r.name = obj.value("name").toString();
    return r;
  }

  static QString extractError(const QByteArray& body) {
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isNull() && doc.isObject()) {
      const QString msg = doc.object().value("message").toString();
      if (!msg.isEmpty())
        return msg;
    }
    return QString::fromUtf8(body).trimmed();
  }

 private slots:

  // -----------------------------------------------------------------------
  // Case 1: Parse valid access token
  // -----------------------------------------------------------------------
  void test_parseTokens_accessToken() {
    qDebug() << Q_FUNC_INFO;
    QJsonObject obj;
    obj["access_token"]       = "eyJhbGciOiJIUzI1NiJ9.test.sig";
    obj["refresh_token"]      = "refresh-token-value";
    obj["expires_in"]         = 3600;
    obj["refresh_expires_in"] = 86400;

    const auto t = parseTokens(obj);
    QCOMPARE(t.accessToken, QString("eyJhbGciOiJIUzI1NiJ9.test.sig"));
    qDebug() << "[TstApiClient] access_token parsed OK";
  }

  // -----------------------------------------------------------------------
  // Case 2: Parse refresh token and expiry fields
  // -----------------------------------------------------------------------
  void test_parseTokens_refreshToken() {
    qDebug() << Q_FUNC_INFO;
    QJsonObject obj;
    obj["access_token"]       = "at";
    obj["refresh_token"]      = "rt-secret";
    obj["expires_in"]         = 900;
    obj["refresh_expires_in"] = 7200;

    const auto t = parseTokens(obj);
    QCOMPARE(t.refreshToken, QString("rt-secret"));
    QCOMPARE(t.expiresIn, 900);
    QCOMPARE(t.refreshExpiresIn, 7200);
    qDebug() << "[TstApiClient] refresh_token and expiry parsed OK";
  }

  // -----------------------------------------------------------------------
  // Case 3: Parse empty token object returns empty strings
  // -----------------------------------------------------------------------
  void test_parseTokens_missingFields() {
    qDebug() << Q_FUNC_INFO;
    QJsonObject obj;  // empty
    const auto  t = parseTokens(obj);
    QVERIFY(t.accessToken.isEmpty());
    QVERIFY(t.refreshToken.isEmpty());
    QCOMPARE(t.expiresIn, 0);
    qDebug() << "[TstApiClient] missing fields handled OK";
  }

  // -----------------------------------------------------------------------
  // Case 4: Parse room JSON object
  // -----------------------------------------------------------------------
  void test_parseRoom_basic() {
    qDebug() << Q_FUNC_INFO;
    QJsonObject obj;
    obj["id"]   = "room-uuid-001";
    obj["name"] = "General";

    const auto r = parseRoom(obj);
    QCOMPARE(r.id, QString("room-uuid-001"));
    QCOMPARE(r.name, QString("General"));
    qDebug() << "[TstApiClient] room parsed OK";
  }

  // -----------------------------------------------------------------------
  // Case 5: Parse JSON array of rooms
  // -----------------------------------------------------------------------
  void test_parseRoomArray() {
    qDebug() << Q_FUNC_INFO;
    QJsonArray  arr;
    QJsonObject r1;
    r1["id"]   = "id1";
    r1["name"] = "Room A";
    QJsonObject r2;
    r2["id"]   = "id2";
    r2["name"] = "Room B";
    arr << r1 << r2;

    QList<RoomInfo> rooms;
    for (const auto& v : arr)
      rooms.append(parseRoom(v.toObject()));

    QCOMPARE(rooms.size(), 2);
    QCOMPARE(rooms[0].name, QString("Room A"));
    QCOMPARE(rooms[1].id, QString("id2"));
    qDebug() << "[TstApiClient] room array parsed OK, count:" << rooms.size();
  }

  // -----------------------------------------------------------------------
  // Case 6: Parse error response body (JSON with "message" key)
  // -----------------------------------------------------------------------
  void test_extractError_jsonMessage() {
    qDebug() << Q_FUNC_INFO;
    QJsonObject obj;
    obj["message"]        = "invalid credentials";
    const QByteArray body = QJsonDocument(obj).toJson();

    const QString err = extractError(body);
    QCOMPARE(err, QString("invalid credentials"));
    qDebug() << "[TstApiClient] error JSON parsed OK:" << err;
  }
};

QTEST_MAIN(TstApiClient)
#include "tst_apiclient.moc"
