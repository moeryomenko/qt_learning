/* The GPLv3 License (GPLv3)

Copyright (c) 2022 Maxim Eryomenko <maxim_eryomenko@rambler.ru>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QDebug>

#include "VideoTile.h"

#include <gst/gst.h>
#include <pipewire/pipewire.h>

#include "ApiClient.h"
#include "SignalingClient.h"
#include "WebRtcPeerManager.h"
#include "PortalScreencast.h"
#include "PipeWireNodeModel.h"

// Convert an HTTP(S) base URL to a WebSocket base URL
static QString toWsUrl(const QString& httpUrl) {
    if (httpUrl.startsWith(QLatin1String("https://")))
        return QStringLiteral("wss://") + httpUrl.mid(8);
    if (httpUrl.startsWith(QLatin1String("http://")))
        return QStringLiteral("ws://") + httpUrl.mid(7);
    return httpUrl; // already ws:// or wss://
}

int main(int argc, char **argv) {
    using namespace Qt::StringLiterals;

    // Initialize GStreamer before creating QGuiApplication so it can consume
    // its own argv entries without interfering with Qt's argv parsing.
    gst_init(&argc, &argv);
    qDebug() << Q_FUNC_INFO << "GStreamer initialized, version:" << gst_version_string();

    // Initialize PipeWire (used by PipeWireNodeModel and pipewiresrc)
    pw_init(&argc, &argv);
    qDebug() << Q_FUNC_INFO << "PipeWire initialized, version:" << pw_get_library_version();

    QGuiApplication app(argc, argv);
    app.setApplicationName(u"QtWebRTCChat"_s);
    app.setOrganizationName(u"QtLearning"_s);

    // -----------------------------------------------------------------------
    // Backend objects
    // -----------------------------------------------------------------------
    ApiClient         apiClient;
    SignalingClient   signalingClient;
    WebRtcPeerManager rtcManager;
    PortalScreencast  portalScreencast;
    PipeWireNodeModel pipeWireNodeModel;

    // Default server — override at runtime via APP_SERVER_URL env var
    const QString kDefaultServer =
        qEnvironmentVariable("APP_SERVER_URL", "https://213.171.25.246");
    apiClient.setBaseUrl(kDefaultServer);
    qDebug() << Q_FUNC_INFO << "Default server:" << kDefaultServer
             << "(source:" << (qEnvironmentVariableIsSet("APP_SERVER_URL") ? "APP_SERVER_URL env" : "built-in default") << ")";

    // -----------------------------------------------------------------------
    // ApiClient ↔ SignalingClient
    // On successful login/register, auto-connect the signaling WebSocket.
    // -----------------------------------------------------------------------
    auto connectSignaling = [&](const AuthTokens& tokens) {
        const QString wsUrl = toWsUrl(apiClient.baseUrl());
        qDebug() << Q_FUNC_INFO << "Auth success — connecting signaling to" << wsUrl;
        signalingClient.connectToServer(wsUrl, tokens.accessToken);
    };
    QObject::connect(&apiClient, &ApiClient::loginSuccess,   connectSignaling);
    QObject::connect(&apiClient, &ApiClient::registerSuccess, connectSignaling);

    // -----------------------------------------------------------------------
    // SignalingClient ↔ WebRtcPeerManager: peer lifecycle
    // -----------------------------------------------------------------------
    QObject::connect(&signalingClient, &SignalingClient::peerJoined,
                     [&](const QString& peerId, const QString& username) {
        qDebug() << Q_FUNC_INFO << "peerJoined → ensurePeer:" << peerId << "(" << username << ")";
        rtcManager.ensurePeer(peerId);
    });

    QObject::connect(&signalingClient, &SignalingClient::peerLeft,
                     [&](const QString& peerId) {
        qDebug() << Q_FUNC_INFO << "peerLeft → dropPeer:" << peerId;
        rtcManager.dropPeer(peerId);
    });

    // -----------------------------------------------------------------------
    // Inbound signaling → RTC manager
    // -----------------------------------------------------------------------
    QObject::connect(&signalingClient, &SignalingClient::remoteOffer,
                     &rtcManager,      &WebRtcPeerManager::onRemoteOffer);

    QObject::connect(&signalingClient, &SignalingClient::remoteAnswer,
                     &rtcManager,      &WebRtcPeerManager::onRemoteAnswer);

    QObject::connect(&signalingClient, &SignalingClient::remoteIce,
                     &rtcManager,      &WebRtcPeerManager::onRemoteIce);

    // -----------------------------------------------------------------------
    // Outbound signaling ← RTC manager
    // -----------------------------------------------------------------------
    QObject::connect(&rtcManager,      &WebRtcPeerManager::localOfferReady,
                     &signalingClient, &SignalingClient::sendOffer);

    QObject::connect(&rtcManager,      &WebRtcPeerManager::localAnswerReady,
                     &signalingClient, &SignalingClient::sendAnswer);

    QObject::connect(&rtcManager,      &WebRtcPeerManager::localIceReady,
                     &signalingClient, &SignalingClient::sendIce);

    // -----------------------------------------------------------------------
    // PortalScreencast → WebRtcPeerManager: hand off PipeWire FD + node ID
    // -----------------------------------------------------------------------
    QObject::connect(&portalScreencast, &PortalScreencast::activeChanged,
                     [&]() {
        if (!portalScreencast.active()) return;
        const int  fd     = portalScreencast.pipeWireFd();
        const uint nodeId = portalScreencast.videoNodeId();
        qDebug() << Q_FUNC_INFO << "PortalScreencast active — fd:" << fd << "nodeId:" << nodeId;
        if (rtcManager.running())
            rtcManager.switchToScreen(fd, nodeId);
        else
            rtcManager.startWithScreen(fd, nodeId);
    });

    // -----------------------------------------------------------------------
    // Log RTC info / errors to console
    // -----------------------------------------------------------------------
    QObject::connect(&rtcManager, &WebRtcPeerManager::info,
                     [](const QString& msg) { qDebug()   << "[rtcManager]" << msg; });
    QObject::connect(&rtcManager, &WebRtcPeerManager::error,
                     [](const QString& msg) { qWarning() << "[rtcManager] ERROR:" << msg; });

    QObject::connect(&portalScreencast, &PortalScreencast::info,
                     [](const QString& msg) { qDebug()   << "[portal]" << msg; });
    QObject::connect(&portalScreencast, &PortalScreencast::error,
                     [](const QString& msg) { qWarning() << "[portal] ERROR:" << msg; });

    // -----------------------------------------------------------------------
    // QML engine
    // -----------------------------------------------------------------------
    QQmlApplicationEngine engine;

    QQmlContext* ctx = engine.rootContext();
    ctx->setContextProperty(u"apiClient"_s,        &apiClient);
    ctx->setContextProperty(u"signalingClient"_s,  &signalingClient);
    ctx->setContextProperty(u"rtcManager"_s,        &rtcManager);
    ctx->setContextProperty(u"portalScreencast"_s,  &portalScreencast);
    ctx->setContextProperty(u"pipeWireNodeModel"_s, &pipeWireNodeModel);

    // Register C++ QML types into the "pages" URI so VoiceChannelView can use VideoTile
    qmlRegisterType<VideoTile>("pages", 1, 0, "VideoTile");
    qDebug() << Q_FUNC_INFO << "VideoTile registered in pages 1.0";

    qDebug() << Q_FUNC_INFO << "Context properties registered — loading QML";

    const QUrl url(u"qrc:/pages/main.qml"_s);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [&url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl) {
                qCritical() << "[main] QML root failed to load:" << url;
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    engine.load(url);

    qDebug() << Q_FUNC_INFO << "Entering event loop";
    const int result = app.exec();
    qDebug() << Q_FUNC_INFO << "Event loop exited:" << result;

    // Stop GStreamer pipeline before tearing down PipeWire
    rtcManager.stop();
    gst_deinit();
    pw_deinit();

    return result;
}
