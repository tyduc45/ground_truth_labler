#include "MainWindow.h"
#include "VideoWidget.h"
#include "LabelPanel.h"
#include "ResultPanel.h"
#include "ControlBar.h"
#include "core/AnnotationData.h"
#include "core/VideoManager.h"
#include "core/TrackingEngine.h"
#include "core/MotExporter.h"
#include "util/FrameConverter.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QKeyEvent>
#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_data = new AnnotationData(this);
    m_videoManager = new VideoManager(this);
    m_trackingEngine = new TrackingEngine(m_videoManager, this);
    m_playbackTimer = new QTimer(this);

    setupLayout();
    setupMenuBar();
    connectSignals();
    updateButtonStates();

    statusBar()->showMessage(tr("Ready. Open a video file to begin."));
}

MainWindow::~MainWindow() = default;

void MainWindow::setupLayout()
{
    auto* centralWidget = new QWidget;
    auto* mainLayout = new QVBoxLayout(centralWidget);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left panel: labels
    m_labelPanel = new LabelPanel(m_data);
    splitter->addWidget(m_labelPanel);

    // Center: video
    m_videoWidget = new VideoWidget;
    splitter->addWidget(m_videoWidget);

    // Right panel: results
    m_resultPanel = new ResultPanel(m_data);
    splitter->addWidget(m_resultPanel);

    // Set stretch factors: left=0, center=1(stretch), right=0
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);

    mainLayout->addWidget(splitter, 1);

    // Bottom: control bar
    m_controlBar = new ControlBar;
    mainLayout->addWidget(m_controlBar);

    setCentralWidget(centralWidget);
}

