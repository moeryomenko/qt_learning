#include "PipeWireNodeModel.h"

#include <QDebug>
#include <QMetaObject>

static constexpr const char* PW_NODE_TYPE = "PipeWire:Interface:Node";

static QString pw_prop(const spa_dict* dict, const char* key) {
    if (!dict) return {};
    const char* v = spa_dict_lookup(dict, key);
    return v ? QString::fromUtf8(v) : QString{};
}

// --- Registry callbacks (run on PW thread) ---

static void on_global(void* data, uint32_t id, uint32_t /*perms*/,
                      const char* type, uint32_t /*version*/,
                      const spa_dict* props)
{
    if (!type || QString::fromUtf8(type) != PW_NODE_TYPE) return;

    auto* self = static_cast<PipeWireNodeModel*>(data);

    PipeWireNodeModel::Node n;
    n.id         = id;
    n.name       = pw_prop(props, "node.name");
    n.nodeNick   = pw_prop(props, "node.nick");
    if (n.nodeNick.isEmpty())
        n.nodeNick = pw_prop(props, "node.description");
    n.mediaClass = pw_prop(props, "media.class");
    n.appName    = pw_prop(props, "application.name");

    const QString mc = n.mediaClass;
    n.isAudio  = mc.startsWith("Audio/");
    n.isVideo  = mc.startsWith("Video/");
    n.isStream = mc.contains("Stream");
    n.isMonitor = (mc == "Audio/Source" &&
                   (n.name.contains("monitor", Qt::CaseInsensitive) ||
                    n.nodeNick.contains("monitor", Qt::CaseInsensitive)));

    qDebug() << "[PipeWireNodeModel] on_global id:" << id
             << "mediaClass:" << mc << "name:" << n.name;

    // Marshal to Qt main thread
    QMetaObject::invokeMethod(self, [self, n]() {
        self->addOrUpdate(n);
    }, Qt::QueuedConnection);
}

static void on_global_remove(void* data, uint32_t id)
{
    auto* self = static_cast<PipeWireNodeModel*>(data);
    qDebug() << "[PipeWireNodeModel] on_global_remove id:" << id;

    QMetaObject::invokeMethod(self, [self, id]() {
        self->removeNode(id);
    }, Qt::QueuedConnection);
}

static const pw_registry_events kRegistryEvents = {
    .version       = PW_VERSION_REGISTRY_EVENTS,
    .global        = on_global,
    .global_remove = on_global_remove,
};

// --- PipeWireNodeModel ---

PipeWireNodeModel::PipeWireNodeModel(QObject* parent)
    : QAbstractListModel(parent)
{
    qDebug() << Q_FUNC_INFO << "PipeWireNodeModel created";
    pw_init(nullptr, nullptr);
    startEnumeration();
}

PipeWireNodeModel::~PipeWireNodeModel() {
    qDebug() << Q_FUNC_INFO << "PipeWireNodeModel destroyed";
    stopEnumeration();
    pw_deinit();
}

int PipeWireNodeModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_nodes.size();
}

QVariant PipeWireNodeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_nodes.size())
        return {};
    const auto& n = m_nodes[index.row()];
    switch (role) {
        case IdRole:        return (uint)n.id;
        case NameRole:      return n.name;
        case MediaClassRole: return n.mediaClass;
        case AppNameRole:   return n.appName;
        case NodeNickRole:  return n.nodeNick;
        case IsMonitorRole: return n.isMonitor;
        case IsAudioRole:   return n.isAudio;
        case IsVideoRole:   return n.isVideo;
        case IsStreamRole:  return n.isStream;
    }
    return {};
}

QHash<int, QByteArray> PipeWireNodeModel::roleNames() const {
    return {
        {IdRole,          "nodeId"},
        {NameRole,        "name"},
        {MediaClassRole,  "mediaClass"},
        {AppNameRole,     "appName"},
        {NodeNickRole,    "nodeNick"},
        {IsMonitorRole,   "isMonitor"},
        {IsAudioRole,     "isAudio"},
        {IsVideoRole,     "isVideo"},
        {IsStreamRole,    "isStream"},
    };
}

