#include "MergeEngine.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

MergeEngine::MergeEngine(QString ffmpegPath, QString ffprobePath, QObject *parent)
    : QObject(parent)
    , m_ffmpegPath(std::move(ffmpegPath))
    , m_ffprobePath(std::move(ffprobePath)) {}

MergeEngine::~MergeEngine() {
    cancel();
}

void MergeEngine::startMerge(const QStringList &inputFiles, const QString &outputFile, Preset preset) {
    m_inputFiles = inputFiles;
    m_outputFile = outputFile;
    m_preset = preset;
    m_cancelled = false;
    m_totalDurationMs = 0;

    emit statusUpdated("Analyzing input clips...");
    emit progressUpdated(0);

    runProbeStep();
}

void MergeEngine::cancel() {
    m_cancelled = true;
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

void MergeEngine::runProbeStep() {
    m_totalDurationMs = 0;
    for (const auto &file : m_inputFiles) {
        QProcess probe;
        QStringList args = {
            "-v", "error",
            "-show_entries", "format=duration",
            "-of", "default=noprint_wrappers=1:nokey=1",
            file
        };
        probe.start(m_ffprobePath, args);
        if (probe.waitForFinished(5000)) {
            double sec = probe.readAllStandardOutput().trimmed().toDouble();
            m_totalDurationMs += static_cast<qint64>(sec * 1000.0);
        }
    }

    executeStreamCopy();
}

void MergeEngine::executeStreamCopy() {
    if (m_cancelled) return;

    emit statusUpdated("Attempting lossless stream-copy merge...");

    QString listFilePath = QDir::tempPath() + "/concat_list.txt";
    QFile listFile(listFilePath);
    if (!listFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        executeTranscode();
        return;
    }

    QTextStream out(&listFile);
    for (const auto &f : m_inputFiles) {
        QString escaped = f;
        escaped.replace("'", "'\\''");
        out << "file '" << escaped << "'\n";
    }
    listFile.close();

    if (m_process) m_process->deleteLater();
    m_process = new QProcess(this);

    QStringList args = {
        "-y",
        "-f", "concat",
        "-safe", "0",
        "-i", listFilePath,
        "-c", "copy",
        "-movflags", "+faststart",
        m_outputFile
    };

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, listFilePath](int exitCode, QProcess::ExitStatus exitStatus) {
        QFile::remove(listFilePath);

        if (m_cancelled) {
            emit mergeFinished(false, "Merge aborted by user.");
            return;
        }

        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            emit progressUpdated(100);
            emit mergeFinished(true, QString());
        } else {
            emit statusUpdated("Codecs vary across clips. Transcoding to match...");
            executeTranscode();
        }
    });

    m_process->start(m_ffmpegPath, args);
}

void MergeEngine::executeTranscode() {
    if (m_cancelled) return;

    if (m_process) m_process->deleteLater();
    m_process = new QProcess(this);

    QString crfVal = "23";
    QString presetVal = "fast";

    switch (m_preset) {
        case Preset::HighQuality:
            crfVal = "18";
            presetVal = "slow";
            break;
        case Preset::Draft:
            crfVal = "28";
            presetVal = "veryfast";
            break;
        case Preset::Fast:
        default:
            crfVal = "23";
            presetVal = "fast";
            break;
    }

    QStringList args = {"-y"};
    QString filterComplex;

    for (int i = 0; i < m_inputFiles.size(); ++i) {
        args << "-i" << m_inputFiles[i];
        filterComplex += QString("[%1:v]scale=1920:1080:force_original_aspect_ratio=decrease,"
                                 "pad=1920:1080:(ow-iw)/2:(oh-ih)/2,setsar=1[v%1];"
                                 "[%1:a]aformat=sample_rates=48000:channel_layouts=stereo[a%1];").arg(i);
    }

    for (int i = 0; i < m_inputFiles.size(); ++i) {
        filterComplex += QString("[v%1][a%1]").arg(i);
    }
    filterComplex += QString("concat=n=%1:v=1:a=1[outv][outa]").arg(m_inputFiles.size());

    args << "-filter_complex" << filterComplex
         << "-map" << "[outv]"
         << "-map" << "[outa]"
         << "-c:v" << "libx264"
         << "-preset" << presetVal
         << "-crf" << crfVal
         << "-c:a" << "aac"
         << "-b:a" << "192k"
         << "-movflags" << "+faststart"
         << m_outputFile;

    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        while (m_process && m_process->canReadLine()) {
            parseProgress(QString::fromUtf8(m_process->readLine()));
        }
    });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        if (m_cancelled) {
            emit mergeFinished(false, "Merge aborted by user.");
            return;
        }

        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            emit progressUpdated(100);
            emit mergeFinished(true, QString());
        } else {
            emit mergeFinished(false, "Transcode failed. Exit code: " + QString::number(exitCode));
        }
    });

    m_process->start(m_ffmpegPath, args);
}

void MergeEngine::parseProgress(const QString &line) {
    static QRegularExpression re(R"(time=(\d+):(\d+):(\d+\.\d+))");
    auto match = re.match(line);
    if (match.hasMatch() && m_totalDurationMs > 0) {
        qint64 hours = match.captured(1).toLongLong();
        qint64 mins = match.captured(2).toLongLong();
        double secs = match.captured(3).toDouble();
        qint64 currentMs = (hours * 3600000) + (mins * 60000) + static_cast<qint64>(secs * 1000.0);

        int pct = qBound(0, static_cast<int>((currentMs * 100) / m_totalDurationMs), 99);
        emit progressUpdated(pct);
    }
}