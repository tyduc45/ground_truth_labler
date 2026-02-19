#pragma once

#include <QWidget>
#include <QImage>
#include <QRectF>
#include <QPointF>
#include <QTransform>
#include <vector>
#include <opencv2/core.hpp>
#include "core/AnnotationData.h"

class QWheelEvent;

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
    void removeLastUserBox();
    void resetZoom();
    std::vector<QRectF> userDrawnBoxes() const { return m_userBoxes; }

    // Coordinate conversion
    QRectF  widgetToVideo(const QRectF& widgetRect) const;
    QRectF  videoToWidget(const QRectF& videoRect) const;
    QPointF widgetToVideoPoint(const QPointF& p) const;
    QPointF videoToWidgetPoint(const QPointF& p) const;

signals:
    void boxDrawn(const QRectF& videoRect);
    void userBoxesChanged();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateTransforms();
    void initializePan();
    void clampPan();
    int  hitTestUserBox(const QPointF& widgetPos) const;
    int  hitTestResizeHandle(const QPointF& widgetPos, int boxIndex) const;
    void drawHandles(QPainter& painter, const QRectF& wRect);
    void updateCursorForPos(const QPointF& widgetPos);

    QImage                   m_displayImage;
    std::vector<BoundingBox> m_overlayBoxes;
    std::vector<LabelDef>    m_labels;

    // Drawing enabled flag
    bool m_drawingEnabled = false;

    // Drag mode
    enum DragMode { DragNone, DragDraw, DragMove, DragResize };
    DragMode m_dragMode = DragNone;

    // New-box drawing state (widget coords)
    QPointF m_drawStart;
    QPointF m_drawCurrent;

    // Box selection / manipulation
    int     m_selectedBox    = -1; // index in m_userBoxes (-1 = none)
    int     m_resizeHandle   = -1; // 0=TL,1=TR,2=BR,3=BL
    QPointF m_dragStartWidget;
    QRectF  m_dragBoxOrigRect; // video coords at drag start

    std::vector<QRectF> m_userBoxes; // video coordinates

    // Display geometry
    QRectF m_displayRect;
    QSize  m_videoSize;

    // Zoom / pan
    double m_baseScale   = 1.0; // best-fit scale (no zoom)
    double m_zoomFactor  = 1.0;
    double m_panX        = 0.0; // video display origin in widget coords
    double m_panY        = 0.0;

    // Middle-mouse panning state
    bool    m_panning      = false;
    QPointF m_panStartMouse;
    double  m_panXAtStart  = 0.0;
    double  m_panYAtStart  = 0.0;

    // Coordinate transforms
    QTransform m_widgetToVideoXform;
    QTransform m_videoToWidgetXform;
};
