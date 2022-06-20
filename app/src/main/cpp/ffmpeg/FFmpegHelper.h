#ifndef FFMPEG_FRAME_HELPER_H
#define FFMPEG_FRAME_HELPER_H

extern "C" {
#include "libavutil/frame.h"
#include "libavutil/pixdesc.h"
#include "libavutil/channel_layout.h"
}

namespace FFmpegHelper {

    /** Allocate a picture frame based on parameters provided
    * @return allocated frame or null if allocation failed */
    AVFrame *allocatePictureFrame(int width, int height, AVPixelFormat pixFmt);

    /** Allocate an audio frame based on parameters provided
    * @return allocated frame or null if allocation failed */
    AVFrame *allocateAudioFrame(int sampleRate, uint64_t channelLayout, int nbSamples, enum AVSampleFormat sampleFmt);

}

#endif // FFMPEG_FRAME_HELPER_H
