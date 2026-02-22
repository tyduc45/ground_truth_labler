#include "VideoWidget.h"
#include "util/FrameConverter.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <algorithm>

static constexpr double kMinZoom      = 0.5;
static constexpr double kMaxZoom      = 20.0;
static constexpr double kZoomStep     = 1.15;
static constexpr qreal  kHandleHalf   = 6.0; // handle square half-size (widget px)

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(320, 240);
    setStyleSheet("background-color: black;");
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------


int VideoWidget::getSelectedBox() {
    return m_selectedBox;
}
void VideoWidget::setSelectedBox(int value) {
	m_selectedBox = value;
}

void VideoWidget::displayFrame(const cv::Mat& frame)
{
    m_displayImage = FrameConverter::matToQImage(frame);
    if (!m_displayImage.isNull()) {
        bool sizeChanged = (m_displayImage.size() != m_videoSize);
        m_videoSize = m_displayImage.size();
        if (sizeChanged) {
            m_zoomFactor = 1.0;
            initializePan();
        }
        updateTransforms();
    }
    update();
}

void VideoWidget::setOverlayBoxes(const std::vector<BoundingBox>& boxes,
                                   const std::vector<LabelDef>& labels)
{
    m_overlayBoxes = boxes;
    m_labels = labels;
    update();
}

void VideoWidget::clearOverlayBoxes()
{
    m_overlayBoxes.clear();
    update();
}

