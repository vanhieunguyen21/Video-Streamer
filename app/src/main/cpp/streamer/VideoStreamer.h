#ifndef VIDEO_STREAMER_H
#define VIDEO_STREAMER_H

extern "C" {
#include "libswscale/swscale.h"
#include "libavutil/pixdesc.h"
}

#include "TimeUtils.h"
#include "../ffmpeg/FFmpegHelper.h"
#include "Sink.h"
#include "../gles/RendererES3.h"
#include "mutex"
#include "FrameBuffer.h"

class VideoStreamer : public VideoSink {
    friend class VideoStreamerBuilder;
    friend class MediaStreamerBuilder;

private:
    // Scaler to convert source frame into desired format
    SwsContext *mSwsCtx = nullptr;
    // Storage for output frame
    AVFrame *mFrame = nullptr;
    // Storage for scaled frame
    AVFrame *mTmpFrame = nullptr;
    // Mutex to prevent reading and writing data to frame at the same time
    std::mutex mMutex;
    // Frame buffer for buffering
    FrameBuffer *mFrameBuffer;
    // Current time of stream in millis
    std::atomic_int64_t *mCurrentTsMs = nullptr;

    // OpenGLES Renderer
    RendererES3 **mRenderer;

    AVRational mTimeBase = {0, 1};
    // Video input params {
    int mSrcWidth = 0, mSrcHeight = 0;
    AVPixelFormat mSrcPixFmt = AV_PIX_FMT_NONE;
    // } Video input params

    // Video output params {
    int mWidth = 0, mHeight = 0;
    GLenum mPixFmt = GL_RGB; // Output pixel format of video
    GLint mInternalPixFmt = GL_RGB; // Pixel format of video stored inside the buffer
    // } Video output params
public:

private:

    bool initiate();

    /** Create frame buffer and allocate resources to it */
    bool createFrameBuffer();

    /** Create a scaler to convert picture from input format to output format. */
    bool createScaler();

    /** Return equivalent ffmpeg AVPixelFormat given gl pixel format. */
    static AVPixelFormat glPixFmtToAvPixFmt(GLenum pPixFmt);

public:
    VideoStreamer();

    ~VideoStreamer();

    /** Callback, will be called when there is an incoming frame from decoder.
     * Incoming frames will be converted with scaler and stored in frame buffer. */
    int onVideoFrame(AVFrame *srcFrame) override;

    /** Callback function. Will be called when surface needs a new frame.
     * This will try to take a frame from buffer. If there is no frame pulled out
     * from buffer, re-draw the most recent frame. */
    void render();
};


#endif //VIDEO_STREAMER_H
