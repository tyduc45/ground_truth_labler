#include "AnnotationData.h"

AnnotationData::AnnotationData(QObject* parent)
    : QObject(parent)
{
}

void AnnotationData::addLabel(const LabelDef& label)
{
    m_labels.push_back(label);
    emit labelsChanged();
}

void AnnotationData::removeLabel(int labelId)
{
    auto it = std::remove_if(m_labels.begin(), m_labels.end(),
                             [labelId](const LabelDef& l) { return l.id == labelId; });
    if (it != m_labels.end()) {
        m_labels.erase(it, m_labels.end());
        emit labelsChanged();
    }
}

LabelDef* AnnotationData::labelById(int id)
{
    for (auto& l : m_labels) {
        if (l.id == id)
            return &l;
    }
    return nullptr;
}

int AnnotationData::nextLabelId()
{
    return m_nextLabelId++;
}

void AnnotationData::addFrameAnnotation(const FrameAnnotation& fa)
{
    m_activeAnnotations.push_back(fa);
    emit activeAnnotationsChanged();
}

void AnnotationData::clearActiveAnnotations()
{
    m_activeAnnotations.clear();
    m_trackingStartFrame = 0;
    emit activeAnnotationsChanged();
}

void AnnotationData::acceptSegment(const ResultSegment& seg)
{
    m_segments.push_back(seg);
    emit segmentsChanged();
}

void AnnotationData::removeSegment(int index)
{
    if (index >= 0 && index < static_cast<int>(m_segments.size())) {
        m_segments.erase(m_segments.begin() + index);
        emit segmentsChanged();
    }
}
