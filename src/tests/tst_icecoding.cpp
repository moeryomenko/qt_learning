#include <QtTest/QtTest>
#include <QJsonDocument>
#include <QJsonObject>

// -----------------------------------------------------------------------
// Unit tests for ICE candidate encoding/decoding format.
// Verifies that sdpMid (string) and sdpMLineIndex (int) are correctly
// serialized and round-tripped through the Go server protocol.
// -----------------------------------------------------------------------

struct IceCandidate {
    QString candidate;
    QString sdpMid;
    int sdpMLineIndex = 0;
};

static QJsonObject encodeIce(const QString& toPeerId, const IceCandidate& ice) {
    QJsonObject payload;
    payload["candidate"]     = ice.candidate;
    payload["sdpMid"]        = ice.sdpMid;
    payload["sdpMLineIndex"] = ice.sdpMLineIndex;

    QJsonObject msg;
    msg["type"]    = "ice";
    msg["to"]      = toPeerId;
    msg["payload"] = payload;
    return msg;
}

static IceCandidate decodeIce(const QJsonObject& msg) {
    const QJsonObject p = msg.value("payload").toObject();
    IceCandidate ice;
    ice.candidate     = p.value("candidate").toString();
    ice.sdpMid        = p.value("sdpMid").toString();
    ice.sdpMLineIndex = p.value("sdpMLineIndex").toInt();
    return ice;
}

class TstIceCoding : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // Case 1: sdpMid field is present and is a string
    // -----------------------------------------------------------------------
    void test_sdpMid_isString() {
        qDebug() << Q_FUNC_INFO;
        IceCandidate ice;
        ice.candidate     = "candidate:1 1 udp 2 10.0.0.1 9 typ host";
        ice.sdpMid        = "audio";
        ice.sdpMLineIndex = 0;

        const QJsonObject msg = encodeIce("peer-a", ice);
        const QJsonValue midVal = msg["payload"].toObject()["sdpMid"];
        QVERIFY(midVal.isString());
        qDebug() << "[TstIceCoding] sdpMid is string OK";
    }

    // -----------------------------------------------------------------------
    // Case 2: sdpMLineIndex field is present and is an integer
    // -----------------------------------------------------------------------
    void test_sdpMLineIndex_isInt() {
        qDebug() << Q_FUNC_INFO;
        IceCandidate ice;
        ice.candidate     = "candidate:1 1 udp 2 10.0.0.1 9 typ host";
        ice.sdpMid        = "video";
        ice.sdpMLineIndex = 1;

        const QJsonObject msg = encodeIce("peer-b", ice);
        const QJsonValue idxVal = msg["payload"].toObject()["sdpMLineIndex"];
        QVERIFY(idxVal.isDouble());  // JSON numbers are doubles in Qt
        QCOMPARE((int)idxVal.toDouble(), 1);
        qDebug() << "[TstIceCoding] sdpMLineIndex is int OK";
    }

    // -----------------------------------------------------------------------
    // Case 3: Round-trip audio ICE (encode -> decode -> compare)
    // -----------------------------------------------------------------------
    void test_roundtrip_audio() {
        qDebug() << Q_FUNC_INFO;
        IceCandidate orig;
        orig.candidate     = "candidate:1 1 udp 2130706431 192.168.0.10 54321 typ host";
        orig.sdpMid        = "audio";
        orig.sdpMLineIndex = 0;

        const QJsonObject msg = encodeIce("remote-1", orig);
        const IceCandidate decoded = decodeIce(msg);

        QCOMPARE(decoded.candidate,     orig.candidate);
        QCOMPARE(decoded.sdpMid,        QString("audio"));
        QCOMPARE(decoded.sdpMLineIndex, 0);
        qDebug() << "[TstIceCoding] audio ICE round-trip OK";
    }

    // -----------------------------------------------------------------------
    // Case 4: Round-trip video ICE (mlineIndex = 1)
    // -----------------------------------------------------------------------
    void test_roundtrip_video() {
        qDebug() << Q_FUNC_INFO;
        IceCandidate orig;
        orig.candidate     = "candidate:3 1 tcp 1518214911 172.17.0.2 9 typ host tcptype active";
        orig.sdpMid        = "video";
        orig.sdpMLineIndex = 1;

        const QJsonObject msg = encodeIce("remote-2", orig);
        const IceCandidate decoded = decodeIce(msg);

        QCOMPARE(decoded.sdpMid,        QString("video"));
        QCOMPARE(decoded.sdpMLineIndex, 1);
        qDebug() << "[TstIceCoding] video ICE round-trip OK";
    }

    // -----------------------------------------------------------------------
    // Case 5: JSON wire format round-trip via QJsonDocument
    // -----------------------------------------------------------------------
    void test_jsonDocument_roundtrip() {
        qDebug() << Q_FUNC_INFO;
        IceCandidate orig;
        orig.candidate     = "candidate:1 1 udp 1 192.168.1.1 40000 typ host";
        orig.sdpMid        = "audio";
        orig.sdpMLineIndex = 0;

        const QJsonObject msg      = encodeIce("peer-x", orig);
        const QByteArray bytes     = QJsonDocument(msg).toJson(QJsonDocument::Compact);
        const QJsonObject restored = QJsonDocument::fromJson(bytes).object();
        const IceCandidate decoded = decodeIce(restored);

        QCOMPARE(decoded.candidate,     orig.candidate);
        QCOMPARE(decoded.sdpMid,        orig.sdpMid);
        QCOMPARE(decoded.sdpMLineIndex, orig.sdpMLineIndex);
        qDebug() << "[TstIceCoding] JSON document round-trip OK, bytes:" << bytes.size();
    }

    // -----------------------------------------------------------------------
    // Case 6: sdpMLineIndex is NOT a string (guard against wrong encoding)
    // -----------------------------------------------------------------------
    void test_sdpMLineIndex_notString() {
        qDebug() << Q_FUNC_INFO;
        IceCandidate ice;
        ice.candidate     = "candidate:1 1 udp 2 10.0.0.1 9 typ host";
        ice.sdpMid        = "audio";
        ice.sdpMLineIndex = 0;

        const QJsonObject msg   = encodeIce("peer-y", ice);
        const QJsonValue idxVal = msg["payload"].toObject()["sdpMLineIndex"];

        QVERIFY(!idxVal.isString());
        QVERIFY(idxVal.isDouble());
        qDebug() << "[TstIceCoding] sdpMLineIndex is numeric (not string) OK";
    }
};

QTEST_MAIN(TstIceCoding)
#include "tst_icecoding.moc"