void PipeWireNodeModel::refresh() {
    qDebug() << Q_FUNC_INFO << "Refreshing node list";
    beginResetModel();
    m_nodes.clear();
    endResetModel();
    stopEnumeration();
    startEnumeration();
}

int PipeWireNodeModel::findDefaultAudioMonitorId() const {
    for (const auto& n : m_nodes) {
        if (n.isMonitor) {
            qDebug() << Q_FUNC_INFO << "Found monitor:" << n.id << n.name;
            return (int)n.id;
        }
    }
    for (const auto& n : m_nodes) {
        if (n.mediaClass == "Audio/Sink") {
            qDebug() << Q_FUNC_INFO << "Fallback to sink:" << n.id << n.name;
            return (int)n.id;
        }
    }
    if (!m_nodes.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Fallback to first node:" << m_nodes.first().id;
        return (int)m_nodes.first().id;
    }
    qWarning() << Q_FUNC_INFO << "No audio nodes found";
    return 0;
}

void PipeWireNodeModel::addOrUpdate(const Node& n) {
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].id == n.id) {
            m_nodes[i] = n;
            emit dataChanged(index(i), index(i));
            return;
        }
    }
    beginInsertRows(QModelIndex(), m_nodes.size(), m_nodes.size());
    m_nodes.push_back(n);
    endInsertRows();
    qDebug() << Q_FUNC_INFO << "Node added id:" << n.id << "class:" << n.mediaClass;
}

void PipeWireNodeModel::removeNode(uint32_t id) {
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].id == id) {
            beginRemoveRows(QModelIndex(), i, i);
            m_nodes.removeAt(i);
            endRemoveRows();
            qDebug() << Q_FUNC_INFO << "Node removed id:" << id;
            return;
        }
    }
}

void PipeWireNodeModel::startEnumeration() {
    if (m_running) return;
    qDebug() << Q_FUNC_INFO << "Starting PipeWire enumeration";

    m_loop = pw_thread_loop_new("pw-nodes", nullptr);
    if (!m_loop) {
        qWarning() << Q_FUNC_INFO << "pw_thread_loop_new failed";
        emit error("Failed to create PipeWire thread loop");
        return;
    }

    m_ctx = pw_context_new(pw_thread_loop_get_loop(m_loop), nullptr, 0);
    if (!m_ctx) {
        qWarning() << Q_FUNC_INFO << "pw_context_new failed";
        emit error("Failed to create PipeWire context");
        return;
    }

    m_core = pw_context_connect(m_ctx, nullptr, 0);
    if (!m_core) {
        qWarning() << Q_FUNC_INFO << "pw_context_connect failed";
        emit error("Failed to connect to PipeWire");
        return;
    }

    m_registry = pw_core_get_registry(m_core, PW_VERSION_REGISTRY, 0);
    if (!m_registry) {
        qWarning() << Q_FUNC_INFO << "pw_core_get_registry failed";
        emit error("Failed to get PipeWire registry");
        return;
    }

    spa_zero(m_registryHook);
    pw_registry_add_listener(m_registry, &m_registryHook, &kRegistryEvents, this);

    pw_thread_loop_start(m_loop);
    m_running = true;
    qDebug() << Q_FUNC_INFO << "PipeWire enumeration started";
}

void PipeWireNodeModel::stopEnumeration() {
    if (!m_running) return;
    qDebug() << Q_FUNC_INFO << "Stopping PipeWire enumeration";

    if (m_loop) pw_thread_loop_stop(m_loop);

    if (m_registry) {
        pw_proxy_destroy((pw_proxy*)m_registry);
        m_registry = nullptr;
    }
    if (m_core) {
        pw_core_disconnect(m_core);
        m_core = nullptr;
    }
    if (m_ctx) {
        pw_context_destroy(m_ctx);
        m_ctx = nullptr;
    }
    if (m_loop) {
        pw_thread_loop_destroy(m_loop);
        m_loop = nullptr;
    }

    m_running = false;
    qDebug() << Q_FUNC_INFO << "PipeWire enumeration stopped";
}
