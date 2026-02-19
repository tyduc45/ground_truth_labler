#pragma once

#include <QImage>
#include <opencv2/core.hpp>

class FrameConverter {
public:
    static QImage matToQImage(const cv::Mat& mat);
    static cv::Mat qImageToMat(const QImage& image);
};
