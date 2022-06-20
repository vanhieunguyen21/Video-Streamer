
#include "Sink.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/timestamp.h"
#include "libavutil/channel_layout.h"
#include "pthread.h"
}
#include "mutex"
#include "unistd.h"

class DecodeStream {
public:
    int mStreamIdx = -1;
    AVCodecContext *mCodecCtx = nullptr;
    AVStream *mStream = nullptr;

    AVRational mTimebase = av_make_q(0, 1);
    // Video only attributes {
    int mWidth = 0, mHeight = 0;
    AVPixelFormat mPixFmt = AV_PIX_FMT_NONE;
    // } Video only attributes

    // Audio only attributes {
    int mSampleRate = 0;
    uint64_t mChannelLayout = 0;
    int mNbSamples = 0;
    AVSampleFormat mSampleFmt = AV_SAMPLE_FMT_NONE;
    // } Audio only attributes
};

enum class DemuxerState {
    INITIATE, READY, PAUSED, RUNNING, STOPPED
};

class Demuxer {
private:
public:
    AVFormatContext *mFmtCtx = nullptr;
    VideoSinkNode *mVideoSinks = nullptr;
    AudioSinkNode *mAudioSinks = nullptr;

    char *mUrl = nullptr;
    DecodeStream *mAudioStream = nullptr, *mVideoStream = nullptr;
    AVPacket *mPacket = nullptr;
    AVFrame *mFrame = nullptr;

    pthread_t mThread = 0;
    std::mutex mMutex;

    std::atomic<DemuxerState> mState = {DemuxerState::INITIATE};

    bool mHasDuration = 0;
    int64_t mDuration = 0;

private:
    void logPacket(AVPacket *pkt);

    void initiateDemuxer();

    /** Allocate codec context and stream index respective to given media type. */
    int openCodecContext(DecodeStream *dst, AVMediaType type);

    /** Decode given mPacket and output mFrame into mFrame field inside this class. */
    int decodePacket(DecodeStream *dst, AVPacket *pkt);

    /** Decode mPacket field from this class and output mFrame into mFrame field inside this class. */
    int decodePacket(DecodeStream *dst);

    static void *threadDemux(void *args);

public:
    Demuxer(const char *url);

    ~Demuxer();

    /** Return audio stream sample rate, 0 if failed to get sample rate. */
    int getSampleRate() const;

    /** Return audio stream channel layout, 0 if failed to get channel layout. */
    uint64_t getChannelLayout() const;

    /** Return audio stream number of samples per frame, 0 if failed to get number of samples per frame. */
    int getNbSamples() const;

    /** Return audio stream sample format, AV_SAMPLE_FMT_NONE if failed to get sample format. */
    AVSampleFormat getSampleFormat() const;

    /** Return audio stream timebase, default timebase if failed to get audio timebase. */
    AVRational getAudioTimebase() const;

    /** Return video stream width, 0 if failed to get width. */
    int getWidth() const;

    /** Return video stream height, 0 if failed to get height. */
    int getHeight() const;

    /** Return video stream pixel format, AV_PIX_FMT_NONE if failed to get pixel format. */
    AVPixelFormat getPixelFormat() const;

    /** Return video stream timebase, default timebase if failed to get video timebase. */
    AVRational getVideoTimebase() const;

    /** Return duration of media, 0 if media has no duration */
    int64_t getDuration() const;

    /** Add an audio destination where audio frames will be written into. */
    void addVideoSink(VideoSink *videoSink);

    /** Add a video destination where video frames will be written into. */
    void addAudioSink(AudioSink *audioSink);

    /** Remove an audio sink with id from audio sink list. */
    void removeVideoSink(int id);

    /** Remove a video sink with id from video sink list. */
    void removeAudioSink(int id);

    /** Remove all audio sinks and video sinks from demuxer. */
    void removeAllSinks();

    /** Start demuxing. Demuxer must be initiated and be ready. */
    void start();

    /** Seek to timestamp. Only usable if media has duration. */
    void seek(int64_t ts);

    /** Pause demuxer. Resume by calling resume(). */
    void pause();

    /** Resume demuxer from paused state. */
    void resume();

    /** Stop the demuxer and wait for demuxing thread to stop.
     * This method will block until thread is terminated. */
    void stop();

    /** Signal end of stream, flush any leftover frames inside buffer. */
    void flushDecoder(DecodeStream *dst);

    /** Release demuxer. Free any allocated storage. */
    void release();
};
