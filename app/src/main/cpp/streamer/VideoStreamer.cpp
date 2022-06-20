#include "VideoStreamer.h"
#include "JNILogHelper.h"

#define LOG_TAG "VideoStreamer"

VideoStreamer::VideoStreamer() {
    // Generate an unique id for this sink based on time
    mId = getCurrentTimeMs();
}

VideoStreamer::~VideoStreamer() {
    delete mFrameBuffer;
    sws_freeContext(mSwsCtx);
    if (mFrame) av_frame_free(&mFrame);
    if (mTmpFrame) av_frame_free(&mTmpFrame);
}

bool VideoStreamer::initiate() {
    // Validate output pixel format
    AVPixelFormat dstPixFmt = glPixFmtToAvPixFmt(mPixFmt);
    if (dstPixFmt == AV_PIX_FMT_NONE) {
        LOGE("Failed to initiate player, invalid picture format.");
        return false;
    }

    // Create frame buffer
    if (!createFrameBuffer()) return false;

    // Check if scaler is needed
    if (mSrcWidth == mWidth && mSrcHeight == mHeight && mSrcPixFmt == dstPixFmt) {
        LOGD("Input and output format matched, no scaler needed.");
    } else {
        // Create scaler
        int ret = createScaler();
        if (!ret) return false;
    }

    // Allocate frame to store data for rendering
    mFrame = FFmpegHelper::allocatePictureFrame(mWidth, mHeight, dstPixFmt);
    if (!mFrame) return false;

    return true;
}

bool VideoStreamer::createFrameBuffer() {
    AVPixelFormat dstPixFmt = glPixFmtToAvPixFmt(mPixFmt);
    if (dstPixFmt == AV_PIX_FMT_NONE) {
        LOGE("Failed to create frame buffer, invalid picture format.");
        return false;
    }
    mFrameBuffer = new FrameBuffer(128, mWidth, mHeight, dstPixFmt);
    return true;
}

bool VideoStreamer::createScaler() {
    LOGV("Creating scaler....");
    // Make sure all attributes are set
    if (mSrcWidth <= 0 || mSrcHeight <= 0 || mSrcPixFmt == AV_PIX_FMT_NONE ||
        mWidth <= 0 || mHeight <= 0) {
        LOGE("Failed to create scaler, invalid argument.");
        return false;
    }

    AVPixelFormat dstPixFmt = glPixFmtToAvPixFmt(mPixFmt);
    if (dstPixFmt == AV_PIX_FMT_NONE) {
        LOGE("Failed to create scaler, invalid picture format.");
        return false;
    }

    mSwsCtx = sws_getContext(mSrcWidth, mSrcHeight, mSrcPixFmt,
                             mWidth, mHeight, dstPixFmt,
                             SWS_BICUBIC,
                             nullptr, nullptr, nullptr);

    if (!mSwsCtx) {
        LOGE("Impossible to create scale context for the conversion fmt_:%s s:%dx%d -> fmt_:%s s:%dx%d",
             av_get_pix_fmt_name(mSrcPixFmt), mSrcWidth, mSrcHeight,
             av_get_pix_fmt_name(dstPixFmt), mWidth, mHeight);
        return false;
    }
    LOGD("Created scale context for the conversion fmt:%s s:%dx%d -> fmt:%s s:%dx%d",
         av_get_pix_fmt_name(mSrcPixFmt), mSrcWidth, mSrcHeight,
         av_get_pix_fmt_name(dstPixFmt), mWidth, mHeight);

    // Allocate frame for scaler output
    mTmpFrame = FFmpegHelper::allocatePictureFrame(mWidth, mHeight, dstPixFmt);
    if (!mTmpFrame) return false;

    return true;
}

AVPixelFormat VideoStreamer::glPixFmtToAvPixFmt(GLenum pPixFmt) {
    switch (pPixFmt) {
        case GL_RGB:
            return AV_PIX_FMT_RGB24;
        case GL_RGBA:
            return AV_PIX_FMT_RGBA;
        default:
            return AV_PIX_FMT_NONE;
    }
}

int VideoStreamer::onVideoFrame(AVFrame *srcFrame) {
    if (mFrameBuffer->isFull()) return false;

    AVFrame *frame = srcFrame;
    // Using scaler
    if (mSwsCtx) {
        // Scale frame to output format
        int ret = sws_scale_frame(mSwsCtx, mTmpFrame, srcFrame);
        if (ret < 0) {
            LOGE("Error scaling image: %s", av_err2str(ret));
            return 0;
        }
        frame = mTmpFrame;
        frame->pts = srcFrame->pts;
    }

    return mFrameBuffer->putFrame(frame);
}

void VideoStreamer::render() {
    if (!mFrameBuffer || !*mRenderer) return;

    // If time is presented, take the nearest frame close to current time
    // Otherwise just take whatever frame inside buffer
    if (mCurrentTsMs) {
        int64_t pts = msToPts(mCurrentTsMs->load(), mTimeBase);
        mFrameBuffer->takeFrame(mFrame, pts);
    } else {
        mFrameBuffer->takeFrame(mFrame);
    }

    (*mRenderer)->render(mWidth, mHeight, mPixFmt, mInternalPixFmt, mFrame->linesize[0], mFrame->data[0]);
}