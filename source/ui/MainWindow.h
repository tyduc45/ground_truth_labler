#pragma once

#include <QMainWindow>
#include <QTimer>

class VideoWidget;
class LabelPanel;
class ResultPanel;
class ControlBar;
class AnnotationData;
class VideoManager;
class TrackingEngine;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onOpenVideo();
    void onRun();
    void onStop();
    void onAccept();
    void onUndo();
    void onFrameTracked(int frameIndex, const std::vector<struct BoundingBox>& boxes);
    void onTrackingFinished();
    void onFrameSliderChanged(int frame);
    void onSegmentDoubleClicked(int index);
    void onMergeRequested();
    void onExportMotRequested();
    void onBoxDrawn(const QRectF& videoRect);

private:
    void setupLayout();
    void setupMenuBar();
    void connectSignals();
    void updateButtonStates();
    void displayFrameAt(int frameIndex);

    enum AppState { STATE_NO_VIDEO, STATE_IDLE, STATE_TRACKING, STATE_PAUSED };
    AppState m_state = STATE_NO_VIDEO;

    // UI
    VideoWidget*  m_videoWidget;
    LabelPanel*   m_labelPanel;
    ResultPanel*  m_resultPanel;
    ControlBar*   m_controlBar;

    // Core
    AnnotationData*  m_data;
    VideoManager*    m_videoManager;
    TrackingEngine*  m_trackingEngine;

    // Playback timer for viewing result segments
    QTimer*  m_playbackTimer;
    int      m_playbackSegmentIndex = -1;
    int      m_playbackFrame = 0;
};