void VideoWidget::setDrawingEnabled(bool enabled)
{
    m_drawingEnabled = enabled;
    if (!enabled) {
        m_dragMode    = DragNone;
        m_panning     = false;
        m_selectedBox = -1;
        setCursor(Qt::ArrowCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }
}

void VideoWidget::clearUserBoxes()
{
    m_userBoxes.clear();
    m_selectedBox = -1;
    m_dragMode    = DragNone;
    update();
}
void VideoWidget::removeSelectedUserbox(int index)
{
	m_userBoxes.erase(m_userBoxes.begin() + index);
}

void VideoWidget::removeLastUserBox()
{
    if (m_userBoxes.empty()) return;
    m_userBoxes.pop_back();
    if (m_selectedBox >= static_cast<int>(m_userBoxes.size()))
        m_selectedBox = -1;
    emit userBoxesChanged();
    update();
}

void VideoWidget::resetZoom()
{
    m_zoomFactor = 1.0;
    initializePan();
    updateTransforms();
    update();
}

// ---------------------------------------------------------------------------
// Coordinate conversion
// ---------------------------------------------------------------------------

QRectF VideoWidget::widgetToVideo(const QRectF& widgetRect) const
{
    return m_widgetToScreenXform.mapRect(widgetRect);
}

QRectF VideoWidget::videoToWidget(const QRectF& videoRect) const
{
    return m_screenToWidgetXform.mapRect(videoRect);
}

QPointF VideoWidget::widgetToVideoPoint(const QPointF& p) const
{
    return m_widgetToScreenXform.map(p);
}

QPointF VideoWidget::videoToWidgetPoint(const QPointF& p) const
{
    return m_screenToWidgetXform.map(p);
}

// ---------------------------------------------------------------------------
// Layout / transform helpers
// ---------------------------------------------------------------------------

void VideoWidget::initializePan()
{
    if (m_videoSize.isEmpty()) return;
    double videoW = m_videoSize.width();
    double videoH = m_videoSize.height();
    m_baseScale = std::min(width() / videoW, height() / videoH);
    double scale = m_baseScale * m_zoomFactor;
    m_panX = (width()  - videoW * scale) / 2.0;
    m_panY = (height() - videoH * scale) / 2.0;
}

void VideoWidget::clampPan()
{
    if (m_videoSize.isEmpty()) return;
    double scale      = m_baseScale * m_zoomFactor;
    double videoDispW = m_videoSize.width()  * scale;
    double videoDispH = m_videoSize.height() * scale;
    double widgetW    = width();
    double widgetH    = height();

    if (videoDispW <= widgetW)
        m_panX = (widgetW - videoDispW) / 2.0;
    else
        m_panX = qBound(widgetW - videoDispW, m_panX, 0.0);

    if (videoDispH <= widgetH)
        m_panY = (widgetH - videoDispH) / 2.0;
    else
        m_panY = qBound(widgetH - videoDispH, m_panY, 0.0);
}

void VideoWidget::updateTransforms()
{
    if (m_videoSize.isEmpty()) return;

    double scale = m_baseScale * m_zoomFactor;
    m_displayRect = QRectF(m_panX, m_panY,
                           m_videoSize.width()  * scale,
                           m_videoSize.height() * scale);
    /*
    |-----------|<----screen
    |  |--|     |
    |  |. |<----|---- widget
    |  |__|     |
    |           |
    |___________|
    */

    m_screenToWidgetXform = QTransform();
    m_screenToWidgetXform.translate(m_panX, m_panY);
    m_screenToWidgetXform.scale(scale, scale);

    m_widgetToScreenXform = QTransform();
    m_widgetToScreenXform.scale(1.0 / scale, 1.0 / scale);
    m_widgetToScreenXform.translate(-m_panX, -m_panY);
}

// ---------------------------------------------------------------------------
// Hit testing
// ---------------------------------------------------------------------------

int VideoWidget::hitTestUserBox(const QPointF& pos) const
{
    // Iterate in reverse so topmost (last added) is selected first
    for (int i = static_cast<int>(m_userBoxes.size()) - 1; i >= 0; --i) {
        if (videoToWidget(m_userBoxes[i]).contains(pos))
            return i;
    }
    return -1;
}

int VideoWidget::hitTestResizeHandle(const QPointF& pos, int boxIndex) const
{
    if (boxIndex < 0 || boxIndex >= static_cast<int>(m_userBoxes.size()))
        return -1;
    QRectF wRect = videoToWidget(m_userBoxes[boxIndex]);
    QPointF corners[4] = {
        wRect.topLeft(), wRect.topRight(),
        wRect.bottomRight(), wRect.bottomLeft()
    };
    for (int i = 0; i < 4; ++i) {
        QRectF handle(corners[i] - QPointF(kHandleHalf, kHandleHalf),
                      QSizeF(2 * kHandleHalf, 2 * kHandleHalf));
        if (handle.contains(pos))
            return i;
    }
    return -1;
}

void VideoWidget::updateCursorForPos(const QPointF& pos)
{
    if (!m_drawingEnabled) { setCursor(Qt::ArrowCursor); return; }

    if (m_selectedBox >= 0) {
        int h = hitTestResizeHandle(pos, m_selectedBox);
        if (h >= 0) {
            static const Qt::CursorShape kResizeCursors[4] = {
                Qt::SizeFDiagCursor, Qt::SizeBDiagCursor,
                Qt::SizeFDiagCursor, Qt::SizeBDiagCursor
            };
            setCursor(kResizeCursors[h]);
            return;
        }
    }
    if (hitTestUserBox(pos) >= 0) { setCursor(Qt::SizeAllCursor); return; }
    setCursor(Qt::CrossCursor);
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void VideoWidget::drawHandles(QPainter& painter, const QRectF& wRect)
{
    QPointF corners[4] = {
        wRect.topLeft(), wRect.topRight(),
        wRect.bottomRight(), wRect.bottomLeft()
    };
    painter.setBrush(Qt::white);
    painter.setPen(QPen(Qt::black, 1));
    for (const auto& c : corners) {
        painter.drawRect(QRectF(c - QPointF(kHandleHalf, kHandleHalf),
                                QSizeF(2 * kHandleHalf, 2 * kHandleHalf)));
    }
}

void VideoWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Background
    painter.fillRect(rect(), Qt::black);

    // Video frame
    if (!m_displayImage.isNull())
        painter.drawImage(m_displayRect, m_displayImage);

    // Overlay boxes (tracked / existing)
    for (const auto& box : m_overlayBoxes) {
        QColor color = Qt::green;
        QString label;
        for (const auto& lbl : m_labels) {
            if (lbl.id == box.labelId) {
                color = lbl.color;
                label = lbl.name;
                break;
            }
        }
        QRectF wRect = videoToWidget(box.rect);
        painter.setPen(QPen(color, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(wRect);
        if (!label.isEmpty()) {
            painter.setFont(QFont("Arial", 10));
            QRectF textBg(wRect.topLeft() - QPointF(0, 18),
                          QSizeF(label.length() * 8 + 8, 18));
            painter.fillRect(textBg, QColor(color.red(), color.green(), color.blue(), 160));
            painter.setPen(Qt::white);
            painter.drawText(textBg, Qt::AlignCenter, label);
        }
    }

    // User-drawn boxes (pending)
    for (int i = 0; i < static_cast<int>(m_userBoxes.size()); ++i) {
        QRectF wRect = videoToWidget(m_userBoxes[i]);
        bool selected = (i == m_selectedBox);
        painter.setBrush(Qt::NoBrush);
        if (selected) {
            painter.setPen(QPen(QColor(0, 160, 255), 2, Qt::SolidLine));
            painter.drawRect(wRect);
            drawHandles(painter, wRect);
        } else {
            painter.setPen(QPen(Qt::yellow, 2, Qt::DashLine));
            painter.drawRect(wRect);
        }
    }

    // Rubber-band while drawing
    if (m_dragMode == DragDraw) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Qt::cyan, 2, Qt::DashDotLine));
        painter.drawRect(QRectF(m_drawStart, m_drawCurrent).normalized());
    }

    // Zoom indicator (top-right corner) when zoomed
    if (m_zoomFactor > 1.01) {
        QString zoomText = QString("x%1").arg(m_zoomFactor, 0, 'f', 1);
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        QRect textRect(width() - 60, 4, 56, 20);
        painter.fillRect(textRect, QColor(0, 0, 0, 140));
        painter.setPen(Qt::white);
        painter.drawText(textRect, Qt::AlignCenter, zoomText);
    }
}

