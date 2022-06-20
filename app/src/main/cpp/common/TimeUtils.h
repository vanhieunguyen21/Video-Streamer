#ifndef TIME_UTILS_H
#define TIME_UTILS_H

extern "C" {
#include <libavutil/rational.h>
#include <libavutil/mathematics.h>
}

#include "chrono"

int64_t getCurrentTimeMs();

int64_t ptsToMs(int64_t pts, AVRational timebase);

int64_t msToPts(int64_t ms, AVRational timebase);

int64_t getFrameTimeMs(int64_t startTimestamp, int64_t startPts, int64_t currPts, AVRational timebase);

int64_t getFrameTimePts(int64_t startTimestamp, int64_t currentTimestamp, AVRational timebase);

#endif //TIME_UTILS_H
