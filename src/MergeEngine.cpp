#include "MergeEngine.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>

MergeEngine::MergeEngine(QString ffmpegPath, QString ffprobePath, QObject *parent)
    : QObject(parent)
    , m_ffmpegPath(std::move(ffmpegPath))
    , m_ffprobePath(std::move(ffprobePath)) {}

MergeEngine::~MergeEngine() {
    cancel();
}

void MergeEngine::cancel() {
    m_canceled = true;
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

void MergeEngine::startMerge(const QStringList &inputFiles, const QString &outputPath) {
    m_inputFiles = inputFiles;
    m_outputPath = outputPath;
    m_canceled = false;
    m_totalDurationMs = 0;

    if (m_inputFiles.size() < 2) {
        emit mergeFinished(false, "At least two video files are required to merge.");
        return;
    }

    emit statusUpdated("Probing video streams and codec properties...");
    runProbeStep();
}

StreamFingerprint MergeEngine::probeFile(const QString &filePath) {
    StreamFingerprint fp;
    QProcess probeProc;
    QStringList args = {
        "-v", "quiet",
        "-print_format", "json",
        "-show_format",
        "-show_streams",
        filePath
    };

    probeProc.start(m_ffprobePath, args);
    if (!probeProc.waitForFinished(15000)) {
        probeProc.kill();
        return fp;
    }

    QByteArray out = probeProc.readAllStandardOutput();
    QJsonDocument doc = QJsonDocument::fromJson(out);
    if (!doc.isObject()) return fp;

    QJsonObject root = doc.object();
    QJsonArray streams = root["streams"].toArray();

    for (const auto &v : streams) {
        QJsonObject s = v.toObject();
        QString type = s["codec_type"].toString();
        if (type == "video" && fp.codecName.isEmpty()) {
            fp.codecName = s["codec_name"].toString();
            fp.width = s["width"].toInt();
            fp.height = s["height"].toInt();
            fp.pixelFormat = s["pix_fmt"].toString();
        } else if (type == "audio" && fp.sampleRate.isEmpty()) {
            fp.sampleRate = s["sample_rate"].toString();
            fp.channels = s["channels"].toInt();
        }
    }

    QJsonObject fmt = root["format"].toObject();
    double durSec = fmt["duration"].toString().toDouble();
    if (durSec > 0.0) {
        m_totalDurationMs += static_cast<qint64>(durSec * 1000.0);
    }

    fp.isValid = (!fp.codecName.isEmpty());
    return fp;
}

bool MergeEngine::checkStreamsHomogeneous(const QList<StreamFingerprint> &fingerprints) {
    if (fingerprints.isEmpty()) return false;
    const auto &base = fingerprints.first();

    for (int i = 1; i < fingerprints.size(); ++i) {
        const auto &f = fingerprints[i];
        if (!f.isValid) return false;
        if (f.codecName != base.codecName) return false;
        if (f.width != base.width || f.height != base.height) return false;
        if (f.pixelFormat != base.pixelFormat) return false;
        if (f.sampleRate != base.sampleRate) return false;
        if (f.channels != base.channels) return false;
    }
    return true;
}

void MergeEngine::runProbeStep() {
    QList<StreamFingerprint> fps;
    for (const auto &f : m_inputFiles) {
        if (m_canceled) return;
        fps.append(probeFile(f));
    }

    bool allMp4 = true;
    for (const auto &p : m_inputFiles) {
        if (QFileInfo(p).suffix().toLower() != "mp4") {
            allMp4 = false;
            break;
        }
    }

    bool streamsMatch = checkStreamsHomogeneous(fps);

    if (allMp4 && streamsMatch) {
        m_isStreamCopy = true;
        emit statusUpdated("Streams are compatible. Performing high-speed stream copy merge...");
        executeStreamCopy();
    } else {
        m_isStreamCopy = false;
        emit statusUpdated("Streams or formats differ. Transcoding and standardizing to final MP4...");
        executeTranscodeMerge();
    }
}

void MergeEngine::executeStreamCopy() {
    if (!m_concatScriptFile.open()) {
        emit mergeFinished(false, "Failed to create temporary FFmpeg concat file.");
        return;
    }

    QTextStream out(&m_concatScriptFile);
    out.setEncoding(QStringConverter::Utf8);
    for (const auto &path : m_inputFiles) {
        // Concat demuxer script requires escaping single quotes
        QString formatted = path;
        formatted.replace("'", "'\\''");
        out << "file '" << formatted << "'\n";
    }
    m_concatScriptFile.close();

    QStringList args = {
        "-y",
        "-f", "concat",
        "-safe", "0",
        "-i", m_concatScriptFile.fileName(),
        "-c", "copy",
        "-movflags", "+faststart",
        m_outputPath
    };

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        while (m_process && m_process->canReadLine()) {
            parseFFmpegProgress(QString::fromUtf8(m_process->readLine()));
        }
    });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        m_concatScriptFile.remove();
        if (m_canceled) {
            QFile::remove(m_outputPath);
            emit mergeFinished(false, "Operation aborted by user.");
            return;
        }
        if (status == QProcess::NormalExit && exitCode == 0) {
            emit progressUpdated(100);
            emit mergeFinished(true, QString());
        } else {
            QFile::remove(m_outputPath);
            emit mergeFinished(false, QString("FFmpeg process terminated with error code %1").arg(exitCode));
        }
    });

    m_process->start(m_ffmpegPath, args);
}

