#pragma once

#include <QMainWindow>
#include <QListView>
#include <QPushButton>
#include <QLineEdit>
#include <QProgressBar>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QItemSelection>

#include "VideoListModel.h"
#include "FFmpegManager.h"
#include "MergeEngine.h"
#include "UpdateChecker.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onAddFilesClicked();
    void onAddFolderClicked();
    void onRemoveSelectedClicked();
    void onClearAllClicked();
    void onBrowseOutputClicked();
    void onStartMergeClicked();
    void onCancelClicked();
    void onPlayPauseClicked();
    void onMediaPositionChanged(qint64 position);
    void onMediaDurationChanged(qint64 duration);
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void updateQueueSummary();
    void onCheckForUpdatesTriggered();
    void onAboutTriggered();
    void onUpdateAvailable(const QString &newVersion, const QString &releaseNotes, const QString &downloadUrl);

private:
    void setupUi();
    void setupMenuBar();
    void applyStyles();
    void verifyOrAcquireFFmpeg();

    VideoListModel *m_model{nullptr};
    QMediaPlayer *m_player{nullptr};
    QAudioOutput *m_audioOutput{nullptr};
    QVideoWidget *m_videoWidget{nullptr};
    FFmpegManager *m_ffmpegManager{nullptr};
    MergeEngine *m_mergeEngine{nullptr};
    UpdateChecker *m_updateChecker{nullptr};

    QListView *m_listView{nullptr};
    QLabel *m_queueSummaryLabel{nullptr};
    QPushButton *m_btnAdd{nullptr};
    QPushButton *m_btnAddFolder{nullptr};
    QPushButton *m_btnRemove{nullptr};
    QPushButton *m_btnClear{nullptr};

    QPushButton *m_btnPlayPause{nullptr};
    QSlider *m_seekSlider{nullptr};
    QLabel *m_timeLabel{nullptr};

    QLineEdit *m_editOutputPath{nullptr};
    QPushButton *m_btnBrowseOutput{nullptr};

    QComboBox *m_comboPreset{nullptr};
    QProgressBar *m_progressBar{nullptr};
    QPushButton *m_btnStartMerge{nullptr};
    QPushButton *m_btnCancel{nullptr};
    QLabel *m_statusLabel{nullptr};
};