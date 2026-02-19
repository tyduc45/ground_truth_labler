#pragma once

#include <QObject>
#include <QSize>
#include <QString>
#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>
#include <vector>

struct ResultSegment;

class VideoManager : public QObject {
    Q_OBJECT
public:
    explicit VideoManager(QObject* parent = nullptr);
    ~VideoManager();

    bool openVideo(const QString& path);
    void closeVideo();
    bool isOpened() const;

    cv::Mat getFrame(int index);
    cv::Mat currentFrame() const { return m_currentFrame; }
    int currentFrameIndex() const { return m_currentIndex; }
    int totalFrames() const { return m_totalFrames; }
    double fps() const { return m_fps; }
    QSize frameSize() const { return m_frameSize; }
    QString filePath() const { return m_filePath; }

    // Write a result segment as a video file with bounding box overlays
    bool writeSegmentVideo(const QString& outputPath,
                           const ResultSegment& segment);

    // Merge multiple segments into one video
    bool mergeSegments(const QString& outputPath,
                       const std::vector<ResultSegment>& segments);

signals:
    void videoOpened(const QString& path);
    void videoClosed();

private:
    cv::VideoCapture m_capture;
    cv::Mat          m_currentFrame;
    int              m_currentIndex = -1;
    int              m_totalFrames = 0;
    double           m_fps = 30.0;
    QSize            m_frameSize;
    QString          m_filePath;
};
