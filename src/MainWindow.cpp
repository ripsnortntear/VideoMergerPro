#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QTime>
#include <QProgressDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_model(new VideoListModel(this))
    , m_player(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_ffmpegManager(new FFmpegManager(this)) {
    setupUi();
    applyStyles();
    setAcceptDrops(true);

    m_player->setAudioOutput(m_audioOutput);
    m_player->setVideoOutput(m_videoWidget);

    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::onMediaPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MainWindow::onMediaDurationChanged);

    verifyOrAcquireFFmpeg();

//  UPDATE THE VERSION NUMBER  #######################################	
	m_updateChecker = new UpdateChecker("1.1.0", this); // Keep in sync with your releases
    connect(m_updateChecker, &UpdateChecker::updateAvailable, this, &MainWindow::onUpdateAvailable);
//  UPDATE THE VERSION NUMBER  #######################################	

    // Check quietly in background on launch
    m_updateChecker->checkForUpdates();
}

MainWindow::~MainWindow() {
    if (m_player) {
        m_player->stop();
    }
}

void MainWindow::setupUi() {
    setWindowTitle("Video Merger Pro (Qt 6 / C++20 / FFmpeg)");
    setWindowIcon(QIcon("app_icon.ico"));
    resize(1024, 720);

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    auto *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(16);

    // Left: Reorderable List & Controls
    auto *leftLayout = new QVBoxLayout();
    auto *listLabel = new QLabel("Source Video Clips (Drag to reorder):", this);
    listLabel->setStyleSheet("font-weight: bold; font-size: 13px;");

    m_listView = new QListView(this);
    m_listView->setModel(m_model);
    m_listView->setDragEnabled(true);
    m_listView->setAcceptDrops(true);
    m_listView->setDefaultDropAction(Qt::MoveAction);
    m_listView->setDragDropMode(QAbstractItemView::InternalMove);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged);

    auto *listBtnLayout = new QHBoxLayout();
    m_btnAdd = new QPushButton("Add Videos...", this);
    m_btnAddFolder = new QPushButton("Add Folder...", this);
    m_btnRemove = new QPushButton("Remove", this);
    m_btnClear = new QPushButton("Clear All", this);

    connect(m_btnAdd, &QPushButton::clicked, this, &MainWindow::onAddFilesClicked);
    connect(m_btnAddFolder, &QPushButton::clicked, this, &MainWindow::onAddFolderClicked);
    connect(m_btnRemove, &QPushButton::clicked, this, &MainWindow::onRemoveSelectedClicked);
    connect(m_btnClear, &QPushButton::clicked, this, &MainWindow::onClearAllClicked);

    listBtnLayout->addWidget(m_btnAdd);
    listBtnLayout->addWidget(m_btnAddFolder);
    listBtnLayout->addWidget(m_btnRemove);
    listBtnLayout->addWidget(m_btnClear);

    leftLayout->addWidget(listLabel);
    leftLayout->addWidget(m_listView);
    leftLayout->addLayout(listBtnLayout);

    // Right: Preview Player
    auto *rightLayout = new QVBoxLayout();
    auto *previewLabel = new QLabel("Clip Preview:", this);
    previewLabel->setStyleSheet("font-weight: bold; font-size: 13px;");

    m_videoWidget = new QVideoWidget(this);
    m_videoWidget->setMinimumSize(440, 280);
    m_videoWidget->setStyleSheet("background-color: #000; border-radius: 4px;");

    auto *playControlLayout = new QHBoxLayout();
    m_btnPlayPause = new QPushButton("Play", this);
    m_seekSlider = new QSlider(Qt::Horizontal, this);
    m_timeLabel = new QLabel("00:00 / 00:00", this);

    connect(m_btnPlayPause, &QPushButton::clicked, this, &MainWindow::onPlayPauseClicked);
    connect(m_seekSlider, &QSlider::sliderMoved, m_player, &QMediaPlayer::setPosition);

    playControlLayout->addWidget(m_btnPlayPause);
    playControlLayout->addWidget(m_seekSlider);
    playControlLayout->addWidget(m_timeLabel);

    rightLayout->addWidget(previewLabel);
    rightLayout->addWidget(m_videoWidget, 1);
    rightLayout->addLayout(playControlLayout);

    contentLayout->addLayout(leftLayout, 5);
    contentLayout->addLayout(rightLayout, 6);
    rootLayout->addLayout(contentLayout, 1);

    // Bottom Area: Destination & Execution
    auto *destLayout = new QHBoxLayout();
    auto *destLabel = new QLabel("Output File:", this);
    destLabel->setStyleSheet("font-weight: bold;");
    m_editOutputPath = new QLineEdit(this);
    m_editOutputPath->setPlaceholderText("Select output file path (*.mp4)...");
    m_btnBrowseOutput = new QPushButton("Browse...", this);

    connect(m_btnBrowseOutput, &QPushButton::clicked, this, &MainWindow::onBrowseOutputClicked);

    destLayout->addWidget(destLabel);
    destLayout->addWidget(m_editOutputPath);
    destLayout->addWidget(m_btnBrowseOutput);
    rootLayout->addLayout(destLayout);

    auto *actionLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);

    m_btnStartMerge = new QPushButton("Merge Videos", this);
    m_btnStartMerge->setStyleSheet("background-color: #0078D4; color: white; font-weight: bold; padding: 8px 18px;");
    m_btnCancel = new QPushButton("Cancel", this);
    m_btnCancel->setEnabled(false);

    connect(m_btnStartMerge, &QPushButton::clicked, this, &MainWindow::onStartMergeClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &MainWindow::onCancelClicked);

    actionLayout->addWidget(m_progressBar, 1);
    actionLayout->addWidget(m_btnStartMerge);
    actionLayout->addWidget(m_btnCancel);
    rootLayout->addLayout(actionLayout);

    m_statusLabel = new QLabel("Ready.", this);
    m_statusLabel->setStyleSheet("color: #666;");
    rootLayout->addWidget(m_statusLabel);
}

