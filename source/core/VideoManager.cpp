#include "VideoManager.h"
#include "AnnotationData.h"
#include <opencv2/imgproc.hpp>

VideoManager::VideoManager(QObject* parent)
    : QObject(parent)
{
}

VideoManager::~VideoManager()
{
    closeVideo();
}

bool VideoManager::openVideo(const QString& path)
{
    closeVideo();

    if (!m_capture.open(path.toStdString()))
        return false;

    m_filePath = path;
    m_totalFrames = static_cast<int>(m_capture.get(cv::CAP_PROP_FRAME_COUNT));
    m_fps = m_capture.get(cv::CAP_PROP_FPS);
    if (m_fps <= 0) m_fps = 30.0;

    int w = static_cast<int>(m_capture.get(cv::CAP_PROP_FRAME_WIDTH));
    int h = static_cast<int>(m_capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    m_frameSize = QSize(w, h);

    // Read the first frame
    getFrame(0);

    emit videoOpened(path);
    return true;
}

void VideoManager::closeVideo()
{
    if (m_capture.isOpened()) {
        m_capture.release();
        m_currentFrame = cv::Mat();
        m_currentIndex = -1;
        m_totalFrames = 0;
        m_filePath.clear();
        emit videoClosed();
    }
}

bool VideoManager::isOpened() const
{
    return m_capture.isOpened();
}

cv::Mat VideoManager::getFrame(int index)
{
    if (!m_capture.isOpened() || index < 0 || index >= m_totalFrames)
        return cv::Mat();

    // Only seek if not the next consecutive frame
    if (index != m_currentIndex + 1) {
        m_capture.set(cv::CAP_PROP_POS_FRAMES, index);
    }

    cv::Mat frame;
    if (m_capture.read(frame)) {
        m_currentFrame = frame;
        m_currentIndex = index;
    }
    return m_currentFrame;
}

bool VideoManager::writeSegmentVideo(const QString& outputPath,
                                      const ResultSegment& segment)
{
    if (!m_capture.isOpened())
        return false;

    int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    cv::VideoWriter writer(outputPath.toStdString(), fourcc, m_fps,
                           cv::Size(m_frameSize.width(), m_frameSize.height()));
    if (!writer.isOpened())
        return false;

    for (int i = segment.startFrame; i <= segment.endFrame; ++i) {
        cv::Mat frame = getFrame(i);
        if (frame.empty()) break;

        // Find annotation for this frame and draw boxes
        for (const auto& fa : segment.annotations) {
            if (fa.frameIndex == i) {
                for (const auto& box : fa.boxes) {
                    cv::Rect r(static_cast<int>(box.rect.x()),
                               static_cast<int>(box.rect.y()),
                               static_cast<int>(box.rect.width()),
                               static_cast<int>(box.rect.height()));
                    cv::rectangle(frame, r, cv::Scalar(0, 255, 0), 2);
                }
                break;
            }
        }

        writer.write(frame);
    }

    writer.release();
    return true;
}

bool VideoManager::mergeSegments(const QString& outputPath,
                                  const std::vector<ResultSegment>& segments)
{
    if (!m_capture.isOpened() || segments.empty())
        return false;

    int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    cv::VideoWriter writer(outputPath.toStdString(), fourcc, m_fps,
                           cv::Size(m_frameSize.width(), m_frameSize.height()));
    if (!writer.isOpened())
        return false;

    for (const auto& seg : segments) {
        for (int i = seg.startFrame; i <= seg.endFrame; ++i) {
            cv::Mat frame = getFrame(i);
            if (frame.empty()) continue;

            for (const auto& fa : seg.annotations) {
                if (fa.frameIndex == i) {
                    for (const auto& box : fa.boxes) {
                        cv::Rect r(static_cast<int>(box.rect.x()),
                                   static_cast<int>(box.rect.y()),
                                   static_cast<int>(box.rect.width()),
                                   static_cast<int>(box.rect.height()));
                        cv::rectangle(frame, r, cv::Scalar(0, 255, 0), 2);
                    }
                    break;
                }
            }

            writer.write(frame);
        }
    }

    writer.release();
    return true;
}
