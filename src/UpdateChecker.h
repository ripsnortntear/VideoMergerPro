#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

class UpdateChecker : public QObject {
    Q_OBJECT
public:
    explicit UpdateChecker(QString currentVersion, QObject *parent = nullptr);

    void checkForUpdates(bool manual = false);
    void downloadAndInstallUpdate(const QString &downloadUrl, const QString &fileName);
    void cancelDownload();

    [[nodiscard]] QString getCurrentVersion() const { return m_currentVersion; }

signals:
    void updateAvailable(const QString &newVersion, const QString &releaseNotes, const QString &downloadUrl);
    void upToDate(const QString &currentVersion);
    void checkFailed(const QString &errorString);
    void downloadProgress(qint64 received, qint64 total);
    void downloadFinished(bool success, const QString &errorString);

private:
    void onCheckFinished();
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished();
    bool isNewerVersion(const QString &latest, const QString &current);

    QString m_currentVersion;
    QString m_repoPath{"ripsnortntear/VideoMergerPro"};
    QNetworkAccessManager m_netManager;
    QNetworkReply *m_reply{nullptr};
    QFile m_installerFile;
    bool m_manualCheck{false};
};