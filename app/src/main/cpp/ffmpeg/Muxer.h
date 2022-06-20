#ifndef MUXER_H
#define MUXER_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavutil/timestamp.h"
#include "libavutil/imgutils.h"
#include "libavutil/channel_layout.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}
#include <cstdint>
#include "jni.h"
#include "Sink.h"
#include "SampleBuffer.h"
#include "FFmpegHelper.h"

class OutputStream {
public:
    AVStream *mStream;
    const AVCodec *mCodec;
    AVCodecContext *mCodecCtx;

    int64_t mStartTimestamp = 0;
    AVRational mTimeBase = AVRational {0, 0};
    int64_t mBitRate = 0;

    AVFrame *mFrame, *mTmpFrame;
    AVPacket *mPacket;

    // Video only attributes {
    SwsContext *mSwsCtx;
    int mSrcWidth, mSrcHeight, mDstWidth, mDstHeight;
    AVPixelFormat mSrcPixFmt = AV_PIX_FMT_NONE, mDstPixFmt = AV_PIX_FMT_NONE;
    // } Video only attributes

    // Audio only attributes {
    SwrContext *mSwrCtx;
    SampleBuffer *mSampleBuffer;
    uint64_t mSrcChannelLayout, mDstChannelLayout;
    int mSrcSampleRate, mSrcNbSamples, mDstSampleRate, mDstNbSamples;
    AVSampleFormat mSrcSampleFmt = AV_SAMPLE_FMT_NONE, mDstSampleFmt = AV_SAMPLE_FMT_NONE;
    // } Audio only attributes
};

enum class MuxerState {
    INITIATE, READY, STARTED, STOPPED
};

class Muxer : public VideoSink, public AudioSink {
    friend class MuxerBuilder;
private:
    char *mFileName = nullptr;
    char *mVideoEncoderName = nullptr;
    char *mAudioEncoderName = nullptr;

    AVFormatContext *mFmtCtx = nullptr;

public:
    OutputStream *mVideoSt = nullptr;
    OutputStream *mAudioSt = nullptr;
    bool mHasVideo = true, mHasAudio = true;

    MuxerState mState = MuxerState::INITIATE;

    int mIsStarted = 0, mIsStopped = 0;
    int mIsScalerCreated = 0, mIsResamplerCreated = 0;

private:
    Muxer(const char *fileName, const char *audioEncoder, const char *videoEncoder);

    /** Initialize the muxer.
     * @return true if success, false if failure */
    bool initiate();

    void logPacket(AVPacket *pkt);

    /** Add audio or video stream to the muxer.
     * @return 1 if added successfully, 0 otherwise */
    int addStream(OutputStream *ost);

    /** Allocate an audio mFrame based on parameters set in this class.
     * @return allocated mFrame or null if allocation failed */
    AVFrame *allocateAudioFrame();

    /** Allocate a video mFrame based on parameters set in this class.
     * @return allocated mFrame or null if allocation failed */
    AVFrame *allocatePictureFrame();

    /** Set up audio output.
     * @return 0 if failed to open, 1 if successfully opened */
    int openAudio(OutputStream *ost);

    /** Set up video output.
     * @return 0 if failed to open, 1 if successfully opened */
    int openVideo(OutputStream *ost);

    /** Write an AVFrame to output with data inside an OutputStream and custom AVFrame.
     * @return 1 if mFrame written, 0 if failed */
    int writeFrame(OutputStream *ost, AVFrame *frame);

    /** Write an AVFrame to output with data inside an OutputStream.
     * @return 1 if mFrame written, 0 if failed */
    int writeFrame(OutputStream *ost);

    /** Flush leftover packets from a codec inside an OutputStream. */
    int flushCodec(OutputStream *ost);

    void closeStream(OutputStream *ost);

public:
    ~Muxer();

    /** Validate if all needed video params are properly set. */
//    int validate_video_params();

    /** Validate if all needed audio params are properly set. */
//    int validate_audio_params();

    /** Validate if all params are properly set.= */
//    int validate_params();

    /** Create a scaler to convert video mFrame from src format to dst format
     * These params must be set:
     * mVideoSt: mSrcWidth, mSrcHeight, mDstWidth, mDstHeight, mSrcPixFmt, _dst_pix_fmt
     * @param flag: Scale algorithm to use */
    int createScaler(int flag);

    /** Create a scaler using default scale algorithm: SWS_BICUBIC
     * @see createScaler(int flag) */
    int createScaler();

    /** Create a resampler to convert audio from source format to destination format.
     * These params must be set:
     * mAudioSt: src_channels, mSrcSampleRate, mSrcNbSamples, mSrcSampleFmt
     * mDstSampleFmt, mDstChannelLayout, mDstSampleRate, mDstNbSamples; */
    int createResampler();

    int start();

    /** Given video frame data from input, convert it using scaler and write it to muxer. */
    int writeVideoFrame(uint8_t *srcData, int64_t frameTimestamp);

    /** Given audio frame data from input, convert it using resampler, put it inside sample buffer
     * and write to muxer if a full frame after conversion if available. */
    int writeAudioFrame(uint8_t *buffer, int offset, int nbSamples, int64_t frameTimestamp);

    /** Callback, will be called if an input video frame is available. */
    int onVideoFrame(AVFrame *frame);

    /** Callback, will be called if an input audio frame is available. */
    int onAudioFrame(AVFrame *frame);

    void stop();

    void release();
};

#endif // MUXER_H