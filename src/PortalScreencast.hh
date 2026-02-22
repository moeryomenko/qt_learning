#pragma once
#include <QDBusObjectPath>
#include <QObject>
#include <QVariantMap>

class PortalScreencast : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool active READ active NOTIFY activeChanged)
  Q_PROPERTY(uint videoNodeId READ videoNodeId NOTIFY videoNodeIdChanged)
  Q_PROPERTY(int pipeWireFd READ pipeWireFd NOTIFY pipeWireFdChanged)
  Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

 public:
  explicit PortalScreencast(QObject* parent = nullptr);

  [[nodiscard]] bool active() const {
    return m_active;
  }
  [[nodiscard]] uint videoNodeId() const {
    return m_videoNodeId;
  }
  [[nodiscard]] int pipeWireFd() const {
    return m_pipeWireFd;
  }
  [[nodiscard]] QString lastError() const {
    return m_lastError;
  }

  Q_INVOKABLE void startScreenCast();
  Q_INVOKABLE void stop();

 signals:
  void activeChanged();
  void videoNodeIdChanged();
  void pipeWireFdChanged();
  void lastErrorChanged();
  void info(QString message);
  void error(QString message);

 private slots:
  // Invoked by QDBusConnection::connect for portal Request "Response" signals
  void handleCreateSessionResponse(uint response, const QVariantMap& results);
  void handleSelectSourcesResponse(uint response, const QVariantMap& results);
  void handleStartResponse(uint response, const QVariantMap& results);

  void setError(const QString& e);
  void createSession();
  void selectSources();
  void startSession();
  void openPipeWireRemote();

 private:
  bool    m_active = false;
  QString m_lastError;
  QString m_sessionHandle;
  uint    m_videoNodeId = 0;
  int     m_pipeWireFd  = -1;
};
