#pragma once

#include <QAbstractListModel>
#include <QFileInfo>
#include <QStringList>

struct VideoItem {
    QString filePath;
    QString fileName;
    qint64 fileSizeBytes{0};
};

class VideoListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit VideoListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                  const QModelIndex &destinationParent, int destinationChild) override;

    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) override;

    void addFiles(const QStringList &files);
    void removeAt(int index);
    void clear();
    QStringList getFilePaths() const;
    const VideoItem& getItem(int index) const;

private:
    QList<VideoItem> m_items;
};