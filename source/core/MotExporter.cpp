#include "MotExporter.h"
#include "AnnotationData.h"
#include <QFile>
#include <QTextStream>

bool MotExporter::exportToFile(const QString& filePath,
                                const std::vector<ResultSegment>& segments,
                                const std::vector<LabelDef>& labels)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);

    // Global frame counter for continuous numbering across segments
    int globalFrame = 1;

    for (const auto& seg : segments) {
        for (const auto& fa : seg.annotations) {
            for (const auto& box : fa.boxes) {
                // Find class id from label
                int classId = box.labelId;
                double visibility = (box.confidence > 0) ? 1.0 : 0.0;

                // MOT format: frame,id,bb_left,bb_top,bb_width,bb_height,conf,class,visibility
                out << globalFrame << ","
                    << box.trackId << ","
                    << static_cast<int>(box.rect.x()) << ","
                    << static_cast<int>(box.rect.y()) << ","
                    << static_cast<int>(box.rect.width()) << ","
                    << static_cast<int>(box.rect.height()) << ","
                    << box.confidence << ","
                    << classId << ","
                    << visibility << "\n";
            }
            globalFrame++;
        }
    }

    file.close();
    return true;
}
