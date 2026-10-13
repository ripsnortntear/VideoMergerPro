#pragma once

#include <QMainWindow>
#include <QListView>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QLineEdit>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QSlider>
#include "VideoListModel.h"
#include "FFmpegManager.h"
#include "MergeEngine.h"

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
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onPlayPauseClicked();
    void onMediaPositionChanged(qint64 position);
    void onMediaDurationChanged(qint64 duration);

private:
    void setupUi();
    void applyStyles();
    void verifyOrAcquireFFmpeg();

    VideoListModel *m_model{nullptr};
    QListView *m_listView{nullptr};
    QPushButton *m_btnAdd{nullptr};
	QPushButton *m_btnAddFolder{nullptr};
    QPushButton *m_btnRemove{nullptr};
    QPushButton *m_btnClear{nullptr};

    // Preview
    QMediaPlayer *m_player{nullptr};
    QAudioOutput *m_audioOutput{nullptr};
    QVideoWidget *m_videoWidget{nullptr};
    QPushButton *m_btnPlayPause{nullptr};
    QSlider *m_seekSlider{nullptr};
    QLabel *m_timeLabel{nullptr};

    // Output & Processing
    QLineEdit *m_editOutputPath{nullptr};
    QPushButton *m_btnBrowseOutput{nullptr};
    QPushButton *m_btnStartMerge{nullptr};
    QPushButton *m_btnCancel{nullptr};
    QProgressBar *m_progressBar{nullptr};
    QLabel *m_statusLabel{nullptr};

    FFmpegManager *m_ffmpegManager{nullptr};
    MergeEngine *m_mergeEngine{nullptr};
};