void MergeEngine::executeTranscodeMerge() {
    QStringList args;
    args << "-y";

    for (const auto &file : m_inputFiles) {
        args << "-i" << file;
    }

    // Build filter_complex string
    QString filter;
    for (int i = 0; i < m_inputFiles.size(); ++i) {
        filter += QString("[%1:v:0]scale=1920:1080:force_original_aspect_ratio=decrease,pad=1920:1080:(ow-iw)/2:(oh-ih)/2,setsar=1[v%1];").arg(i);
        filter += QString("[%1:a:0]aformat=sample_rates=48000:channel_layouts=stereo[a%1];").arg(i);
    }

    for (int i = 0; i < m_inputFiles.size(); ++i) {
        filter += QString("[v%1][a%1]").arg(i);
    }
    filter += QString("concat=n=%1:v=1:a=1[v][a]").arg(m_inputFiles.size());

    args << "-filter_complex" << filter;
    args << "-map" << "[v]";
    args << "-map" << "[a]";
    args << "-c:v" << "libx264";
    args << "-preset" << "veryfast";
    args << "-crf" << "22";
    args << "-c:a" << "aac";
    args << "-b:a" << "192k";
    args << "-movflags" << "+faststart";
    args << m_outputPath;

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        while (m_process && m_process->canReadLine()) {
            parseFFmpegProgress(QString::fromUtf8(m_process->readLine()));
        }
    });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        if (m_canceled) {
            QFile::remove(m_outputPath);
            emit mergeFinished(false, "Operation aborted by user.");
            return;
        }
        if (status == QProcess::NormalExit && exitCode == 0) {
            emit progressUpdated(100);
            emit mergeFinished(true, QString());
        } else {
            QFile::remove(m_outputPath);
            emit mergeFinished(false, QString("Encoding process failed with exit code %1").arg(exitCode));
        }
    });

    m_process->start(m_ffmpegPath, args);
}

void MergeEngine::parseFFmpegProgress(const QString &line) {
    static QRegularExpression timeRegex(R"(time=(\d+):(\d+):(\d+\.\d+))");
    auto match = timeRegex.match(line);
    if (match.hasMatch() && m_totalDurationMs > 0) {
        int hours = match.captured(1).toInt();
        int minutes = match.captured(2).toInt();
        double seconds = match.captured(3).toDouble();
        qint64 currentMs = static_cast<qint64>((hours * 3600 + minutes * 60 + seconds) * 1000.0);

        int pct = static_cast<int>((currentMs * 100) / m_totalDurationMs);
        pct = qBound(0, pct, 99);
        emit progressUpdated(pct);
    }
}