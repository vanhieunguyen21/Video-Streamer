#include "AudioStreamerBuilder.h"
#include "JNILogHelper.h"
#define LOG_TAG "AudioStreamerBuilder"

AudioStreamerBuilder *AudioStreamerBuilder::setAudioTimeBase(AVRational timebase) {
    mAudioTimebase = timebase;
    return this;
}

AudioStreamerBuilder *AudioStreamerBuilder::setSrcSampleRate(int sampleRate) {
    mSrcSampleRate = sampleRate;
    return this;
}

AudioStreamerBuilder *AudioStreamerBuilder::setSrcChannelLayout(uint64_t channelLayout) {
    mSrcChannelLayout = channelLayout;
    mSrcNbChannels = av_get_channel_layout_nb_channels(channelLayout);
    return this;
}

AudioStreamerBuilder *AudioStreamerBuilder::setSrcNbSamples(int nbSamples) {
    mSrcNbSamples = nbSamples;
    return this;
}

AudioStreamerBuilder *AudioStreamerBuilder::setSrcSampleFmt(AVSampleFormat sampleFmt) {
    mSrcSampleFmt = sampleFmt;
    return this;
}

AudioStreamerBuilder *AudioStreamerBuilder::setSampleRate(int sampleRate) {
    mSampleRate = sampleRate;
    return this;
}

AudioStreamerBuilder *AudioStreamerBuilder::setChannelLayout(uint64_t channelLayout) {
    mChannelLayout = channelLayout;
    mNbChannels = av_get_channel_layout_nb_channels(channelLayout);
    return this;
}

AudioStreamerBuilder *AudioStreamerBuilder::setNbSamples(int nbSamples) {
    mNbSamples = nbSamples;
    return this;
}

AudioStreamerBuilder *AudioStreamerBuilder::setSampleFmt(oboe::AudioFormat sampleFmt) {
    mSampleFmt = sampleFmt;
    return this;
}

AudioStreamer *AudioStreamerBuilder::buildAudioStreamer() {
    // Validate all parameters
    if (mSrcSampleRate <= 0 || mSrcChannelLayout == 0 || mSrcNbSamples <= 0 ||
        mSrcSampleFmt == AV_SAMPLE_FMT_NONE) {
        LOGE("Failed to create audio streamer. Missing or invalid audio params.");
        return nullptr;
    }
    // Create audio streamer
    auto *audioStreamer = new AudioStreamer();
    // Assign attributes
    audioStreamer->mTimeBase = mAudioTimebase;

    audioStreamer->mSrcSampleRate = mSrcSampleRate;
    audioStreamer->mSrcChannelLayout = mSrcChannelLayout;
    audioStreamer->mSrcNbChannels = mSrcNbChannels;
    audioStreamer->mSrcNbSamples = mSrcNbSamples;
    audioStreamer->mSrcSampleFmt = mSrcSampleFmt;

    audioStreamer->mSampleRate = (mSampleRate <= 0) ? mSrcSampleRate : mSampleRate;
    audioStreamer->mChannelLayout = (mChannelLayout == 0) ? mSrcChannelLayout : mChannelLayout;
    audioStreamer->mNbChannels = (mNbChannels <= 0) ? mSrcNbChannels : mNbChannels;
    audioStreamer->mNbSamples = (mNbSamples <= 0) ? mSrcNbSamples : mNbSamples;
    audioStreamer->mSampleFmt = (mSampleFmt == oboe::AudioFormat::Unspecified) ? oboe::AudioFormat::I16 : mSampleFmt;

    int ret = audioStreamer->initiate();
    if (!ret) {
        LOGE("Failed to initiate audio player.");
        delete audioStreamer;
        return nullptr;
    }

    return audioStreamer;
}