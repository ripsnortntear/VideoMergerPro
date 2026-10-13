#include "UpdateChecker.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>
#include <QRegularExpression>

UpdateChecker::UpdateChecker(QString currentVersion, QObject *parent)
    : QObject(parent)
    , m_currentVersion(std::move(currentVersion)) {}

void UpdateChecker::checkForUpdates() {
    QUrl url(QString("https://api.github.com/repos/%1/releases/latest").arg(m_repoPath));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "VideoMergerPro-App");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_reply = m_netManager.get(request);
    connect(m_reply, &QNetworkReply::finished, this, &UpdateChecker::onCheckFinished);
}

bool UpdateChecker::isNewerVersion(const QString &latestTag, const QString &currentTag) {
    auto clean = [](QString s) {
        if (s.startsWith('v', Qt::CaseInsensitive)) s.remove(0, 1);
        return s;
    };

    QStringList lParts = clean(latestTag).split('.');
    QStringList cParts = clean(currentTag).split('.');

    while (lParts.size() < 3) lParts << "0";
    while (cParts.size() < 3) cParts << "0";

    for (int i = 0; i < 3; ++i) {
        int lNum = lParts[i].toInt();
        int cNum = cParts[i].toInt();
        if (lNum > cNum) return true;
        if (lNum < cNum) return false;
    }
    return false;
}

void UpdateChecker::onCheckFinished() {
    if (!m_reply) return;

    if (m_reply->error() == QNetworkReply::NoError) {
        QByteArray data = m_reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            QString tagName = obj["tag_name"].toString();
            QString body = obj["body"].toString();
            QJsonArray assets = obj["assets"].toArray();

            QString setupUrl;
            for (const auto &v : assets) {
                QJsonObject asset = v.toObject();
                QString name = asset["name"].toString();
                if (name.endsWith(".exe", Qt::CaseInsensitive)) {
                    setupUrl = asset["browser_download_url"].toString();
                    break;
                }
            }

            if (!setupUrl.isEmpty() && isNewerVersion(tagName, m_currentVersion)) {
                emit updateAvailable(tagName, body, setupUrl);
            }
        }
    }

    m_reply->deleteLater();
    m_reply = nullptr;
}

void UpdateChecker::downloadAndInstallUpdate(const QString &downloadUrl, const QString &fileName) {
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_installerFile.setFileName(tempDir + "/" + fileName);

    if (!m_installerFile.open(QIODevice::WriteOnly)) {
        emit downloadFinished(false, "Could not open temporary installer path: " + m_installerFile.errorString());
        return;
    }

    // Using uniform brace initialization avoids C++ most vexing parse
    QNetworkRequest request{QUrl(downloadUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "VideoMergerPro-App");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_reply = m_netManager.get(request);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &UpdateChecker::onDownloadProgress);
    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        if (m_reply) {
            m_installerFile.write(m_reply->readAll());
        }
    });
    connect(m_reply, &QNetworkReply::finished, this, &UpdateChecker::onDownloadFinished);
}

void UpdateChecker::cancelDownload() {
    if (m_reply && m_reply->isRunning()) {
        m_reply->abort();
    }
}

void UpdateChecker::onDownloadProgress(qint64 received, qint64 total) {
    emit downloadProgress(received, total);
}

void UpdateChecker::onDownloadFinished() {
    if (!m_reply) return;

    m_installerFile.close();

    if (m_reply->error() != QNetworkReply::NoError) {
        QString err = m_reply->errorString();
        m_reply->deleteLater();
        m_reply = nullptr;
        m_installerFile.remove();
        emit downloadFinished(false, "Download failed: " + err);
        return;
    }

    m_reply->deleteLater();
    m_reply = nullptr;

    emit downloadFinished(true, QString());

    QString installerPath = QDir::toNativeSeparators(m_installerFile.fileName());
    QProcess::startDetached(installerPath, {});

    QCoreApplication::quit();
}