#include "FFmpegManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QDirIterator>

FFmpegManager::FFmpegManager(QObject *parent)
    : QObject(parent) {
    checkFFmpegAvailability();
}

bool FFmpegManager::checkFFmpegAvailability() {
    QString appLocalDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/FFmpeg/bin";
    QString localFFmpeg = appLocalDir + "/ffmpeg.exe";
    QString localFFprobe = appLocalDir + "/ffprobe.exe";

    if (QFileInfo::exists(localFFmpeg) && QFileInfo::exists(localFFprobe)) {
        m_ffmpegPath = QDir::toNativeSeparators(localFFmpeg);
        m_ffprobePath = QDir::toNativeSeparators(localFFprobe);
        return true;
    }

    QString systemFFmpeg = QStandardPaths::findExecutable("ffmpeg");
    QString systemFFprobe = QStandardPaths::findExecutable("ffprobe");

    if (!systemFFmpeg.isEmpty() && !systemFFprobe.isEmpty()) {
        m_ffmpegPath = QDir::toNativeSeparators(systemFFmpeg);
        m_ffprobePath = QDir::toNativeSeparators(systemFFprobe);
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
    // Official GitHub automated release build of FFmpeg for Windows 64-bit
    QUrl downloadUrl("https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip");

    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_tempFile.setFileName(tempDir + "/ffmpeg_bootstrap.zip");
    if (!m_tempFile.open(QIODevice::WriteOnly)) {
        emit downloadFinished(false, "Could not create temporary write file: " + m_tempFile.errorString());
        return;
    }

    QNetworkRequest request(downloadUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    emit downloadStatus("Connecting to FFmpeg build repository...");
    m_currentReply = m_netManager.get(request);

    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &FFmpegManager::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::readyRead, this, [this]() {
        if (m_currentReply) {
            m_tempFile.write(m_currentReply->readAll());
        }
    });
    connect(m_currentReply, &QNetworkReply::finished, this, &FFmpegManager::onDownloadFinished);
}

void FFmpegManager::cancelDownload() {
    if (m_currentReply && m_currentReply->isRunning()) {
        m_currentReply->abort();
    }
}

void FFmpegManager::onDownloadProgress(qint64 received, qint64 total) {
    emit downloadProgress(received, total);
}

void FFmpegManager::onDownloadFinished() {
    if (!m_currentReply) return;

    m_tempFile.close();

    if (m_currentReply->error() != QNetworkReply::NoError) {
        QString err = m_currentReply->errorString();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        m_tempFile.remove();
        emit downloadFinished(false, "Download failed: " + err);
        return;
    }

    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    emit downloadStatus("Extracting binaries via Windows Native Shell...");

    QString targetDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/FFmpeg";
    QDir().mkpath(targetDir);

    // Unpack ZIP archive using PowerShell 5.1+ Expand-Archive
    QString script = QString(
        "Expand-Archive -Path '%1' -DestinationPath '%2' -Force"
    ).arg(QDir::toNativeSeparators(m_tempFile.fileName()), QDir::toNativeSeparators(targetDir));

    QProcess ps;
    ps.start("powershell.exe", {"-NoProfile", "-NonInteractive", "-Command", script});
    if (!ps.waitForFinished(180000)) {
        m_tempFile.remove();
        emit downloadFinished(false, "Extraction timed out after 3 minutes.");
        return;
    }

    if (ps.exitStatus() != QProcess::NormalExit || ps.exitCode() != 0) {
        m_tempFile.remove();
        emit downloadFinished(false, "Archive extraction failed: " + QString::fromUtf8(ps.readAllStandardError()));
        return;
    }

    m_tempFile.remove();

    // Locate extracted ffmpeg.exe and ffprobe.exe
    QDirIterator it(targetDir, {"ffmpeg.exe"}, QDir::Files, QDirIterator::Subdirectories);
    if (!it.hasNext()) {
        emit downloadFinished(false, "ffmpeg.exe could not be located inside extracted directory.");
        return;
    }

    QString foundFFmpeg = it.next();
    QString binDir = QFileInfo(foundFFmpeg).absolutePath();
    QString foundFFprobe = binDir + "/ffprobe.exe";

    if (!QFileInfo::exists(foundFFprobe)) {
        emit downloadFinished(false, "ffprobe.exe could not be located alongside ffmpeg.exe.");
        return;
    }

    QString standardBinDir = targetDir + "/bin";
    QDir().mkpath(standardBinDir);
    QString targetFFmpeg = standardBinDir + "/ffmpeg.exe";
    QString targetFFprobe = standardBinDir + "/ffprobe.exe";

    QFile::remove(targetFFmpeg);
    QFile::remove(targetFFprobe);

    QFile::copy(foundFFmpeg, targetFFmpeg);
    QFile::copy(foundFFprobe, targetFFprobe);

    m_ffmpegPath = QDir::toNativeSeparators(targetFFmpeg);
    m_ffprobePath = QDir::toNativeSeparators(targetFFprobe);

    emit downloadStatus("FFmpeg successfully verified.");
    emit downloadFinished(true, QString());
}