void MainWindow::applyStyles() {
    setStyleSheet(R"(
        /* Global Window & Labels */
        QMainWindow { 
            background-color: #F8F9FA; 
        }
        QLabel {
            color: #212529;
        }

        /* Dialogs & Popups */
        QMessageBox {
            background-color: #FFFFFF;
        }
        QMessageBox QLabel {
            color: #212529;
            background-color: transparent;
            font-size: 13px;
        }
        QMessageBox QPushButton {
            border: 1px solid #CED4DA;
            background-color: #FFFFFF;
            color: #212529;
            padding: 6px 18px;
            border-radius: 4px;
            min-width: 70px;
        }
        QMessageBox QPushButton:hover {
            background-color: #E9ECEF;
        }

        /* Source Video List */
        QListView {
            background-color: #FFFFFF;
            color: #212529;
            border: 1px solid #CED4DA;
            border-radius: 4px;
            padding: 4px;
        }
        QListView::item {
            color: #212529;
            padding: 6px;
            border-bottom: 1px solid #E9ECEF;
        }
        QListView::item:selected {
            background-color: #0D6EFD;
            color: #FFFFFF;
        }

        /* Buttons */
        QPushButton {
            border: 1px solid #CED4DA;
            background-color: #FFFFFF;
            color: #212529;
            padding: 6px 14px;
            border-radius: 4px;
            font-weight: 500;
        }
        QPushButton:hover { 
            background-color: #E9ECEF; 
        }
        QPushButton:pressed {
            background-color: #DEE2E6;
        }
        QPushButton:disabled { 
            background-color: #E9ECEF; 
            color: #ADB5BD; 
            border-color: #E9ECEF;
        }

        /* Text Input */
        QLineEdit {
            border: 1px solid #CED4DA;
            background-color: #FFFFFF;
            color: #212529;
            padding: 6px 8px;
            border-radius: 4px;
            selection-background-color: #0D6EFD;
            selection-color: #FFFFFF;
        }

        /* Progress Bar */
        QProgressBar {
            border: 1px solid #CED4DA;
            border-radius: 4px;
            text-align: center;
            background-color: #E9ECEF;
            color: #212529;
            font-weight: bold;
        }
        QProgressBar::chunk {
            background-color: #0D6EFD;
            border-radius: 3px;
        }

        /* Seek Slider */
        QSlider::groove:horizontal {
            height: 6px;
            background: #CED4DA;
            border-radius: 3px;
        }
        QSlider::sub-page:horizontal {
            background: #0D6EFD;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #0D6EFD;
            border: 1px solid #0B5ED7;
            width: 14px;
            margin-top: -4px;
            margin-bottom: -4px;
            border-radius: 7px;
        }
    )");
}

