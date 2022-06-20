#include "TimeUtils.h"

int64_t getCurrentTimeMs() {
    using namespace std::chrono;
    // Get current time with native precision
    auto now = system_clock::now();
    // Convert time_point to signed integral type
    auto integralDuration = duration_cast<milliseconds>(now.time_since_epoch()).count();
    return integralDuration;
}

int64_t ptsToMs(int64_t pts, AVRational timebase) {
    return av_rescale(pts * 1000, timebase.num, timebase.den);
}

int64_t msToPts(int64_t ms, AVRational timebase) {
    return av_rescale(ms, timebase.den, timebase.num * 1000);
}

int64_t getFrameTimeMs(int64_t startTimestamp, int64_t startPts, int64_t currPts, AVRational timebase) {
    int64_t pts = currPts - startPts;
    return startTimestamp + av_rescale(pts * 1000, timebase.num, timebase.den);
}

int64_t getFrameTimePts(int64_t startTimestamp, int64_t currentTimestamp, AVRational timebase) {
    int64_t timeDiff = currentTimestamp - startTimestamp;
    return av_rescale(timeDiff, timebase.den, timebase.num * 1000);
}