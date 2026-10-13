#include "FFmpegManager.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

FFmpegManager::FFmpegManager(QObject *parent)
    : QObject(parent) {}

bool FFmpegManager::checkFFmpegAvailability() {
    // 1. Look adjacent to running executable
    QString localDir = QCoreApplication::applicationDirPath();
    QString localFFmpeg = localDir + "/ffmpeg.exe";
    QString localFFprobe = localDir + "/ffprobe.exe";

    if (QFileInfo::exists(localFFmpeg) && QFileInfo::exists(localFFprobe)) {
        m_ffmpegPath = localFFmpeg;
        m_ffprobePath = localFFprobe;
        return true;
    }

    // 2. Look in AppData bootstrap directory
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString appDataFFmpeg = appData + "/ffmpeg/bin/ffmpeg.exe";
    QString appDataFFprobe = appData + "/ffmpeg/bin/ffprobe.exe";

    if (QFileInfo::exists(appDataFFmpeg) && QFileInfo::exists(appDataFFprobe)) {
        m_ffmpegPath = appDataFFmpeg;
        m_ffprobePath = appDataFFprobe;
        return true;
    }

    // 3. Fallback to system PATH
    QString pathFFmpeg = QStandardPaths::findExecutable("ffmpeg");
    QString pathFFprobe = QStandardPaths::findExecutable("ffprobe");

    if (!pathFFmpeg.isEmpty() && !pathFFprobe.isEmpty()) {
        m_ffmpegPath = pathFFmpeg;
        m_ffprobePath = pathFFprobe;
        return true;
    }

    return false;
}

QString FFmpegManager::getFFmpegPath() const {
    return m_ffmpegPath;
}

QString FFmpegManager::getFFprobePath() const {
    return m_ffprobePath;
}

void FFmpegManager::downloadAndSetupFFmpeg() {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);

    QString tempZip = appData + "/ffmpeg_setup.zip";
    m_tempFile.setFileName(tempZip);

    if (!m_tempFile.open(QIODevice::WriteOnly)) {
        emit downloadFinished(false, "Failed to open temporary destination file for writing.");
        return;
    }

    QUrl url("https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip");
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    emit downloadStatus("Connecting to FFmpeg build repository...");
    m_reply = m_netManager.get(request);

    connect(m_reply, &QNetworkReply::downloadProgress, this, &FFmpegManager::onDownloadProgress);
    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        if (m_reply) {
            m_tempFile.write(m_reply->readAll());
        }
    });
    connect(m_reply, &QNetworkReply::finished, this, &FFmpegManager::onDownloadFinished);
}

void FFmpegManager::cancelDownload() {
    if (m_reply && m_reply->isRunning()) {
        m_reply->abort();
    }
}

void FFmpegManager::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    emit downloadProgress(bytesReceived, bytesTotal);
}

void FFmpegManager::onDownloadFinished() {
    if (!m_reply) return;

    m_tempFile.close();

    if (m_reply->error() != QNetworkReply::NoError) {
        QString err = m_reply->errorString();
        m_reply->deleteLater();
        m_reply = nullptr;
        m_tempFile.remove();
        emit downloadFinished(false, "Download failed: " + err);
        return;
    }

    m_reply->deleteLater();
    m_reply = nullptr;

    emit downloadStatus("Extracting FFmpeg binaries...");

    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString extractDir = appData + "/ffmpeg_temp";
    QDir().mkpath(extractDir);

    if (!extractZip(m_tempFile.fileName(), extractDir)) {
        m_tempFile.remove();
        emit downloadFinished(false, "Failed to extract FFmpeg archive.");
        return;
    }

    m_tempFile.remove();

    QDir dir(extractDir);
    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (subdirs.isEmpty()) {
        emit downloadFinished(false, "Invalid directory layout in FFmpeg archive.");
        return;
    }

    QString targetDir = appData + "/ffmpeg";
    QDir(targetDir).removeRecursively();

    QString unpackedRoot = extractDir + "/" + subdirs.first();
    if (!QDir().rename(unpackedRoot, targetDir)) {
        emit downloadFinished(false, "Could not move FFmpeg to target location.");
        return;
    }

    QDir(extractDir).removeRecursively();

    if (checkFFmpegAvailability()) {
        emit downloadFinished(true, QString());
    } else {
        emit downloadFinished(false, "FFmpeg setup concluded, but binaries could not be validated.");
    }
}

bool FFmpegManager::extractZip(const QString &zipPath, const QString &destDir) {
    QProcess tar;
    tar.start("tar", {"-xf", zipPath, "-C", destDir});
    if (!tar.waitForStarted(3000)) {
        return false;
    }
    tar.waitForFinished(-1);
    return (tar.exitStatus() == QProcess::NormalExit && tar.exitCode() == 0);
}