void MainWindow::verifyOrAcquireFFmpeg() {
    if (m_ffmpegManager->checkFFmpegAvailability()) {
        m_statusLabel->setText("FFmpeg detected: " + m_ffmpegManager->getFFmpegPath());
        return;
    }

    auto reply = QMessageBox::question(
        this,
        "FFmpeg Required",
        "FFmpeg is not detected on your system.\n\n"
        "Would you like the application to automatically download and configure a "
        "compatible 64-bit Windows FFmpeg build from official sources?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
        m_statusLabel->setText("Warning: FFmpeg is missing. Merging is disabled until available.");
        m_btnStartMerge->setEnabled(false);
        return;
    }

    auto *dlg = new QProgressDialog("Downloading FFmpeg build...", "Cancel", 0, 100, this);
    dlg->setWindowModality(Qt::WindowModal);
    dlg->setAutoClose(true);

    connect(m_ffmpegManager, &FFmpegManager::downloadProgress, dlg, [dlg](qint64 received, qint64 total) {
        if (total > 0) {
            dlg->setValue(static_cast<int>((received * 100) / total));
        }
    });

    connect(m_ffmpegManager, &FFmpegManager::downloadStatus, dlg, [dlg](const QString &msg) {
        dlg->setLabelText(msg);
    });

    connect(dlg, &QProgressDialog::canceled, m_ffmpegManager, &FFmpegManager::cancelDownload);

    connect(m_ffmpegManager, &FFmpegManager::downloadFinished, this, [this, dlg](bool ok, const QString &err) {
        dlg->close();
        dlg->deleteLater();
        if (ok) {
            QMessageBox::information(this, "Success", "FFmpeg configured successfully.");
            m_statusLabel->setText("FFmpeg detected: " + m_ffmpegManager->getFFmpegPath());
            m_btnStartMerge->setEnabled(true);
        } else {
            QMessageBox::critical(this, "Setup Failed", "Could not set up FFmpeg:\n" + err);
            m_statusLabel->setText("Error: FFmpeg configuration failed.");
            m_btnStartMerge->setEnabled(false);
        }
    });

    m_ffmpegManager->downloadAndSetupFFmpeg();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    QStringList paths;
    for (const auto &url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            paths.append(url.toLocalFile());
        }
    }
    if (!paths.isEmpty()) {
        m_model->addFiles(paths);
        event->acceptProposedAction();
    }
}

void MainWindow::onAddFilesClicked() {
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "Select Video Files",
        QString(),
        "Video Files (*.mp4 *.mkv *.avi *.mov *.ts *.flv *.webm *.wmv)"
    );
    if (!files.isEmpty()) {
        m_model->addFiles(files);
    }
}

void MainWindow::onAddFolderClicked() {
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Folder Containing Videos",
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (!dir.isEmpty()) {
        m_model->addFiles({dir});
    }
}

void MainWindow::onRemoveSelectedClicked() {
    auto idx = m_listView->currentIndex();
    if (idx.isValid()) {
        m_player->stop();
        m_model->removeAt(idx.row());
    }
}

void MainWindow::onClearAllClicked() {
    m_player->stop();
    m_model->clear();
}

void MainWindow::onBrowseOutputClicked() {
    QString out = QFileDialog::getSaveFileName(
        this,
        "Select Destination Video",
        QString(),
        "MP4 Video (*.mp4)"
    );
    if (!out.isEmpty()) {
        if (!out.endsWith(".mp4", Qt::CaseInsensitive)) {
            out += ".mp4";
        }
        m_editOutputPath->setText(out);
    }
}

void MainWindow::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    Q_UNUSED(deselected);
    if (selected.indexes().isEmpty()) return;

    int row = selected.indexes().first().row();
    QString path = m_model->getItem(row).filePath;

    m_player->stop();
    m_player->setSource(QUrl::fromLocalFile(path));
    m_btnPlayPause->setText("Play");
}

void MainWindow::onPlayPauseClicked() {
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
        m_btnPlayPause->setText("Play");
    } else {
        m_player->play();
        m_btnPlayPause->setText("Pause");
    }
}

