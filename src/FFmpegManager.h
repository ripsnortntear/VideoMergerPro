#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

class FFmpegManager : public QObject {
    Q_OBJECT
public:
    explicit FFmpegManager(QObject *parent = nullptr);

    bool checkFFmpegAvailability();
    QString getFFmpegPath() const;
    QString getFFprobePath() const;

    void downloadAndSetupFFmpeg();
    void cancelDownload();

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadStatus(const QString &message);
    void downloadFinished(bool success, const QString &errorString);

private:
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished();
    bool extractTarXz(const QString &archivePath, const QString &destDir);

    QNetworkAccessManager m_netManager;
    QNetworkReply *m_currentReply{nullptr};
    QFile m_tempFile;
    QString m_ffmpegPath;
    QString m_ffprobePath;
};