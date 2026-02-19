#pragma once

#include <QString>
#include <vector>

struct ResultSegment;
struct LabelDef;

class MotExporter {
public:
    // Export all segments to MOT format CSV
    // Format: frame,trackId,x,y,w,h,conf,classId,visibility
    static bool exportToFile(const QString& filePath,
                             const std::vector<ResultSegment>& segments,
                             const std::vector<LabelDef>& labels);
};