// ---------------------------------------------------------------------------
// Mouse events
// ---------------------------------------------------------------------------

void VideoWidget::mousePressEvent(QMouseEvent* event)
{
    // Middle button: start panning
    if (event->button() == Qt::MiddleButton) {
        m_panning       = true;
        m_panStartMouse = event->pos();
        m_panXAtStart   = m_panX;
        m_panYAtStart   = m_panY;
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (!m_drawingEnabled || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    QPointF pos = event->pos();

    // 1. Check resize handles of currently selected box
    if (m_selectedBox >= 0) {
        int handle = hitTestResizeHandle(pos, m_selectedBox);
        if (handle >= 0) {
            m_dragMode        = DragResize;
            m_resizeHandle    = handle;
            m_dragStartWidget = pos;
            m_dragBoxOrigRect = m_userBoxes[m_selectedBox];
            return;
        }
    }

    // 2. Check if clicking on any user box → select + move
    int hitBox = hitTestUserBox(pos);
    if (hitBox >= 0) {
        m_selectedBox     = hitBox;
        m_dragMode        = DragMove;
        m_dragStartWidget = pos;
        m_dragBoxOrigRect = m_userBoxes[hitBox];
        update();
        return;
    }

    // 3. Start drawing a new box if click lands within video bounds
    QPointF vp = widgetToVideoPoint(pos);
    if (vp.x() >= 0 && vp.x() <= m_videoSize.width() &&
        vp.y() >= 0 && vp.y() <= m_videoSize.height()) {
        m_selectedBox = -1;
        m_dragMode    = DragDraw;
        m_drawStart   = pos;
        m_drawCurrent = pos;
        update();
    }
}

void VideoWidget::mouseMoveEvent(QMouseEvent* event)
{
    QPointF pos = event->pos();

    // Panning
    if (m_panning) {
        QPointF delta = pos - m_panStartMouse;
        m_panX = m_panXAtStart + delta.x();
        m_panY = m_panYAtStart + delta.y();
        clampPan();
        updateTransforms();
        update();
        return;
    }

    switch (m_dragMode) {
    case DragDraw:
        m_drawCurrent = pos;
        update();
        break;

    case DragMove: {
        if (m_selectedBox >= 0 && m_selectedBox < static_cast<int>(m_userBoxes.size())) {
            double scale = m_baseScale * m_zoomFactor;
            QPointF delta = pos - m_dragStartWidget;
            QPointF deltaV(delta.x() / scale, delta.y() / scale);
            QRectF  newRect(m_dragBoxOrigRect.topLeft() + deltaV,
                            m_dragBoxOrigRect.size());
            // Clamp so box stays within video
            QPointF tl = newRect.topLeft();
            tl.setX(qBound(0.0, tl.x(),
                           m_videoSize.width()  - newRect.width()));
            tl.setY(qBound(0.0, tl.y(),
                           m_videoSize.height() - newRect.height()));
            m_userBoxes[m_selectedBox] = QRectF(tl, m_dragBoxOrigRect.size());
            update();
        }
        break;
    }

    case DragResize: {
        if (m_selectedBox >= 0 && m_selectedBox < static_cast<int>(m_userBoxes.size())) {
            // The corner opposite to the handle stays fixed
            QPointF fixedCorner;
            switch (m_resizeHandle) {
            case 0: fixedCorner = m_dragBoxOrigRect.bottomRight(); break;
            case 1: fixedCorner = m_dragBoxOrigRect.bottomLeft();  break;
            case 2: fixedCorner = m_dragBoxOrigRect.topLeft();     break;
            case 3: fixedCorner = m_dragBoxOrigRect.topRight();    break;
            default: break;
            }
            QPointF movingCorner = widgetToVideoPoint(pos);
            movingCorner.setX(qBound(0.0, movingCorner.x(),
                                     static_cast<double>(m_videoSize.width())));
            movingCorner.setY(qBound(0.0, movingCorner.y(),
                                     static_cast<double>(m_videoSize.height())));
            QRectF newRect = QRectF(fixedCorner, movingCorner).normalized();
            if (newRect.width() > 1 && newRect.height() > 1)
                m_userBoxes[m_selectedBox] = newRect;
            update();
        }
        break;
    }

    case DragNone:
        updateCursorForPos(pos);
        break;
    }
}

void VideoWidget::mouseReleaseEvent(QMouseEvent* event)
{
    // End panning
    if (event->button() == Qt::MiddleButton && m_panning) {
        m_panning = false;
        updateCursorForPos(event->pos());
        return;
    }

    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    switch (m_dragMode) {
    case DragDraw: {
        QRectF widgetRect = QRectF(m_drawStart, m_drawCurrent).normalized();
        if (widgetRect.width() > 5 && widgetRect.height() > 5) {
            QRectF videoRect = widgetToVideo(widgetRect);
            videoRect = videoRect.intersected(
                QRectF(0, 0, m_videoSize.width(), m_videoSize.height()));
            if (videoRect.width() > 1 && videoRect.height() > 1) {
                m_userBoxes.push_back(videoRect);
                m_selectedBox = static_cast<int>(m_userBoxes.size()) - 1;
                emit boxDrawn(videoRect);
            }
        }
        break;
    }
    case DragMove:
    case DragResize:
        emit userBoxesChanged();
        break;
    case DragNone:
        break;
    }

    m_dragMode = DragNone;
    update();
}

void VideoWidget::wheelEvent(QWheelEvent* event)
{
    if (m_videoSize.isEmpty()) { QWidget::wheelEvent(event); return; }

    double factor   = event->angleDelta().y() > 0 ? kZoomStep : 1.0 / kZoomStep;
    double oldZoom  = m_zoomFactor;
    m_zoomFactor    = qBound(kMinZoom, m_zoomFactor * factor, kMaxZoom);

    if (qAbs(m_zoomFactor - oldZoom) > 1e-9) {
        // Keep the point under the cursor fixed in video space
        double ratio = m_zoomFactor / oldZoom;
        QPointF mp   = event->position();
        m_panX = mp.x() - (mp.x() - m_panX) * ratio;
        m_panY = mp.y() - (mp.y() - m_panY) * ratio;
        clampPan();
        updateTransforms();
        update();
    }
    event->accept();
}

void VideoWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (!m_videoSize.isEmpty()) {
        double videoW      = m_videoSize.width();
        double videoH      = m_videoSize.height();
        double newBase     = std::min(width() / videoW, height() / videoH);
        double oldScale    = m_baseScale * m_zoomFactor;
        double newScale    = newBase    * m_zoomFactor;

        // Maintain video center
        double cx = (width()  / 2.0 - m_panX) / oldScale;
        double cy = (height() / 2.0 - m_panY) / oldScale;
        m_baseScale = newBase;
        m_panX = width()  / 2.0 - cx * newScale;
        m_panY = height() / 2.0 - cy * newScale;
        clampPan();
        updateTransforms();
    }
}
