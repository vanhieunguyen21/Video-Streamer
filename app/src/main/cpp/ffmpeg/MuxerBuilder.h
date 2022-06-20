#ifndef MUXER_BUILDER_H
#define MUXER_BUILDER_H
#include "Muxer.h"

class MuxerBuilder {
private:
    // Output file name
    char *mFileName = nullptr;

    // Default audio encoder
    const char *mDefaultAudioEncoder = "aac";
    // Default video encoder
    const char *mDefaultVideoEncoder = "libx264";

    // Audio encoder name
    char *mAudioEncoder = nullptr;
    // Video encoder name
    char *mVideoEncoder = nullptr;

    bool mHasAudio = 1;
    AVRational mAudioTimebase = av_make_q(0, 1);
    // Audio input attributes {
    int mSrcSampleRate = 0;
    uint64_t mSrcChannelLayout = 0;
    int mSrcNbChannels = 0;
    int mSrcNbSamples = 0;
    AVSampleFormat mSrcSampleFmt = AV_SAMPLE_FMT_NONE;
    // } Audio input attributes

    bool mHasVideo = 1;
    // Audio output attributes {
    int mDstSampleRate =0;
    uint64_t mDstChannelLayout = 0;
    int mDstNbChannels = 0;
    int mDstNbSamples = 0;
    AVSampleFormat mDstSampleFmt = AV_SAMPLE_FMT_FLTP;
    // } Audio output attributes

    AVRational mVideoTimebase = av_make_q(0, 1);
    // Video input attributes {
    int mSrcWidth = 0;
    int mSrcHeight = 0;
    AVPixelFormat mSrcPixFmt = AV_PIX_FMT_NONE;
    // } Video input attributes

    // Video output attributes {
    int mDstWidth = 0;
    int mDstHeight = 0;
    AVPixelFormat mDstPixFmt = AV_PIX_FMT_YUV420P;
    // } Video output attributes

public:
    MuxerBuilder();
    ~MuxerBuilder();

    /** Set output file name */
    MuxerBuilder *setFileName(const char *filename);

    /** Set audio encoder for muxer. Use default encoder if no encoder with name found. */
    MuxerBuilder *setAudioEncoder(const char *encoderName);
    /** Set video encoder for muxer. Use default encoder if no encoder with name found. */
    MuxerBuilder *setVideoEncoder(const char *encoderName);

    /** Disable or enable audio, default enable. */
    MuxerBuilder *setHasAudio(bool hasAudio);
    /** Disable or enable video, default enable. */
    MuxerBuilder *setHasVideo(bool hasVideo);

    /** Set audio time base */
    MuxerBuilder *setAudioTimeBase(AVRational timebase);

    /** Set input sample rate. */
    MuxerBuilder *setSrcSampleRate(int sampleRate);
    /** Set input channel layout. */
    MuxerBuilder *setSrcChannelLayout(uint64_t channelLayout);
    /** Set input number of samples per frame. */
    MuxerBuilder *setSrcNbSamples(int nbSamples);
    /** Set input sample format. */
    MuxerBuilder *setSrcSampleFmt(AVSampleFormat sampleFmt);

    /** Set output sample rate.
     * If this value is not set, use input value instead. */
    MuxerBuilder *setSampleRate(int sampleRate);
    /** Set output channel layout.
     * If this value is not set, use input value instead. */
    MuxerBuilder *setChannelLayout(uint64_t channelLayout);
    /** Set output number of samples per frame.
     * If this value is not set, use input value instead. */
    MuxerBuilder *setNbSamples(int nbSamples);
    /** Set output sample format.
     * If this value is not set, use default value instead. */
    MuxerBuilder *setSampleFmt(AVSampleFormat sampleFmt);

    /** Set video time base */
    MuxerBuilder *setVideoTimeBase(AVRational timebase);

    /** Set input picture width. */
    MuxerBuilder *setSrcWidth(int width);
    /** Set input picture height. */
    MuxerBuilder *setSrcHeight(int height);
    /** Set input picture format. */
    MuxerBuilder *setSrcPixelFormat(AVPixelFormat pixFmt);

    /** Set output picture width.
     * If this value is not set, use input value instead. */
    MuxerBuilder *setWidth(int width);
    /** Set output picture height.
     * If this value is not set, use input value instead. */
    MuxerBuilder *setHeight(int height);
    /** Set output picture format.
     * If this value is not set, use default value instead. */
    MuxerBuilder *setPixelFormat(AVPixelFormat pixFmt);

    /** Build muxer from given parameters.
     * @return muxer or nullptr if failed to build muxer */
     Muxer *buildMuxer();
};


#endif //MUXER_BUILDER_H
