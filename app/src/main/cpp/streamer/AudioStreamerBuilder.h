#ifndef AUDIO_STREAMER_BUILDER_H
#define AUDIO_STREAMER_BUILDER_H
#include "AudioStreamer.h"

class AudioStreamerBuilder {
protected:

    AVRational mAudioTimebase = {0, 1};
    // Audio input params {
    int mSrcSampleRate = 0;
    uint64_t mSrcChannelLayout = 0;
    int mSrcNbChannels = 0;
    int mSrcNbSamples = 0;
    AVSampleFormat mSrcSampleFmt = AV_SAMPLE_FMT_NONE;
    // } Audio input params

    // Audio output params {
    int mSampleRate = 0;
    uint64_t mChannelLayout = 0;
    int mNbChannels = 0;
    int mNbSamples = 0;
    oboe::AudioFormat mSampleFmt = oboe::AudioFormat::I16;
    // } Audio output params
public:
    /** Set audio stream time base. */
    AudioStreamerBuilder *setAudioTimeBase(AVRational timebase);
    /** Set input sample rate. */
    AudioStreamerBuilder *setSrcSampleRate(int sampleRate);
    /** Set input channel layout. */
    AudioStreamerBuilder *setSrcChannelLayout(uint64_t channelLayout);
    /** Set input number of samples per frame. */
    AudioStreamerBuilder *setSrcNbSamples(int nbSamples);
    /** Set input sample format. */
    AudioStreamerBuilder *setSrcSampleFmt(AVSampleFormat sampleFmt);

    /** Set output sample rate.
     * If this value is not set, use input value instead. */
    AudioStreamerBuilder *setSampleRate(int sampleRate);
    /** Set output channel layout.
     * If this value is not set, use input value instead. */
    AudioStreamerBuilder *setChannelLayout(uint64_t channelLayout);
    /** Set output number of samples per frame.
     * If this value is not set, use input value instead. */
    AudioStreamerBuilder *setNbSamples(int nbSamples);
    /** Set output sample format.
     * If this value is not set, use default value instead. */
    AudioStreamerBuilder *setSampleFmt(oboe::AudioFormat sampleFmt);

    /** Build audio streamer from given parameters.
     * @return steamer or nullptr if failed to build streamer */
    AudioStreamer *buildAudioStreamer();
};


#endif //AUDIO_STREAMER_BUILDER_H
