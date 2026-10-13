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
    [[nodiscard]] QString getFFmpegPath() const;
    [[nodiscard]] QString getFFprobePath() const;

    void downloadAndSetupFFmpeg();
    void cancelDownload();

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadStatus(const QString &message);
    void downloadFinished(bool success, const QString &errorMessage);

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();

private:
    bool extractZip(const QString &zipPath, const QString &destDir);

    QString m_ffmpegPath;
    QString m_ffprobePath;
    QNetworkAccessManager m_netManager;
    QNetworkReply *m_reply{nullptr};
    QFile m_tempFile;
};