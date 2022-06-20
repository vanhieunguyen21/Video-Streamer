#include "FFmpegHelper.h"
#include "../common/JNILogHelper.h"

#define LOG_TAG "FrameHelper"

AVFrame *FFmpegHelper::allocatePictureFrame(int width, int height, AVPixelFormat pixFmt) {
    LOGD("Allocating picture frame: (%d, %d, %s)", width, height, av_get_pix_fmt_name(pixFmt));
    AVFrame *frame;
    int ret;

    frame = av_frame_alloc();
    if (!frame) {
        LOGE("Could not allocate mFrame.");
        return nullptr;
    }

    frame->format = pixFmt;
    frame->width = width;
    frame->height = height;

    /* allocate the buffers for the mFrame data */
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        LOGE("Could not allocate mFrame data: %s", av_err2str(ret));
        return nullptr;
    }
    return frame;
}

AVFrame *FFmpegHelper::allocateAudioFrame(int sampleRate, uint64_t channelLayout, int nbSamples, enum AVSampleFormat sampleFmt) {
    LOGD("Allocating audio frame: (%d, %d, %d, %s)",
         sampleRate, av_get_channel_layout_nb_channels(channelLayout),
         nbSamples, av_get_sample_fmt_name(sampleFmt));

    int ret;
    AVFrame *frame;
    frame = av_frame_alloc();

    frame->format = sampleFmt;
    frame->channel_layout = channelLayout;
    frame->sample_rate = sampleRate;
    frame->nb_samples = nbSamples;

    if (nbSamples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            LOGE("Error allocating an audio buffer: %s", av_err2str(ret));
            return nullptr;
        }
    }

    return frame;
}