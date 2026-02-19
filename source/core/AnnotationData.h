#pragma once

#include <QObject>
#include <QColor>
#include <QImage>
#include <QRectF>
#include <QString>
#include <vector>

struct LabelDef {
    int     id = 0;
    QString name;
    QString description;
    QColor  color = Qt::green;
};

struct BoundingBox {
    int    trackId = 0;
    int    labelId = 0;
    QRectF rect;           // x, y, w, h in video pixel coordinates
    double confidence = 1.0;
};

struct FrameAnnotation {
    int                      frameIndex = 0;
    std::vector<BoundingBox> boxes;
};

struct ResultSegment {
    int                              segmentId = 0;
    QString                          title;
    int                              startFrame = 0;
    int                              endFrame = 0;
    QImage                           thumbnail;
    std::vector<FrameAnnotation>     annotations;
};

class AnnotationData : public QObject {
    Q_OBJECT
public:
    explicit AnnotationData(QObject* parent = nullptr);

    // Label management
    void addLabel(const LabelDef& label);
    void removeLabel(int labelId);
    void updateLabelColor(int id, QColor color);
    const std::vector<LabelDef>& labels() const { return m_labels; }
    LabelDef* labelById(int id);
    int nextLabelId();

    // Active tracking session (not yet accepted)
    std::vector<FrameAnnotation>& activeAnnotations() { return m_activeAnnotations; }
    void addFrameAnnotation(const FrameAnnotation& fa);
    void clearActiveAnnotations();
    void setTrackingStartFrame(int frame) { m_trackingStartFrame = frame; }
    int trackingStartFrame() const { return m_trackingStartFrame; }

    // Finalized segments
    void acceptSegment(const ResultSegment& seg);
    const std::vector<ResultSegment>& segments() const { return m_segments; }
    void removeSegment(int index);

    // Track ID management
    int nextTrackId() { return m_nextTrackId++; }

signals:
    void labelsChanged();
    void segmentsChanged();
    void activeAnnotationsChanged();

private:
    std::vector<LabelDef>        m_labels;
    std::vector<FrameAnnotation> m_activeAnnotations;
    std::vector<ResultSegment>   m_segments;
    int                          m_nextTrackId = 1;
    int                          m_nextLabelId = 1;
    int                          m_trackingStartFrame = 0;
};