void MainWindow::setupMenuBar()
{
    auto* fileMenu = menuBar()->addMenu(tr("&File"));

    auto* openAction = fileMenu->addAction(tr("&Open Video..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenVideo);

    fileMenu->addSeparator();

    auto* exportMotAction = fileMenu->addAction(tr("Export &MOT CSV..."));
    connect(exportMotAction, &QAction::triggered, this, &MainWindow::onExportMotRequested);

    auto* mergeAction = fileMenu->addAction(tr("Merge && Export &Video..."));
    connect(mergeAction, &QAction::triggered, this, &MainWindow::onMergeRequested);

    fileMenu->addSeparator();

    auto* exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::connectSignals()
{
    // Control bar
    connect(m_controlBar, &ControlBar::runClicked, this, &MainWindow::onRun);
    connect(m_controlBar, &ControlBar::stopClicked, this, &MainWindow::onStop);
    connect(m_controlBar, &ControlBar::acceptClicked, this, &MainWindow::onAccept);
    connect(m_controlBar, &ControlBar::undoClicked, this, &MainWindow::onUndo);
    connect(m_controlBar, &ControlBar::frameSliderChanged, this, &MainWindow::onFrameSliderChanged);

    // Tracking engine
    connect(m_trackingEngine, &TrackingEngine::frameTracked,
            this, &MainWindow::onFrameTracked);
    connect(m_trackingEngine, &TrackingEngine::trackingFinished,
            this, &MainWindow::onTrackingFinished);
    connect(m_trackingEngine, &TrackingEngine::trackingError,
            this, [this](const QString& msg) {
                QMessageBox::warning(this, tr("Tracking Error"), msg);
                m_state = STATE_PAUSED;
                updateButtonStates();
            });

    // Result panel
    connect(m_resultPanel, &ResultPanel::segmentDoubleClicked,
            this, &MainWindow::onSegmentDoubleClicked);
    connect(m_resultPanel, &ResultPanel::mergeRequested,
            this, &MainWindow::onMergeRequested);
    connect(m_resultPanel, &ResultPanel::exportMotRequested,
            this, &MainWindow::onExportMotRequested);

    // Video widget box drawn
    connect(m_videoWidget, &VideoWidget::boxDrawn,
            this, &MainWindow::onBoxDrawn);

    // Playback timer
    connect(m_playbackTimer, &QTimer::timeout, this, [this]() {
        if (m_playbackSegmentIndex < 0 ||
            m_playbackSegmentIndex >= static_cast<int>(m_data->segments().size())) {
            m_playbackTimer->stop();
            return;
        }

        const auto& seg = m_data->segments()[m_playbackSegmentIndex];
        if (m_playbackFrame > seg.endFrame) {
            m_playbackTimer->stop();
            statusBar()->showMessage(tr("Playback finished."));
            m_state = STATE_IDLE;
            updateButtonStates();
            return;
        }

        cv::Mat frame = m_videoManager->getFrame(m_playbackFrame);
        m_videoWidget->displayFrame(frame);

        // Find annotation for this frame
        for (const auto& fa : seg.annotations) {
            if (fa.frameIndex == m_playbackFrame) {
                m_videoWidget->setOverlayBoxes(fa.boxes, m_data->labels());
                break;
            }
        }

        m_controlBar->setCurrentFrame(m_playbackFrame);
        m_playbackFrame++;
    });
}

void MainWindow::updateButtonStates()
{
    bool hasBoxes = !m_videoWidget->userDrawnBoxes().empty();

    switch (m_state) {
    case STATE_NO_VIDEO:
        m_controlBar->setRunEnabled(false);
        m_controlBar->setStopEnabled(false);
        m_controlBar->setAcceptEnabled(false);
        m_controlBar->setUndoEnabled(false);
        m_controlBar->setSliderEnabled(false);
        m_videoWidget->setDrawingEnabled(false);
        break;
    case STATE_IDLE:
        m_controlBar->setRunEnabled(hasBoxes);
        m_controlBar->setStopEnabled(false);
        m_controlBar->setAcceptEnabled(false);
        m_controlBar->setUndoEnabled(false);
        m_controlBar->setSliderEnabled(true);
        m_videoWidget->setDrawingEnabled(true);
        break;
    case STATE_TRACKING:
        m_controlBar->setRunEnabled(false);
        m_controlBar->setStopEnabled(true);
        m_controlBar->setAcceptEnabled(false);
        m_controlBar->setUndoEnabled(false);
        m_controlBar->setSliderEnabled(false);
        m_videoWidget->setDrawingEnabled(false);
        break;
    case STATE_PAUSED:
        m_controlBar->setRunEnabled(hasBoxes);
        m_controlBar->setStopEnabled(false);
        m_controlBar->setAcceptEnabled(true);
        m_controlBar->setUndoEnabled(true);
        m_controlBar->setSliderEnabled(false);
        m_videoWidget->setDrawingEnabled(true);
        break;
    }
}

void MainWindow::displayFrameAt(int frameIndex)
{
    cv::Mat frame = m_videoManager->getFrame(frameIndex);
    if (!frame.empty()) {
        m_videoWidget->displayFrame(frame);
        m_controlBar->setCurrentFrame(frameIndex);
    }
}

void MainWindow::onOpenVideo()
{
    QString path = QFileDialog::getOpenFileName(
        this, tr("Open Video"), QString(),
        tr("MP4 Videos (*.mp4);;All Files (*)"));

    if (path.isEmpty()) return;

    if (!m_videoManager->openVideo(path)) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to open video file: %1").arg(path));
        return;
    }

    m_trackingEngine->reset();
    m_data->clearActiveAnnotations();
    m_videoWidget->clearUserBoxes();
    m_videoWidget->clearOverlayBoxes();

    displayFrameAt(0);

    m_controlBar->setFrameRange(0, m_videoManager->totalFrames() - 1);
    m_controlBar->setCurrentFrame(0);

    m_state = STATE_IDLE;
    updateButtonStates();

    statusBar()->showMessage(tr("Opened: %1 (%2 frames, %3 fps)")
                                 .arg(path)
                                 .arg(m_videoManager->totalFrames())
                                 .arg(m_videoManager->fps(), 0, 'f', 1));
}

void MainWindow::onRun()
{
    if (m_state == STATE_PAUSED && !m_videoWidget->userDrawnBoxes().empty()) {
        // User drew new boxes after stopping - re-initialize with new boxes
        // Accept previous work first is handled by user explicitly clicking Accept
    }

    auto userBoxes = m_videoWidget->userDrawnBoxes();
    if (userBoxes.empty()) {
        QMessageBox::information(this, tr("Info"),
                                 tr("Please draw at least one bounding box on the video."));
        return;
    }

    int labelId = m_labelPanel->selectedLabelId();
    if (labelId < 0) {
        QMessageBox::information(this, tr("Info"),
                                 tr("Please add and select a label first."));
        return;
    }

    // Create BoundingBox objects from user-drawn rects
    std::vector<BoundingBox> initialBoxes;
    for (const auto& rect : userBoxes) {
        BoundingBox box;
        box.trackId = m_data->nextTrackId();
        box.labelId = labelId;
        box.rect = rect;
        box.confidence = 1.0;
        initialBoxes.push_back(box);
    }

    int currentFrame = m_videoManager->currentFrameIndex();
    cv::Mat frame = m_videoManager->currentFrame();

    m_data->setTrackingStartFrame(currentFrame);

    // Store initial frame annotation
    FrameAnnotation fa;
    fa.frameIndex = currentFrame;
    fa.boxes = initialBoxes;
    m_data->addFrameAnnotation(fa);

    m_trackingEngine->initialize(frame, currentFrame, initialBoxes);
    m_trackingEngine->start();

    m_videoWidget->clearUserBoxes();
    m_state = STATE_TRACKING;
    updateButtonStates();

    statusBar()->showMessage(tr("Tracking %1 object(s)...").arg(initialBoxes.size()));
}

void MainWindow::onStop()
{
    m_trackingEngine->stop();
    m_state = STATE_PAUSED;
    updateButtonStates();

    statusBar()->showMessage(tr("Tracking paused at frame %1. Accept to save, or draw new boxes and Run.")
                                 .arg(m_videoManager->currentFrameIndex()));
}

void MainWindow::onAccept()
{
    auto& active = m_data->activeAnnotations();
    if (active.empty()) {
        QMessageBox::information(this, tr("Info"), tr("No tracked data to accept."));
        return;
    }

    ResultSegment seg;
    seg.segmentId = static_cast<int>(m_data->segments().size());
    seg.title = QString("video%1").arg(seg.segmentId);
    seg.startFrame = m_data->trackingStartFrame();
    seg.endFrame = m_trackingEngine->currentTrackingFrame();
    seg.annotations = active;

    // Capture thumbnail from first frame
    cv::Mat firstFrame = m_videoManager->getFrame(seg.startFrame);
    if (!firstFrame.empty()) {
        seg.thumbnail = FrameConverter::matToQImage(firstFrame)
                            .scaled(120, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    m_data->acceptSegment(seg);
    m_data->clearActiveAnnotations();
    m_trackingEngine->reset();
    m_videoWidget->clearOverlayBoxes();

    m_state = STATE_IDLE;
    updateButtonStates();

    statusBar()->showMessage(tr("Segment '%1' saved (frames %2-%3). Draw new boxes to continue.")
                                 .arg(seg.title).arg(seg.startFrame).arg(seg.endFrame));
}

void MainWindow::onUndo()
{
    int startFrame = m_data->trackingStartFrame();

    m_trackingEngine->reset();
    m_data->clearActiveAnnotations();
    m_videoWidget->clearOverlayBoxes();
    m_videoWidget->clearUserBoxes();

    // Seek back to where tracking started
    displayFrameAt(startFrame);

    m_state = STATE_IDLE;
    updateButtonStates();

    statusBar()->showMessage(tr("Tracking undone. Returned to frame %1.").arg(startFrame));
}

void MainWindow::onFrameTracked(int frameIndex, const std::vector<BoundingBox>& boxes)
{
    // Store annotation
    FrameAnnotation fa;
    fa.frameIndex = frameIndex;
    fa.boxes = boxes;
    m_data->addFrameAnnotation(fa);

    // Update display
    cv::Mat frame = m_videoManager->currentFrame();
    m_videoWidget->displayFrame(frame);
    m_videoWidget->setOverlayBoxes(boxes, m_data->labels());
    m_controlBar->setCurrentFrame(frameIndex);
}

void MainWindow::onTrackingFinished()
{
    m_state = STATE_PAUSED;
    updateButtonStates();
    statusBar()->showMessage(tr("Tracking reached end of video. Accept to save results."));
}

void MainWindow::onFrameSliderChanged(int frame)
{
    if (m_state == STATE_IDLE || m_state == STATE_NO_VIDEO) {
        displayFrameAt(frame);
    }
}

void MainWindow::onSegmentDoubleClicked(int index)
{
    const auto& segments = m_data->segments();
    if (index < 0 || index >= static_cast<int>(segments.size()))
        return;

    // Stop any current tracking
    if (m_state == STATE_TRACKING)
        m_trackingEngine->stop();

    const auto& seg = segments[index];
    m_playbackSegmentIndex = index;
    m_playbackFrame = seg.startFrame;

    int interval = static_cast<int>(1000.0 / m_videoManager->fps());
    m_playbackTimer->start(interval);

    statusBar()->showMessage(tr("Playing segment '%1'...").arg(seg.title));
}

void MainWindow::onMergeRequested()
{
    if (m_data->segments().empty()) {
        QMessageBox::information(this, tr("Info"), tr("No segments to merge."));
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this, tr("Save Merged Video"), "merged_output.mp4",
        tr("MP4 Videos (*.mp4)"));

    if (path.isEmpty()) return;

    if (m_videoManager->mergeSegments(path, m_data->segments())) {
        QMessageBox::information(this, tr("Success"),
                                 tr("Merged video saved to: %1").arg(path));
    } else {
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to merge and save video."));
    }
}

void MainWindow::onExportMotRequested()
{
    if (m_data->segments().empty()) {
        QMessageBox::information(this, tr("Info"), tr("No segments to export."));
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this, tr("Export MOT CSV"), "ground_truth.txt",
        tr("Text Files (*.txt);;CSV Files (*.csv)"));

    if (path.isEmpty()) return;

    if (MotExporter::exportToFile(path, m_data->segments(), m_data->labels())) {
        QMessageBox::information(this, tr("Success"),
                                 tr("MOT ground truth exported to: %1").arg(path));
    } else {
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to export MOT file."));
    }
}

void MainWindow::onBoxDrawn(const QRectF& /*videoRect*/)
{
    // Update button states since we now have boxes
    updateButtonStates();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Space:
        if (m_state == STATE_IDLE || m_state == STATE_PAUSED)
            onRun();
        else if (m_state == STATE_TRACKING)
            onStop();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (m_state == STATE_PAUSED)
            onAccept();
        break;
    case Qt::Key_Z:
        if (event->modifiers() & Qt::ControlModifier) {
            if (m_state == STATE_PAUSED)
                onUndo();
        }
        break;
    case Qt::Key_Left:
        if (m_state == STATE_IDLE && m_videoManager->isOpened()) {
            int f = std::max(0, m_videoManager->currentFrameIndex() - 1);
            displayFrameAt(f);
        }
        break;
    case Qt::Key_Right:
        if (m_state == STATE_IDLE && m_videoManager->isOpened()) {
            int f = std::min(m_videoManager->totalFrames() - 1,
                             m_videoManager->currentFrameIndex() + 1);
            displayFrameAt(f);
        }
        break;
    default:
        QMainWindow::keyPressEvent(event);
    }
}
