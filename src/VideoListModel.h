#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QFileInfo>

struct VideoItem {
    QString filePath;
    QString fileName;
    qint64 fileSizeBytes;
};

class VideoListModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit VideoListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // Drag and drop internal reordering
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    // Data manipulation
    void addFiles(const QStringList &files);
    void removeAt(int row);
    void clear();

    [[nodiscard]] QStringList getFilePaths() const;
    [[nodiscard]] const VideoItem& getItem(int row) const;

private:
    QList<VideoItem> m_items;
};