#pragma once

#include <QObject>
#include <QStringList>
#include <QProcess>
#include <QTemporaryFile>

struct StreamFingerprint {
    QString codecName;
    int width{0};
    int height{0};
    QString pixelFormat;
    QString sampleRate;
    int channels{0};
    bool isValid{false};
};

class MergeEngine : public QObject {
    Q_OBJECT
public:
    explicit MergeEngine(QString ffmpegPath, QString ffprobePath, QObject *parent = nullptr);
    ~MergeEngine() override;

    void startMerge(const QStringList &inputFiles, const QString &outputPath);
    void cancel();

signals:
    void statusUpdated(const QString &status);
    void progressUpdated(int percentage);
    void mergeFinished(bool success, const QString &errorMessage);

private:
    void runProbeStep();
    StreamFingerprint probeFile(const QString &filePath);
    bool checkStreamsHomogeneous(const QList<StreamFingerprint> &fingerprints);
    void executeStreamCopy();
    void executeTranscodeMerge();
    void parseFFmpegProgress(const QString &line);
    qint64 parseDurationToMs(const QString &timeStr);

    QString m_ffmpegPath;
    QString m_ffprobePath;
    QStringList m_inputFiles;
    QString m_outputPath;

    QProcess *m_process{nullptr};
    QTemporaryFile m_concatScriptFile;
    qint64 m_totalDurationMs{0};
    bool m_isStreamCopy{false};
    bool m_canceled{false};
};