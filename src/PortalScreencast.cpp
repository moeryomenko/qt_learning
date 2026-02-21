#include "PortalScreencast.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QDebug>
#include <QRandomGenerator>

static constexpr const char *PORTAL_BUS = "org.freedesktop.portal.Desktop";
static constexpr const char *PORTAL_PATH = "/org/freedesktop/portal/desktop";
static constexpr const char *SCREENCAST_IF =
    "org.freedesktop.portal.ScreenCast";
static constexpr const char *REQUEST_IF = "org.freedesktop.portal.Request";

PortalScreencast::PortalScreencast(QObject *parent) : QObject(parent) {
  qDebug() << Q_FUNC_INFO << "PortalScreencast created";
}

void PortalScreencast::setError(const QString &e) {
  qWarning() << Q_FUNC_INFO << "Error:" << e;
  m_lastError = e;
  emit lastErrorChanged();
  emit error(e);
}

void PortalScreencast::startScreenCast() {
  if (m_active) {
    qDebug() << Q_FUNC_INFO << "Already active, ignoring";
    return;
  }
  qDebug() << Q_FUNC_INFO << "Starting screen cast";

  m_videoNodeId = 0;
  emit videoNodeIdChanged();
  m_pipeWireFd = -1;
  emit pipeWireFdChanged();
  m_sessionHandle.clear();
  m_lastError.clear();
  emit lastErrorChanged();

  createSession();
}

void PortalScreencast::stop() {
  qDebug() << Q_FUNC_INFO << "Stopping. sessionHandle:" << m_sessionHandle;
  if (!m_active && m_sessionHandle.isEmpty())
    return;

  if (!m_sessionHandle.isEmpty()) {
    QDBusMessage msg = QDBusMessage::createMethodCall(
        PORTAL_BUS, PORTAL_PATH, SCREENCAST_IF, "CloseSession");
    msg << QDBusObjectPath(m_sessionHandle);
    QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
    qDebug() << Q_FUNC_INFO << "Sent CloseSession for:" << m_sessionHandle;
  }

  m_sessionHandle.clear();
  m_active = false;
  emit activeChanged();
  m_pipeWireFd = -1;
  emit pipeWireFdChanged();
  emit info("Screen cast stopped");
}

void PortalScreencast::createSession() {
  qDebug() << Q_FUNC_INFO << "Creating portal session";
  QDBusMessage msg = QDBusMessage::createMethodCall(
      PORTAL_BUS, PORTAL_PATH, SCREENCAST_IF, "CreateSession");

  QVariantMap opts;
  opts["handle_token"] =
      QString("qt_%1").arg(QRandomGenerator::global()->generate());
  opts["session_handle_token"] =
      QString("sess_%1").arg(QRandomGenerator::global()->generate());
  msg << opts;

  QDBusPendingReply<QDBusObjectPath> reply =
      QDBusConnection::sessionBus().asyncCall(msg);

  auto *watcher = new QDBusPendingCallWatcher(reply, this);
  connect(watcher, &QDBusPendingCallWatcher::finished, this,
          [this, watcher](QDBusPendingCallWatcher *w) {
            QDBusPendingReply<QDBusObjectPath> r = *w;
            watcher->deleteLater();
            if (r.isError()) {
              setError("CreateSession failed: " + r.error().message());
              return;
            }
            const QString reqPath = r.value().path();
            qDebug() << Q_FUNC_INFO
                     << "Awaiting CreateSession response on:" << reqPath;
            QDBusConnection::sessionBus().connect(
                PORTAL_BUS, reqPath, REQUEST_IF, "Response", this,
                SLOT(handleCreateSessionResponse(uint, QVariantMap)));
          });
}

