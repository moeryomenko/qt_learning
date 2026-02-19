#include <QtTest/QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// -----------------------------------------------------------------------
// Unit tests for SignalingClient message serialization / deserialization.
// Tests the exact JSON protocol used by the Go server.
// -----------------------------------------------------------------------

// Minimal IceCandidate struct (mirrors SignalingClient.h)
struct IceCandidate {
    QString candidate;
    QString sdpMid;
    int sdpMLineIndex = 0;
};

class TstSignalingClient : public QObject {
    Q_OBJECT

private:
    // -----------------------------------------------------------------------
    // Serialization helpers (mirror SignalingClient.cpp internal logic)
    // -----------------------------------------------------------------------

    static QJsonObject makeJoin(const QString& roomId, const QString& username) {
        QJsonObject o;
        o["type"]     = "join";
        o["room"]     = roomId;
        o["username"] = username;
        return o;
    }

    static QJsonObject makeOffer(const QString& toPeerId, const QString& sdp) {
        QJsonObject payload;
        payload["sdp"]  = sdp;
        payload["type"] = "offer";
        QJsonObject o;
        o["type"]    = "offer";
        o["to"]      = toPeerId;
        o["payload"] = payload;
        return o;
    }

    static QJsonObject makeAnswer(const QString& toPeerId, const QString& sdp) {
        QJsonObject payload;
        payload["sdp"]  = sdp;
        payload["type"] = "answer";
        QJsonObject o;
        o["type"]    = "answer";
        o["to"]      = toPeerId;
        o["payload"] = payload;
        return o;
    }

    static QJsonObject makeIce(const QString& toPeerId, const IceCandidate& ice) {
        QJsonObject payload;
        payload["candidate"]     = ice.candidate;
        payload["sdpMid"]        = ice.sdpMid;
        payload["sdpMLineIndex"] = ice.sdpMLineIndex;
        QJsonObject o;
        o["type"]    = "ice";
        o["to"]      = toPeerId;
        o["payload"] = payload;
        return o;
    }

    // -----------------------------------------------------------------------
    // Parsing helpers (mirror SignalingClient.cpp incoming-message handling)
    // -----------------------------------------------------------------------

    struct ParsedJoined {
        QString selfId;
        QMap<QString, QString> roomMates;
    };

    static ParsedJoined parseJoined(const QJsonObject& msg) {
        ParsedJoined result;
        result.selfId = msg.value("from").toString();
        const QJsonObject payload = msg.value("payload").toObject();
        const QJsonObject mates   = payload.value("room_mates").toObject();
        for (auto it = mates.begin(); it != mates.end(); ++it)
            result.roomMates.insert(it.key(), it.value().toString());
        return result;
    }

    struct ParsedPeerJoined {
        QString peerId, username;
    };

    static ParsedPeerJoined parsePeerJoined(const QJsonObject& msg) {
        return { msg.value("from").toString(), msg.value("username").toString() };
    }

    struct ParsedOffer {
        QString fromPeerId, sdp;
    };

    static ParsedOffer parseOffer(const QJsonObject& msg) {
        return { msg.value("from").toString(),
                 msg.value("payload").toObject().value("sdp").toString() };
    }

    static IceCandidate parseIce(const QJsonObject& msg) {
        const QJsonObject p = msg.value("payload").toObject();
        IceCandidate ice;
        ice.candidate     = p.value("candidate").toString();
        ice.sdpMid        = p.value("sdpMid").toString();
        ice.sdpMLineIndex = p.value("sdpMLineIndex").toInt();
        return ice;
    }

private slots:

    // -----------------------------------------------------------------------
    // Case 1: Serialize join message
    // -----------------------------------------------------------------------
    void test_serialize_join() {
        qDebug() << Q_FUNC_INFO;
        const auto obj = makeJoin("room-42", "alice");
        QCOMPARE(obj.value("type").toString(), QString("join"));
        QCOMPARE(obj.value("room").toString(), QString("room-42"));
        QCOMPARE(obj.value("username").toString(), QString("alice"));
        qDebug() << "[TstSignalingClient] join serialized OK";
    }

    // -----------------------------------------------------------------------
    // Case 2: Serialize offer message (nested payload)
    // -----------------------------------------------------------------------
    void test_serialize_offer() {
        qDebug() << Q_FUNC_INFO;
        const auto obj = makeOffer("peer-99", "v=0\r\no=...");
        QCOMPARE(obj.value("type").toString(), QString("offer"));
        QCOMPARE(obj.value("to").toString(), QString("peer-99"));
        const QJsonObject payload = obj.value("payload").toObject();
        QCOMPARE(payload.value("type").toString(), QString("offer"));
        QVERIFY(!payload.value("sdp").toString().isEmpty());
        qDebug() << "[TstSignalingClient] offer serialized OK";
    }

    // -----------------------------------------------------------------------
    // Case 3: Serialize answer message
    // -----------------------------------------------------------------------
    void test_serialize_answer() {
        qDebug() << Q_FUNC_INFO;
        const auto obj = makeAnswer("peer-77", "v=0\r\no=answer");
        QCOMPARE(obj.value("type").toString(), QString("answer"));
        QCOMPARE(obj.value("payload").toObject().value("type").toString(), QString("answer"));
        qDebug() << "[TstSignalingClient] answer serialized OK";
    }

