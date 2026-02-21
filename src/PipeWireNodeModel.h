#pragma once

#include <QAbstractListModel>
#include <QVector>

#include <pipewire/pipewire.h>

class PipeWireNodeModel : public QAbstractListModel {
  Q_OBJECT

public:
  enum Roles {
    IdRole = Qt::UserRole + 1,
    NameRole,
    MediaClassRole,
    AppNameRole,
    NodeNickRole,
    IsMonitorRole,
    IsAudioRole,
    IsVideoRole,
    IsStreamRole,
  };

  struct Node {
    uint32_t id = 0;
    QString name;
    QString mediaClass;
    QString appName;
    QString nodeNick;
    bool isMonitor = false;
    bool isAudio = false;
    bool isVideo = false;
    bool isStream = false;
  };

  explicit PipeWireNodeModel(QObject *parent = nullptr);
  ~PipeWireNodeModel() override;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void refresh();
  Q_INVOKABLE int findDefaultAudioMonitorId() const;

  // Called from PW thread via invokeMethod — NOT for direct external use
  void addOrUpdate(const Node &n);
  void removeNode(uint32_t id);

signals:
  void info(QString message);
  void error(QString message);

private:
  void startEnumeration();
  void stopEnumeration();

private:
  QVector<Node> m_nodes;

  pw_thread_loop *m_loop = nullptr;
  pw_context *m_ctx = nullptr;
  pw_core *m_core = nullptr;
  pw_registry *m_registry = nullptr;
  spa_hook m_registryHook{};
  bool m_running = false;
};
