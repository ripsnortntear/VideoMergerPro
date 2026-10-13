#pragma once

#include <QObject>
#include <QStringList>
#include <QProcess>

class MergeEngine : public QObject {
    Q_OBJECT
public:
    enum class Preset {
        Fast,        // CRF 23, preset fast
        HighQuality, // CRF 18, preset slow
        Draft        // CRF 28, preset veryfast
    };

    explicit MergeEngine(QString ffmpegPath, QString ffprobePath, QObject *parent = nullptr);
    ~MergeEngine() override;

    void startMerge(const QStringList &inputFiles, const QString &outputFile, Preset preset = Preset::Fast);
    void cancel();

signals:
    void statusUpdated(const QString &status);
    void progressUpdated(int percent);
    void mergeFinished(bool success, const QString &errorMessage);

private:
    void runProbeStep();
    void executeStreamCopy();
    void executeTranscode();
    void parseProgress(const QString &line);

    QString m_ffmpegPath;
    QString m_ffprobePath;
    QStringList m_inputFiles;
    QString m_outputFile;
    Preset m_preset{Preset::Fast};

    QProcess *m_process{nullptr};
    qint64 m_totalDurationMs{0};
    bool m_cancelled{false};
};