void MainWindow::onUpdateAvailable(const QString &newVersion, const QString &releaseNotes, const QString &downloadUrl) {
    QString msg = QString("A new version (%1) of Video Merger Pro is available!\n\n"
                          "Release Notes:\n%2\n\n"
                          "Would you like to download and install this update now?")
                          .arg(newVersion, releaseNotes.isEmpty() ? "Bug fixes and improvements." : releaseNotes);

    auto reply = QMessageBox::question(this, "Update Available", msg, QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }

    auto *dlg = new QProgressDialog("Downloading update...", "Cancel", 0, 100, this);
    dlg->setWindowModality(Qt::WindowModal);
    dlg->setAutoClose(true);

    connect(m_updateChecker, &UpdateChecker::downloadProgress, dlg, [dlg](qint64 received, qint64 total) {
        if (total > 0) {
            dlg->setValue(static_cast<int>((received * 100) / total));
        }
    });

    connect(dlg, &QProgressDialog::canceled, m_updateChecker, &UpdateChecker::cancelDownload);

    connect(m_updateChecker, &UpdateChecker::downloadFinished, this, [this, dlg](bool ok, const QString &err) {
        dlg->close();
        dlg->deleteLater();
        if (!ok && !err.isEmpty()) {
            QMessageBox::warning(this, "Update Error", err);
        }
    });

    m_updateChecker->downloadAndInstallUpdate(downloadUrl, QString("VideoMergerPro_Setup_%1.exe").arg(newVersion));
}

void MainWindow::onMediaPositionChanged(qint64 position) {
    if (!m_seekSlider->isSliderDown()) {
        m_seekSlider->setValue(static_cast<int>(position));
    }

    auto formatTime = [](qint64 ms) {
        QTime t(0, 0, 0);
        t = t.addMSecs(static_cast<int>(ms));
        return t.toString(ms >= 3600000 ? "hh:mm:ss" : "mm:ss");
    };

    m_timeLabel->setText(QString("%1 / %2")
        .arg(formatTime(position))
        .arg(formatTime(m_player->duration())));
}

void MainWindow::onMediaDurationChanged(qint64 duration) {
    m_seekSlider->setRange(0, static_cast<int>(duration));
}

void MainWindow::onStartMergeClicked() {
    QStringList files = m_model->getFilePaths();
    if (files.size() < 2) {
        QMessageBox::warning(this, "Insufficient Files", "Please add at least two videos to merge.");
        return;
    }

    QString outPath = m_editOutputPath->text().trimmed();
    if (outPath.isEmpty()) {
        QMessageBox::warning(this, "Output Path Missing", "Please specify an output file destination.");
        return;
    }

    if (QFileInfo::exists(outPath)) {
        auto ans = QMessageBox::question(
            this,
            "Overwrite File?",
            QString("The output file already exists:\n%1\n\nDo you want to overwrite it?").arg(outPath),
            QMessageBox::Yes | QMessageBox::No
        );
        if (ans != QMessageBox::Yes) {
            return;
        }
    }

    m_player->stop();

    m_btnStartMerge->setEnabled(false);
    m_btnCancel->setEnabled(true);
    m_btnAdd->setEnabled(false);
    m_btnAddFolder->setEnabled(false);
    m_btnRemove->setEnabled(false);
    m_btnClear->setEnabled(false);
    m_progressBar->setValue(0);

    if (m_mergeEngine) {
        m_mergeEngine->deleteLater();
    }

    m_mergeEngine = new MergeEngine(
        m_ffmpegManager->getFFmpegPath(),
        m_ffmpegManager->getFFprobePath(),
        this
    );

    connect(m_mergeEngine, &MergeEngine::statusUpdated, this, [this](const QString &msg) {
        m_statusLabel->setText(msg);
    });

    connect(m_mergeEngine, &MergeEngine::progressUpdated, this, [this](int pct) {
        m_progressBar->setValue(pct);
    });

    connect(m_mergeEngine, &MergeEngine::mergeFinished, this, [this](bool success, const QString &err) {
        m_btnStartMerge->setEnabled(true);
        m_btnCancel->setEnabled(false);
        m_btnAdd->setEnabled(true);
        m_btnAddFolder->setEnabled(true);
        m_btnRemove->setEnabled(true);
        m_btnClear->setEnabled(true);

        if (success) {
            m_statusLabel->setText("Merge operation complete.");
            QMessageBox::information(this, "Done", "Video files were successfully merged!");
        } else {
            m_statusLabel->setText("Process stopped: " + err);
            QMessageBox::critical(this, "Merge Error", err);
        }
    });

    m_mergeEngine->startMerge(files, outPath);
}

void MainWindow::onCancelClicked() {
    if (m_mergeEngine) {
        m_statusLabel->setText("Aborting merge...");
        m_mergeEngine->cancel();
    }
}