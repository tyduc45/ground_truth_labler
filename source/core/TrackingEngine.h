#pragma once

#include <QObject>
#include <QTimer>
#include <opencv2/core.hpp>
#include <opencv2/tracking.hpp>
#include <vector>
#include "AnnotationData.h"

class VideoManager;

class TrackingEngine : public QObject {
    Q_OBJECT
public:
    explicit TrackingEngine(VideoManager* videoManager, QObject* parent = nullptr);

    void initialize(const cv::Mat& frame, int frameIndex,
                    const std::vector<BoundingBox>& initialBoxes);
    void start();
    void stop();
    void reset();

    bool isRunning() const { return m_running; }
    int currentTrackingFrame() const { return m_trackingFrameIndex; }

signals:
    void frameTracked(int frameIndex, const std::vector<BoundingBox>& boxes);
    void trackingFinished();
    void trackingError(const QString& message);

private slots:
    void processNextFrame();

private:
    struct TrackerInstance {
        cv::Ptr<cv::TrackerKCF> tracker;
        BoundingBox             box;
    };

    std::vector<TrackerInstance> m_trackers;
    VideoManager*               m_videoManager;
    QTimer*                     m_stepTimer;
    bool                        m_running = false;
    int                         m_trackingFrameIndex = 0;
};