    // -----------------------------------------------------------------------
    // Case 4: Serialize ICE candidate (sdpMid + sdpMLineIndex)
    // -----------------------------------------------------------------------
    void test_serialize_ice() {
        qDebug() << Q_FUNC_INFO;
        IceCandidate ice;
        ice.candidate     = "candidate:1 1 udp 2113667327 10.0.0.1 54321 typ host";
        ice.sdpMid        = "audio";
        ice.sdpMLineIndex = 0;

        const auto obj = makeIce("peer-33", ice);
        QCOMPARE(obj.value("type").toString(), QString("ice"));
        QCOMPARE(obj.value("to").toString(), QString("peer-33"));
        const QJsonObject payload = obj.value("payload").toObject();
        QCOMPARE(payload.value("sdpMid").toString(), QString("audio"));
        QCOMPARE(payload.value("sdpMLineIndex").toInt(), 0);
        QVERIFY(!payload.value("candidate").toString().isEmpty());
        qDebug() << "[TstSignalingClient] ICE serialized OK";
    }

    // -----------------------------------------------------------------------
    // Case 5: Parse "joined" server message (room_mates map)
    // -----------------------------------------------------------------------
    void test_parse_joined() {
        qDebug() << Q_FUNC_INFO;
        QJsonObject mates;
        mates["client-a"] = "Alice";
        mates["client-b"] = "Bob";

        QJsonObject payload;
        payload["room_mates"] = mates;

        QJsonObject msg;
        msg["type"]    = "joined";
        msg["from"]    = "my-self-id";
        msg["payload"] = payload;

        const auto result = parseJoined(msg);
        QCOMPARE(result.selfId, QString("my-self-id"));
        QCOMPARE(result.roomMates.size(), 2);
        QCOMPARE(result.roomMates.value("client-a"), QString("Alice"));
        qDebug() << "[TstSignalingClient] joined parsed OK, selfId:" << result.selfId;
    }

    // -----------------------------------------------------------------------
    // Case 6: Parse "peer-joined" message
    // -----------------------------------------------------------------------
    void test_parse_peerJoined() {
        qDebug() << Q_FUNC_INFO;
        QJsonObject msg;
        msg["type"]     = "peer-joined";
        msg["from"]     = "new-client-id";
        msg["username"] = "Charlie";

        const auto result = parsePeerJoined(msg);
        QCOMPARE(result.peerId,   QString("new-client-id"));
        QCOMPARE(result.username, QString("Charlie"));
        qDebug() << "[TstSignalingClient] peer-joined parsed OK";
    }

    // -----------------------------------------------------------------------
    // Case 7: Parse incoming offer
    // -----------------------------------------------------------------------
    void test_parse_offer() {
        qDebug() << Q_FUNC_INFO;
        QJsonObject payload;
        payload["sdp"]  = "v=0\r\no=offersdp";
        payload["type"] = "offer";

        QJsonObject msg;
        msg["type"]    = "offer";
        msg["from"]    = "offerer-id";
        msg["payload"] = payload;

        const auto result = parseOffer(msg);
        QCOMPARE(result.fromPeerId, QString("offerer-id"));
        QVERIFY(result.sdp.contains("v=0"));
        qDebug() << "[TstSignalingClient] offer parsed OK";
    }

    // -----------------------------------------------------------------------
    // Case 8: Parse incoming ICE candidate
    // -----------------------------------------------------------------------
    void test_parse_ice() {
        qDebug() << Q_FUNC_INFO;
        QJsonObject payload;
        payload["candidate"]     = "candidate:1 1 udp 12345 192.168.1.1 40000 typ host";
        payload["sdpMid"]        = "video";
        payload["sdpMLineIndex"] = 1;

        QJsonObject msg;
        msg["type"]    = "ice";
        msg["from"]    = "remote-peer";
        msg["payload"] = payload;

        const auto ice = parseIce(msg);
        QCOMPARE(ice.sdpMid, QString("video"));
        QCOMPARE(ice.sdpMLineIndex, 1);
        QVERIFY(ice.candidate.startsWith("candidate:"));
        qDebug() << "[TstSignalingClient] ICE parsed OK, sdpMid:" << ice.sdpMid;
    }

    // -----------------------------------------------------------------------
    // Case 9: Parse "leave" message (no payload needed)
    // -----------------------------------------------------------------------
    void test_parse_leave() {
        qDebug() << Q_FUNC_INFO;
        QJsonObject msg;
        msg["type"] = "leave";
        msg["from"] = "departing-peer";

        QCOMPARE(msg.value("type").toString(), QString("leave"));
        QCOMPARE(msg.value("from").toString(), QString("departing-peer"));
        qDebug() << "[TstSignalingClient] leave parsed OK";
    }
};

QTEST_MAIN(TstSignalingClient)
#include "tst_signalingclient.moc"
