#include "FrameConverter.h"
#include <opencv2/imgproc.hpp>

QImage FrameConverter::matToQImage(const cv::Mat& mat)
{
    if (mat.empty())
        return QImage();

    if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows,
                      static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
    }

    if (mat.type() == CV_8UC1) {
        return QImage(mat.data, mat.cols, mat.rows,
                      static_cast<int>(mat.step), QImage::Format_Grayscale8).copy();
    }

    return QImage();
}

cv::Mat FrameConverter::qImageToMat(const QImage& image)
{
    if (image.isNull())
        return cv::Mat();

    QImage img = image.convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(img.height(), img.width(), CV_8UC3,
                const_cast<uchar*>(img.bits()),
                static_cast<size_t>(img.bytesPerLine()));
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_RGB2BGR);
    return bgr;
}
