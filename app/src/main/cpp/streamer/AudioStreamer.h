#ifndef AUDIO_STREAMER_H
#define AUDIO_STREAMER_H

extern "C" {
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
}

#include "TimeUtils.h"
#include "../ffmpeg/FFmpegHelper.h"
#include "oboe/Oboe.h"
#include "Sink.h"
#include "mutex"
#include "FrameBuffer.h"


class AudioStreamer : public AudioSink, public oboe::AudioStreamCallback {
    friend class AudioStreamerBuilder;
    friend class MediaStreamerBuilder;

private:
    // Resampler to convert source frame to desired format
    SwrContext *mSwrCtx = nullptr;
    // Flag to check if frame is consumed
    std::atomic_bool mIsFrameConsumed = {true};
    // Storage for output frame
    AVFrame *mFrame = nullptr;
    // Storage for resampled frame
    AVFrame *mTmpFrame = nullptr;
    // Mutex to prevent reading and writing data to frame at the same time
    std::mutex mMutex;
    // Frame buffer for buffering
    FrameBuffer *mFrameBuffer;
    // Current time of stream in millis
    std::atomic_int64_t *mCurrentTsMs = nullptr;


    AVRational mTimeBase = {0, 1};
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
    oboe::AudioFormat mSampleFmt = oboe::AudioFormat::I16; // Output sample format
    int mAudioBufferSize = 0; // Size in byte per buffer
    // } Audio output params

    oboe::ManagedStream mOutStream; // Output stream

private:
    bool initiate();

    /** Create frame buffer and allocate resources to it */
    bool createFrameBuffer();

    /** Create a resampler to convert audio from input format to output format. */
    bool createResampler();

    /** Return equivalent ffmpeg AVSampleFormat given oboe AudioFormat. */
    static AVSampleFormat oboeSampleFmtToAvSampleFmt(oboe::AudioFormat sampleFmt);

public:
    AudioStreamer();

    ~AudioStreamer();

    void pause();

    void resume();

    /** Callback, will be called when there is an incoming frame from decoder.
     * Incoming frames will be converted with resampler and stored in frame buffer. */
    int onAudioFrame(AVFrame *srcFrame) override;

    /** Callback, will be called when stream needs more audio data.
     * This will try to take a frame from frame buffer, return silence if no frame returns. */
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override;
};


#endif //AUDIO_STREAMER_H
