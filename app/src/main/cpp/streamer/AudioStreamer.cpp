#include "AudioStreamer.h"
#include "JNILogHelper.h"

#define LOG_TAG "AudioStreamer"

AudioStreamer::AudioStreamer() {
    // Generate an unique id for this sink based on time
    mId = getCurrentTimeMs();
}

AudioStreamer::~AudioStreamer() {
    delete mFrameBuffer;
    if (mSwrCtx) swr_free(&mSwrCtx);
    if (mFrame) av_frame_free(&mFrame);
    if (mTmpFrame) av_frame_free(&mTmpFrame);

    mOutStream->stop();
    mOutStream->close();
}

bool AudioStreamer::initiate() {
    // Validate output sample format
    AVSampleFormat dstSampleFmt = oboeSampleFmtToAvSampleFmt(mSampleFmt);
    if (dstSampleFmt == AV_SAMPLE_FMT_NONE) {
        LOGE("Failed to create initiate streamer, invalid sample format.");
        return false;
    }

    oboe::AudioStreamBuilder builder;
    builder.setFramesPerDataCallback(mNbSamples);
    builder.setSampleRate(mSampleRate);
    builder.setChannelCount(mNbChannels);
    builder.setFormat(mSampleFmt);
    builder.setDirection(oboe::Direction::Output);
    builder.setCallback(this);
    builder.openManagedStream(mOutStream);

    auto result = mOutStream->requestStart();
    if (result != oboe::Result::OK) {
        LOGE("Failed to initiate oboe audio player, error %d", result);
        return false;
    }

    // Create frame buffer
    if (!createFrameBuffer()) return false;

    // Check if resampler is needed
    if (mSrcSampleRate == mSampleRate && mSrcChannelLayout == mChannelLayout &&
        mSrcNbChannels == mNbChannels && mSrcSampleFmt == dstSampleFmt && mSrcNbSamples == mNbSamples) {
        LOGD("Input and output format matched, no resampler needed.");
    } else {
        // Create resampler
        int ret = createResampler();
        if (!ret) return false;
    }

    // Allocate audio frame for audio output
    mFrame = FFmpegHelper::allocateAudioFrame(mSampleRate, mChannelLayout, mNbSamples, dstSampleFmt);
    if (!mFrame) return false;

    // Calculate buffer size
    mAudioBufferSize = av_samples_get_buffer_size(mFrame->linesize, mNbChannels, mNbSamples, dstSampleFmt, 0);

    return true;
}

bool AudioStreamer::createFrameBuffer() {
    AVSampleFormat dstSampleFmt = oboeSampleFmtToAvSampleFmt(mSampleFmt);
    if (dstSampleFmt == AV_SAMPLE_FMT_NONE) {
        LOGE("Failed to create frame buffer, invalid sample format.");
        return false;
    }
    mFrameBuffer = new FrameBuffer(256, mChannelLayout, mNbSamples, dstSampleFmt);
    return true;
}

bool AudioStreamer::createResampler() {
    LOGV("Creating resampler...");
    int ret;
    // Make sure all attributes are set
    if (mSrcSampleRate == 0 || mSrcChannelLayout == 0 || mSrcNbSamples == 0 || mSrcSampleFmt == AV_SAMPLE_FMT_NONE ||
        mSampleRate == 0 || mChannelLayout == 0 || mNbSamples == 0 || mSampleFmt == oboe::AudioFormat::Unspecified) {
        LOGE("Failed to create resampler, missing or invalid parameters.");
        return false;
    }

    AVSampleFormat dstSampleFmt = oboeSampleFmtToAvSampleFmt(mSampleFmt);
    if (dstSampleFmt == AV_SAMPLE_FMT_NONE) {
        LOGE("Failed to create resampler, invalid sample format.");
        return false;
    }

    mSwrCtx = swr_alloc();
    if (!mSwrCtx) {
        LOGE("Could not allocate resampler context.");
        return false;
    }

    // Set swr options
    av_opt_set_int(mSwrCtx, "in_channel_count", mSrcNbChannels, 0);
    av_opt_set_int(mSwrCtx, "in_sample_rate", mSrcSampleRate, 0);
    av_opt_set_sample_fmt(mSwrCtx, "in_sample_fmt", mSrcSampleFmt, 0);
    av_opt_set_int(mSwrCtx, "out_channel_count", mNbChannels, 0);
    av_opt_set_int(mSwrCtx, "out_sample_rate", mSampleRate, 0);
    av_opt_set_sample_fmt(mSwrCtx, "out_sample_fmt", dstSampleFmt, 0);

    // Initialize the resampling context
    if ((ret = swr_init(mSwrCtx)) < 0) {
        LOGE("Failed to initialize the resampling context: %s", av_err2str(ret));
        return false;
    }

    LOGV("Resampler created for conversion %d %d %s -> %d %d %s",
         mSrcNbChannels, mSrcSampleRate, av_get_sample_fmt_name(mSrcSampleFmt),
         mNbChannels, mSrcSampleRate, av_get_sample_fmt_name(dstSampleFmt));

    // Allocate frame for resampler output
    mTmpFrame = FFmpegHelper::allocateAudioFrame(mSampleRate, mChannelLayout, mNbSamples, dstSampleFmt);
    if (!mTmpFrame) return false;

    return true;
}


AVSampleFormat AudioStreamer::oboeSampleFmtToAvSampleFmt(oboe::AudioFormat sampleFmt) {
    switch (sampleFmt) {
        case oboe::AudioFormat::I16:
            return AV_SAMPLE_FMT_S16;
        case oboe::AudioFormat::I32:
            return AV_SAMPLE_FMT_S32;
        case oboe::AudioFormat::Float:
            return AV_SAMPLE_FMT_FLT;
        default:
            return AV_SAMPLE_FMT_NONE;
    }
}

void AudioStreamer::pause() {
    mOutStream->pause();
}

void AudioStreamer::resume() {
    mOutStream->start();
}

int AudioStreamer::onAudioFrame(AVFrame *srcFrame) {
    if (mFrameBuffer->isFull()) return false;

    AVFrame *frame = srcFrame;
    // Using resampler
    if (mSwrCtx) {
        // Resample frame to output format
        int ret = swr_convert_frame(mSwrCtx, mTmpFrame, srcFrame);
        if (ret < 0) {
            LOGE("Could not resample frame: %s", av_err2str(ret));
            return 0;
        }
        frame = mTmpFrame;
        frame->pts = srcFrame->pts;
    }

    return mFrameBuffer->putFrame(frame);
}

oboe::DataCallbackResult AudioStreamer::onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) {
    auto *data = (int16_t *) audioData;

    if (!mFrameBuffer) {
        // Fill with silence
        std::fill_n(data, numFrames * mNbChannels, 0);
        return oboe::DataCallbackResult::Continue;
    }

    bool ret = mFrameBuffer->takeFrame(mFrame);

    if (!ret) {
        // Fill with silence
        std::fill_n(data, numFrames * mNbChannels, 0);
    } else {
        // Set current time
        int64_t ms = ptsToMs(mFrame->pts, mTimeBase);
        if (mCurrentTsMs) mCurrentTsMs->store(ms);
        // Copy samples
        memcpy(data, mFrame->data[0], mAudioBufferSize);
        mIsFrameConsumed = true;
    }
    return oboe::DataCallbackResult::Continue;
}