#include "VideoListModel.h"
#include <QMimeData>
#include <QUrl>
#include <QDataStream>
#include <QDirIterator>
#include <QIODevice>

namespace {
    constexpr char MIME_INTERNAL[] = "application/x-videomodel-items";
}

VideoListModel::VideoListModel(QObject *parent)
    : QAbstractListModel(parent) {}

int VideoListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_items.size());
}

QVariant VideoListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};

    const auto &item = m_items[index.row()];
    if (role == Qt::DisplayRole) {
        double mb = static_cast<double>(item.fileSizeBytes) / (1024.0 * 1024.0);
        return QString("%1 (%2 MB)\n%3")
            .arg(item.fileName)
            .arg(QString::number(mb, 'f', 1))
            .arg(item.filePath);
    } else if (role == Qt::UserRole) {
        return item.filePath;
    }
    return {};
}

Qt::ItemFlags VideoListModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    if (index.isValid()) {
        return defaultFlags | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren;
    }
    return defaultFlags | Qt::ItemIsDropEnabled;
}

bool VideoListModel::insertRows(int row, int count, const QModelIndex &parent) {
    if (parent.isValid() || count <= 0) return false;
    beginInsertRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        m_items.insert(row, VideoItem{});
    }
    endInsertRows();
    return true;
}

bool VideoListModel::removeRows(int row, int count, const QModelIndex &parent) {
    if (parent.isValid() || count <= 0 || row < 0 || (row + count) > m_items.size())
        return false;
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    m_items.remove(row, count);
    endRemoveRows();
    return true;
}

bool VideoListModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                              const QModelIndex &destinationParent, int destinationChild) {
    if (sourceParent.isValid() || destinationParent.isValid() || count <= 0)
        return false;
    if (sourceRow < 0 || sourceRow + count > m_items.size())
        return false;
    if (destinationChild < 0 || destinationChild > m_items.size())
        return false;
    if (destinationChild >= sourceRow && destinationChild <= sourceRow + count)
        return false;

    int adjustedDest = destinationChild;
    if (!beginMoveRows(QModelIndex(), sourceRow, sourceRow + count - 1, QModelIndex(), adjustedDest))
        return false;

    QList<VideoItem> moved;
    for (int i = 0; i < count; ++i) {
        moved.append(m_items.at(sourceRow + i));
    }
    m_items.remove(sourceRow, count);

    int insertPos = (destinationChild > sourceRow) ? (destinationChild - count) : destinationChild;
    for (int i = 0; i < moved.size(); ++i) {
        m_items.insert(insertPos + i, moved.at(i));
    }

    endMoveRows();
    return true;
}

Qt::DropActions VideoListModel::supportedDropActions() const {
    return Qt::MoveAction | Qt::CopyAction;
}

QStringList VideoListModel::mimeTypes() const {
    return {MIME_INTERNAL, "text/uri-list"};
}

QMimeData *VideoListModel::mimeData(const QModelIndexList &indexes) const {
    auto *data = new QMimeData();
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    for (const auto &idx : indexes) {
        if (idx.isValid()) {
            stream << idx.row();
        }
    }
    data->setData(MIME_INTERNAL, encoded);
    return data;
}

bool VideoListModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                  int row, int column, const QModelIndex &parent) {
    Q_UNUSED(column);
    if (action == Qt::IgnoreAction) return true;

    int targetRow = -1;
    if (row != -1) targetRow = row;
    else if (parent.isValid()) targetRow = parent.row();
    else targetRow = m_items.size();

    if (data->hasFormat(MIME_INTERNAL)) {
        QByteArray encoded = data->data(MIME_INTERNAL);
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        QList<int> rows;
        while (!stream.atEnd()) {
            int r;
            stream >> r;
            rows.append(r);
        }
        if (rows.isEmpty()) return false;
        return moveRows(QModelIndex(), rows.first(), rows.size(), QModelIndex(), targetRow);
    }

    if (data->hasUrls()) {
        QStringList paths;
        for (const auto &url : data->urls()) {
            if (url.isLocalFile()) {
                paths.append(url.toLocalFile());
            }
        }
        addFiles(paths);
        return true;
    }

    return false;
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
            // Recursively search the directory for video files
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

void VideoListModel::removeAt(int index) {
    if (index >= 0 && index < m_items.size()) {
        beginRemoveRows(QModelIndex(), index, index);
        m_items.removeAt(index);
        endRemoveRows();
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

const VideoItem& VideoListModel::getItem(int index) const {
    return m_items.at(index);
}