void PortalScreencast::handleCreateSessionResponse(uint response,
                                                   const QVariantMap &results) {
  qDebug() << Q_FUNC_INFO << "response:" << response;
  if (response != 0) {
    setError(QString("CreateSession rejected (%1)").arg(response));
    return;
  }
  QString session = results.value("session_handle").toString();
  if (session.isEmpty()) {
    auto v = results.value("session_handle");
    if (v.canConvert<QDBusObjectPath>())
      session = v.value<QDBusObjectPath>().path();
  }
  if (session.isEmpty()) {
    setError("Missing session_handle in CreateSession response");
    return;
  }
  m_sessionHandle = session;
  qDebug() << Q_FUNC_INFO << "Session handle:" << m_sessionHandle;
  selectSources();
}

void PortalScreencast::selectSources() {
  qDebug() << Q_FUNC_INFO << "Selecting sources";
  QDBusMessage msg = QDBusMessage::createMethodCall(
      PORTAL_BUS, PORTAL_PATH, SCREENCAST_IF, "SelectSources");

  QVariantMap opts;
  opts["handle_token"] =
      QString("sel_%1").arg(QRandomGenerator::global()->generate());
  opts["types"] = (uint)(1 | 2); // monitor | window
  opts["multiple"] = false;
  opts["cursor_mode"] = (uint)2; // embedded cursor
  msg << QDBusObjectPath(m_sessionHandle) << opts;

  QDBusPendingReply<QDBusObjectPath> reply =
      QDBusConnection::sessionBus().asyncCall(msg);

  auto *watcher = new QDBusPendingCallWatcher(reply, this);
  connect(watcher, &QDBusPendingCallWatcher::finished, this,
          [this, watcher](QDBusPendingCallWatcher *w) {
            QDBusPendingReply<QDBusObjectPath> r = *w;
            watcher->deleteLater();
            if (r.isError()) {
              setError("SelectSources failed: " + r.error().message());
              return;
            }
            const QString reqPath = r.value().path();
            qDebug() << Q_FUNC_INFO
                     << "Awaiting SelectSources response on:" << reqPath;
            QDBusConnection::sessionBus().connect(
                PORTAL_BUS, reqPath, REQUEST_IF, "Response", this,
                SLOT(handleSelectSourcesResponse(uint, QVariantMap)));
          });
}

void PortalScreencast::handleSelectSourcesResponse(uint response,
                                                   const QVariantMap &) {
  qDebug() << Q_FUNC_INFO << "response:" << response;
  if (response != 0) {
    setError(QString("SelectSources rejected (%1)").arg(response));
    return;
  }
  startSession();
}

void PortalScreencast::startSession() {
  qDebug() << Q_FUNC_INFO << "Starting session";
  QDBusMessage msg = QDBusMessage::createMethodCall(PORTAL_BUS, PORTAL_PATH,
                                                    SCREENCAST_IF, "Start");

  QVariantMap opts;
  opts["handle_token"] =
      QString("start_%1").arg(QRandomGenerator::global()->generate());
  msg << QDBusObjectPath(m_sessionHandle) << QString("") << opts;

  QDBusPendingReply<QDBusObjectPath> reply =
      QDBusConnection::sessionBus().asyncCall(msg);

  auto *watcher = new QDBusPendingCallWatcher(reply, this);
  connect(watcher, &QDBusPendingCallWatcher::finished, this,
          [this, watcher](QDBusPendingCallWatcher *w) {
            QDBusPendingReply<QDBusObjectPath> r = *w;
            watcher->deleteLater();
            if (r.isError()) {
              setError("Start failed: " + r.error().message());
              return;
            }
            const QString reqPath = r.value().path();
            qDebug() << Q_FUNC_INFO << "Awaiting Start response on:" << reqPath;
            QDBusConnection::sessionBus().connect(
                PORTAL_BUS, reqPath, REQUEST_IF, "Response", this,
                SLOT(handleStartResponse(uint, QVariantMap)));
          });
}

