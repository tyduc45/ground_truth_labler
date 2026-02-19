#include "VideoWidget.h"
#include "util/FrameConverter.h"
#include <QPainter>
#include <QMouseEvent>
#include <algorithm>

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(320, 240);
    setStyleSheet("background-color: black;");
}

void VideoWidget::displayFrame(const cv::Mat& frame)
{
    m_displayImage = FrameConverter::matToQImage(frame);
    if (!m_displayImage.isNull()) {
        m_videoSize = m_displayImage.size();
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
    setCursor(enabled ? Qt::CrossCursor : Qt::ArrowCursor);
}

void VideoWidget::clearUserBoxes()
{
    m_userBoxes.clear();
    update();
}

QRectF VideoWidget::widgetToVideo(const QRectF& widgetRect) const
{
    return m_widgetToVideoXform.mapRect(widgetRect);
}

QRectF VideoWidget::videoToWidget(const QRectF& videoRect) const
{
    return m_videoToWidgetXform.mapRect(videoRect);
}

void VideoWidget::updateTransforms()
{
    if (m_videoSize.isEmpty()) return;

    double widgetW = width();
    double widgetH = height();
    double videoW = m_videoSize.width();
    double videoH = m_videoSize.height();

    double scale = std::min(widgetW / videoW, widgetH / videoH);
    double displayW = videoW * scale;
    double displayH = videoH * scale;
    double offsetX = (widgetW - displayW) / 2.0;
    double offsetY = (widgetH - displayH) / 2.0;

    m_displayRect = QRectF(offsetX, offsetY, displayW, displayH);

    // Widget -> Video: subtract offset, divide by scale
    m_widgetToVideoXform = QTransform();
    m_widgetToVideoXform.translate(-offsetX, -offsetY);
    m_widgetToVideoXform.scale(1.0 / scale, 1.0 / scale);

    // Video -> Widget: multiply by scale, add offset
    m_videoToWidgetXform = QTransform();
    m_videoToWidgetXform.scale(scale, scale);
    m_videoToWidgetXform.translate(offsetX / scale, offsetY / scale);
}

void VideoWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Fill background
    painter.fillRect(rect(), Qt::black);

    // Draw video frame
    if (!m_displayImage.isNull()) {
        painter.drawImage(m_displayRect, m_displayImage);
    }

    // Draw overlay boxes (tracked/existing)
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
        QPen pen(color, 2);
        painter.setPen(pen);
        painter.drawRect(wRect);

        // Draw label text
        if (!label.isEmpty()) {
            painter.setFont(QFont("Arial", 10));
            QRectF textBg(wRect.topLeft() - QPointF(0, 18), QSizeF(label.length() * 8 + 8, 18));
            painter.fillRect(textBg, QColor(color.red(), color.green(), color.blue(), 160));
            painter.setPen(Qt::white);
            painter.drawText(textBg, Qt::AlignCenter, label);
        }
    }

    // Draw user-drawn boxes (pending, not yet tracked)
    QPen userPen(Qt::yellow, 2, Qt::DashLine);
    painter.setPen(userPen);
    for (const auto& vbox : m_userBoxes) {
        QRectF wRect = videoToWidget(vbox);
        painter.drawRect(wRect);
    }

    // Draw current drawing rectangle
    if (m_drawing) {
        QPen drawPen(Qt::cyan, 2, Qt::DashDotLine);
        painter.setPen(drawPen);
        QRectF drawRect = QRectF(m_drawStart, m_drawCurrent).normalized();
        painter.drawRect(drawRect);
    }
}

void VideoWidget::mousePressEvent(QMouseEvent* event)
{
    if (!m_drawingEnabled || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    QPointF pos = event->pos();
    if (m_displayRect.contains(pos)) {
        m_drawing = true;
        m_drawStart = pos;
        m_drawCurrent = pos;
    }
}

void VideoWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_drawing) {
        m_drawCurrent = event->pos();
        update();
    }
}

void VideoWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_drawing || event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    m_drawing = false;
    QRectF widgetRect = QRectF(m_drawStart, m_drawCurrent).normalized();

    // Only accept boxes with minimum size
    if (widgetRect.width() > 5 && widgetRect.height() > 5) {
        QRectF videoRect = widgetToVideo(widgetRect);
        // Clamp to video bounds
        videoRect = videoRect.intersected(QRectF(0, 0, m_videoSize.width(), m_videoSize.height()));
        if (videoRect.width() > 1 && videoRect.height() > 1) {
            m_userBoxes.push_back(videoRect);
            emit boxDrawn(videoRect);
        }
    }

    update();
}

void VideoWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateTransforms();
}
