#include "MuxerBuilder.h"
#include "JNILogHelper.h"

#define LOG_TAG "MuxerBuilder"

MuxerBuilder::MuxerBuilder() {}

MuxerBuilder::~MuxerBuilder() {
    delete[] mFileName;
    delete[] mAudioEncoder;
    delete[] mVideoEncoder;
}

MuxerBuilder *MuxerBuilder::setFileName(const char *filename) {
    delete[] mFileName;
    mFileName = new char[strlen(filename) + 1];
    strcpy(mFileName, filename);
    return this;
}

MuxerBuilder *MuxerBuilder::setAudioEncoder(const char *encoderName) {
    delete[] mAudioEncoder;
    mAudioEncoder = new char[strlen(encoderName) + 1];
    strcpy(mAudioEncoder, encoderName);
    return this;
}

MuxerBuilder *MuxerBuilder::setVideoEncoder(const char *encoderName) {
    delete[] mVideoEncoder;
    mVideoEncoder = new char[strlen(encoderName) + 1];
    strcpy(mVideoEncoder, encoderName);
    return this;
}

MuxerBuilder *MuxerBuilder::setHasAudio(bool hasAudio) {
    mHasAudio = hasAudio;
    return this;
}

MuxerBuilder *MuxerBuilder::setHasVideo(bool hasVideo) {
    mHasVideo = hasVideo;
    return this;
}

MuxerBuilder *MuxerBuilder::setAudioTimeBase(AVRational timebase) {
    mAudioTimebase = timebase;
    return this;
}

MuxerBuilder *MuxerBuilder::setSrcSampleRate(int sampleRate) {
    mSrcSampleRate = sampleRate;
    return this;
}

MuxerBuilder *MuxerBuilder::setSrcChannelLayout(uint64_t channelLayout) {
    mSrcChannelLayout = channelLayout;
    mSrcNbChannels = av_get_channel_layout_nb_channels(channelLayout);
    return this;
}

MuxerBuilder *MuxerBuilder::setSrcNbSamples(int nbSamples) {
    mSrcNbSamples = nbSamples;
    return this;
}

MuxerBuilder *MuxerBuilder::setSrcSampleFmt(AVSampleFormat sampleFmt) {
    mSrcSampleFmt = sampleFmt;
    return this;
}

MuxerBuilder *MuxerBuilder::setSampleRate(int sampleRate) {
    mDstSampleRate = sampleRate;
    return this;
}

MuxerBuilder *MuxerBuilder::setChannelLayout(uint64_t channelLayout) {
    mDstChannelLayout = channelLayout;
    mDstNbChannels = av_get_channel_layout_nb_channels(channelLayout);
    return this;
}

MuxerBuilder *MuxerBuilder::setNbSamples(int nbSamples) {
    mDstNbSamples = nbSamples;
    return this;
}

MuxerBuilder *MuxerBuilder::setSampleFmt(AVSampleFormat sampleFmt) {
    mDstSampleFmt = sampleFmt;
    return this;
}

MuxerBuilder *MuxerBuilder::setVideoTimeBase(AVRational timebase) {
    mVideoTimebase = timebase;
    return this;
}

MuxerBuilder *MuxerBuilder::setSrcWidth(int width) {
    mSrcWidth = width;
    return this;
}

MuxerBuilder *MuxerBuilder::setSrcHeight(int height) {
    mSrcHeight = height;
    return this;
}

MuxerBuilder *MuxerBuilder::setSrcPixelFormat(AVPixelFormat pixFmt) {
    mSrcPixFmt = pixFmt;
    return this;
}

MuxerBuilder *MuxerBuilder::setWidth(int width) {
    mDstWidth = width;
    return this;
}

MuxerBuilder *MuxerBuilder::setHeight(int height) {
    mDstHeight = height;
    return this;
}

MuxerBuilder *MuxerBuilder::setPixelFormat(AVPixelFormat pixFmt) {
    mDstPixFmt = pixFmt;
    return this;
}

Muxer *MuxerBuilder::buildMuxer() {
    // Validate file name
    if (mFileName == nullptr) {
        LOGE("No output file.");
        return nullptr;
    }

    // Validate audio parameters
    if (mSrcSampleRate <= 0 || mSrcChannelLayout == 0 || mSrcNbChannels == 0 ||
        mSrcNbSamples == 0 || mSrcSampleFmt == AV_SAMPLE_FMT_NONE) {
        LOGE("Failed to create muxer. Missing or invalid audio params.");
        return nullptr;
    }

    // Validate all parameters
    if (mSrcWidth <= 0 || mSrcHeight <= 0 || mSrcPixFmt == AV_PIX_FMT_NONE) {
        LOGE("Failed to create muxer. Missing or invalid video params.");
        return nullptr;
    }

    // Create muxer
    const char *audioEncoder = (mAudioEncoder == nullptr) ? mDefaultAudioEncoder : mAudioEncoder;
    const char *videoEncoder = (mVideoEncoder == nullptr) ? mDefaultVideoEncoder : mVideoEncoder;

    auto *muxer = new Muxer(mFileName, audioEncoder, videoEncoder);
    muxer->mHasAudio = mHasAudio;
    muxer->mHasVideo = mHasVideo;

    // Initiate audio stream
    if (mHasAudio) {
        auto *audioSt = new OutputStream();
        muxer->mAudioSt = audioSt;

        audioSt->mTimeBase = mAudioTimebase;

        audioSt->mSrcSampleRate = mSrcSampleRate;
        audioSt->mSrcChannelLayout = mSrcChannelLayout;
        audioSt->mSrcSampleFmt = mSrcSampleFmt;
        audioSt->mSrcNbSamples = mSrcNbSamples;

        audioSt->mDstSampleRate = (mDstSampleRate <= 0) ? mSrcSampleRate : mDstSampleRate;
        audioSt->mDstChannelLayout = (mDstChannelLayout == 0) ? mSrcChannelLayout : mDstChannelLayout;
        audioSt->mDstSampleFmt = mDstSampleFmt;
        audioSt->mDstNbSamples = (mDstNbSamples <= 0) ? mSrcNbSamples : mDstNbSamples;
    }

    // Initiate video stream
    if (mHasVideo) {
        auto *videoSt = new OutputStream();
        muxer->mVideoSt = videoSt;

        videoSt->mTimeBase = mVideoTimebase;

        videoSt->mSrcWidth = mSrcWidth;
        videoSt->mSrcHeight = mSrcHeight;
        videoSt->mSrcPixFmt = mSrcPixFmt;

        videoSt->mDstWidth = (mDstWidth <= 0) ? mSrcWidth : mDstWidth;
        videoSt->mDstHeight = (mDstHeight <= 0) ? mSrcHeight : mDstHeight;
        videoSt->mDstPixFmt = mDstPixFmt;
    }

    bool ret = muxer->initiate();
    if (!ret) {
        delete muxer;
        return nullptr;
    }

    return muxer;
}