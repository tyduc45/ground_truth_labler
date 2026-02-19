#pragma once

#include <QWidget>
#include <QImage>
#include <QRectF>
#include <QPointF>
#include <QTransform>
#include <vector>
#include <opencv2/core.hpp>
#include "core/AnnotationData.h"

class VideoWidget : public QWidget {
    Q_OBJECT
public:
    explicit VideoWidget(QWidget* parent = nullptr);

    void displayFrame(const cv::Mat& frame);
    void setOverlayBoxes(const std::vector<BoundingBox>& boxes,
                         const std::vector<LabelDef>& labels);
    void clearOverlayBoxes();
    void setDrawingEnabled(bool enabled);
    void clearUserBoxes();
    std::vector<QRectF> userDrawnBoxes() const { return m_userBoxes; }

    // Coordinate conversion
    QRectF widgetToVideo(const QRectF& widgetRect) const;
    QRectF videoToWidget(const QRectF& videoRect) const;

signals:
    void boxDrawn(const QRectF& videoRect);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateTransforms();

    QImage                   m_displayImage;
    std::vector<BoundingBox> m_overlayBoxes;
    std::vector<LabelDef>    m_labels;

    // Drawing state
    bool    m_drawingEnabled = false;
    bool    m_drawing = false;
    QPointF m_drawStart;
    QPointF m_drawCurrent;
    std::vector<QRectF> m_userBoxes; // in video coordinates

    // Display area within widget (aspect-ratio preserved)
    QRectF m_displayRect;
    QSize  m_videoSize;

    // Coordinate transforms
    QTransform m_widgetToVideoXform;
    QTransform m_videoToWidgetXform;
};
