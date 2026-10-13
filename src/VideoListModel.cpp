#include "VideoListModel.h"
#include <QMimeData>
#include <QUrl>
#include <QDataStream>
#include <QIODevice>
#include <QDirIterator>

VideoListModel::VideoListModel(QObject *parent)
    : QAbstractListModel(parent) {}

int VideoListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_items.size());
}

QVariant VideoListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_items.size()) {
        return QVariant();
    }

    const auto &item = m_items[index.row()];

    if (role == Qt::DisplayRole) {
        double mb = static_cast<double>(item.fileSizeBytes) / (1024.0 * 1024.0);
        return QString("%1. %2 (%3 MB)")
            .arg(index.row() + 1)
            .arg(item.fileName)
            .arg(mb, 0, 'f', 1);
    } else if (role == Qt::ToolTipRole) {
        return item.filePath;
    }

    return QVariant();
}

Qt::ItemFlags VideoListModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    if (index.isValid()) {
        return defaultFlags | Qt::ItemIsDragEnabled;
    }
    return defaultFlags | Qt::ItemIsDropEnabled;
}

Qt::DropActions VideoListModel::supportedDropActions() const {
    return Qt::MoveAction;
}

QStringList VideoListModel::mimeTypes() const {
    return {"application/x-videolist-internal"};
}

QMimeData *VideoListModel::mimeData(const QModelIndexList &indexes) const {
    auto *mime = new QMimeData();
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);

    for (const auto &idx : indexes) {
        if (idx.isValid()) {
            stream << idx.row();
        }
    }

    mime->setData("application/x-videolist-internal", encoded);
    return mime;
}

bool VideoListModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
    Q_UNUSED(column);
    if (action != Qt::MoveAction) return false;
    if (!data->hasFormat("application/x-videolist-internal")) return false;

    QByteArray encoded = data->data("application/x-videolist-internal");
    QDataStream stream(&encoded, QIODevice::ReadOnly);

    int sourceRow;
    stream >> sourceRow;

    if (sourceRow < 0 || sourceRow >= m_items.size()) return false;

    int destRow = row;
    if (destRow < 0) {
        destRow = parent.isValid() ? parent.row() : m_items.size();
    }

    if (destRow > sourceRow) {
        destRow--;
    }

    if (destRow == sourceRow) return false;

    beginMoveRows(QModelIndex(), sourceRow, sourceRow, QModelIndex(), destRow > sourceRow ? destRow + 1 : destRow);
    m_items.move(sourceRow, destRow);
    endMoveRows();

    emit dataChanged(index(0), index(m_items.size() - 1), {Qt::DisplayRole});
    return true;
}

void VideoListModel::addFiles(const QStringList &files) {
    const QStringList validExts = {"mp4", "mkv", "avi", "mov", "ts", "flv", "webm", "wmv", "m4v"};
    QList<VideoItem> toAdd;

    auto tryAddFile = [&](const QFileInfo &fi) {
        if (!fi.exists() || !fi.isFile() || !validExts.contains(fi.suffix().toLower())) {
            return;
        }

        bool duplicate = false;
        for (const auto &existing : m_items) {
            if (existing.filePath.compare(fi.absoluteFilePath(), Qt::CaseInsensitive) == 0) {
                duplicate = true;
                break;
            }
        }
        for (const auto &pending : toAdd) {
            if (pending.filePath.compare(fi.absoluteFilePath(), Qt::CaseInsensitive) == 0) {
                duplicate = true;
                break;
            }
        }

        if (!duplicate) {
            toAdd.append(VideoItem{fi.absoluteFilePath(), fi.fileName(), fi.size()});
        }
    };

    for (const auto &path : files) {
        QFileInfo fi(path);
        if (!fi.exists()) continue;

        if (fi.isDir()) {
            QDirIterator it(fi.absoluteFilePath(),
                            QDir::Files | QDir::Readable,
                            QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                tryAddFile(it.fileInfo());
            }
        } else if (fi.isFile()) {
            tryAddFile(fi);
        }
    }

    if (toAdd.isEmpty()) return;

    beginInsertRows(QModelIndex(), m_items.size(), m_items.size() + toAdd.size() - 1);
    m_items.append(toAdd);
    endInsertRows();
}

void VideoListModel::removeAt(int row) {
    if (row < 0 || row >= m_items.size()) return;
    beginRemoveRows(QModelIndex(), row, row);
    m_items.removeAt(row);
    endRemoveRows();

    if (!m_items.isEmpty()) {
        emit dataChanged(index(0), index(m_items.size() - 1), {Qt::DisplayRole});
    }
}

void VideoListModel::clear() {
    if (m_items.isEmpty()) return;
    beginResetModel();
    m_items.clear();
    endResetModel();
}

QStringList VideoListModel::getFilePaths() const {
    QStringList paths;
    paths.reserve(m_items.size());
    for (const auto &item : m_items) {
        paths.append(item.filePath);
    }
    return paths;
}

const VideoItem& VideoListModel::getItem(int row) const {
    return m_items.at(row);
}