void PortalScreencast::handleStartResponse(uint response,
                                           const QVariantMap &results) {
  qDebug() << Q_FUNC_INFO << "response:" << response;
  if (response != 0) {
    setError(QString("Start rejected (%1)").arg(response));
    return;
  }

  // streams has DBus type a(ua{sv}) — must unpack via QDBusArgument,
  // not QVariant::toList() which returns empty for marshalled DBus types.
  uint videoNode = 0;
  const QVariant streamsVariant = results.value("streams");
  qDebug() << Q_FUNC_INFO
           << "[FIX] streams variant type:" << streamsVariant.typeName()
           << "isValid:" << streamsVariant.isValid();

  if (streamsVariant.canConvert<QDBusArgument>()) {
    const QDBusArgument arg = streamsVariant.value<QDBusArgument>();
    arg.beginArray();
    int streamCount = 0;
    while (!arg.atEnd()) {
      uint nodeId = 0;
      QVariantMap props;
      arg.beginStructure();
      arg >> nodeId >> props;
      arg.endStructure();
      qDebug() << Q_FUNC_INFO << "[FIX] Stream" << streamCount
               << "node id:" << nodeId << "props:" << props;
      if (!videoNode)
        videoNode = nodeId;
      ++streamCount;
    }
    arg.endArray();
    qDebug() << Q_FUNC_INFO << "[FIX] Parsed" << streamCount
             << "streams via QDBusArgument";
  } else {
    // Fallback: plain QVariantList (unlikely but keep for safety)
    const QVariantList streams = streamsVariant.toList();
    qDebug() << Q_FUNC_INFO
             << "[FIX] Fallback QVariantList streams count:" << streams.size();
    for (const QVariant &s : streams) {
      const QVariantList pair = s.toList();
      if (pair.size() < 2)
        continue;
      uint nodeId = pair[0].toUInt();
      if (!videoNode)
        videoNode = nodeId;
    }
  }

  if (!videoNode) {
    setError("Portal did not return video node id");
    return;
  }

  m_videoNodeId = videoNode;
  emit videoNodeIdChanged();
  qDebug() << Q_FUNC_INFO << "Video node id:" << m_videoNodeId;

  openPipeWireRemote();
}

void PortalScreencast::openPipeWireRemote() {
  qDebug() << Q_FUNC_INFO
           << "Opening PipeWire remote for session:" << m_sessionHandle;
  QDBusMessage msg = QDBusMessage::createMethodCall(
      PORTAL_BUS, PORTAL_PATH, SCREENCAST_IF, "OpenPipeWireRemote");

  QVariantMap opts;
  opts["handle_token"] =
      QString("pw_%1").arg(QRandomGenerator::global()->generate());
  msg << QDBusObjectPath(m_sessionHandle) << opts;

  QDBusPendingReply<QDBusUnixFileDescriptor> reply =
      QDBusConnection::sessionBus().asyncCall(msg);

  auto *watcher = new QDBusPendingCallWatcher(reply, this);
  connect(watcher, &QDBusPendingCallWatcher::finished, this,
          [this, watcher](QDBusPendingCallWatcher *w) {
            QDBusPendingReply<QDBusUnixFileDescriptor> r = *w;
            watcher->deleteLater();
            if (r.isError()) {
              setError("OpenPipeWireRemote failed: " + r.error().message());
              return;
            }
            QDBusUnixFileDescriptor fd = r.value();
            if (!fd.isValid()) {
              setError("OpenPipeWireRemote returned invalid fd");
              return;
            }
            m_pipeWireFd = fd.fileDescriptor();
            emit pipeWireFdChanged();

            m_active = true;
            emit activeChanged();
            emit info("Screen cast active. videoNodeId=" +
                      QString::number(m_videoNodeId) +
                      " fd=" + QString::number(m_pipeWireFd));
            qDebug() << Q_FUNC_INFO
                     << "PortalScreencast active. fd:" << m_pipeWireFd
                     << "videoNodeId:" << m_videoNodeId;
          });
}
