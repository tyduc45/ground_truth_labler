#include "TrackingEngine.h"
#include "VideoManager.h"

TrackingEngine::TrackingEngine(VideoManager* videoManager, QObject* parent)
    : QObject(parent)
    , m_videoManager(videoManager)
{
    m_stepTimer = new QTimer(this);
    m_stepTimer->setInterval(0); // Process as fast as possible
    connect(m_stepTimer, &QTimer::timeout, this, &TrackingEngine::processNextFrame);
}

void TrackingEngine::initialize(const cv::Mat& frame, int frameIndex,
                                 const std::vector<BoundingBox>& initialBoxes)
{
    reset();
    m_trackingFrameIndex = frameIndex;

    for (const auto& box : initialBoxes) {
        TrackerInstance inst;
        inst.tracker = cv::TrackerKCF::create();
        cv::Rect2d roi(box.rect.x(), box.rect.y(),
                       box.rect.width(), box.rect.height());
        inst.tracker->init(frame, roi);
        inst.box = box;
        m_trackers.push_back(std::move(inst));
    }
}

void TrackingEngine::start()
{
    if (m_trackers.empty()) {
        emit trackingError(tr("No boxes to track. Draw bounding boxes first."));
        return;
    }
    m_running = true;
    m_stepTimer->start();
}

void TrackingEngine::stop()
{
    m_running = false;
    m_stepTimer->stop();
}

void TrackingEngine::reset()
{
    stop();
    m_trackers.clear();
    m_trackingFrameIndex = 0;
}

void TrackingEngine::processNextFrame()
{
    int nextFrame = m_trackingFrameIndex + 1;
    if (nextFrame >= m_videoManager->totalFrames()) {
        stop();
        emit trackingFinished();
        return;
    }

    cv::Mat frame = m_videoManager->getFrame(nextFrame);
    if (frame.empty()) {
        stop();
        emit trackingError(tr("Failed to read frame %1").arg(nextFrame));
        return;
    }

    m_trackingFrameIndex = nextFrame;

    std::vector<BoundingBox> updatedBoxes;
    for (auto& inst : m_trackers) {
        cv::Rect roi;
        bool ok = inst.tracker->update(frame, roi);
        if (ok) {
            inst.box.rect = QRectF(roi.x, roi.y, roi.width, roi.height);
            inst.box.confidence = 1.0;
        } else {
            // Tracker lost - keep last known position but lower confidence
            inst.box.confidence = 0.0;
        }
        updatedBoxes.push_back(inst.box);
    }

    emit frameTracked(m_trackingFrameIndex, updatedBoxes);
}
