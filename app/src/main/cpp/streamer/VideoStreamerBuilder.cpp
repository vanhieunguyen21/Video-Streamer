#include "VideoStreamerBuilder.h"
#include "JNILogHelper.h"
#define LOG_TAG "VideoStreamerBuilder"

VideoStreamerBuilder *VideoStreamerBuilder::setRenderer(RendererES3 **renderer) {
    mRenderer = renderer;
    return this;
}

VideoStreamerBuilder *VideoStreamerBuilder::setVideoTimeBase(AVRational timebase) {
    mVideoTimeBase = timebase;
    return this;
}

VideoStreamerBuilder *VideoStreamerBuilder::setSrcWidth(int width) {
    mSrcWidth = width;
    return this;
}

VideoStreamerBuilder *VideoStreamerBuilder::setSrcHeight(int height) {
    mSrcHeight = height;
    return this;
}

VideoStreamerBuilder *VideoStreamerBuilder::setSrcPixelFormat(AVPixelFormat pixFmt) {
    mSrcPixFmt = pixFmt;
    return this;
}

VideoStreamerBuilder *VideoStreamerBuilder::setWidth(int width) {
    mWidth = width;
    return this;
}

VideoStreamerBuilder *VideoStreamerBuilder::setHeight(int height) {
    mHeight = height;
    return this;
}

VideoStreamerBuilder *VideoStreamerBuilder::setPixelFormat(GLenum pixFmt) {
    mPixFmt = pixFmt;
    return this;
}

VideoStreamerBuilder *VideoStreamerBuilder::setInternalPixelFormat(GLint internalPixFmt) {
    mInternalPixFmt = internalPixFmt;
    return this;
}

VideoStreamer *VideoStreamerBuilder::buildVideoStreamer() {
    // Validate all parameters
    if (mSrcWidth <= 0 || mSrcHeight <= 0 || mSrcPixFmt == AV_PIX_FMT_NONE) {
        LOGE("Failed to create video streamer. Missing or invalid video params.");
        return nullptr;
    }
    // Create a video streamer
    auto *videoStreamer = new VideoStreamer();
    // Assign attributes
    videoStreamer->mRenderer = mRenderer;
    videoStreamer->mTimeBase = mVideoTimeBase;

    videoStreamer->mSrcWidth = mSrcWidth;
    videoStreamer->mSrcHeight = mSrcHeight;
    videoStreamer->mSrcPixFmt = mSrcPixFmt;

    videoStreamer->mWidth = (mWidth <= 0) ? mSrcWidth : mWidth;
    videoStreamer->mHeight = (mHeight <= 0) ? mSrcHeight : mHeight;
    videoStreamer->mPixFmt = mPixFmt;
    videoStreamer->mInternalPixFmt = mInternalPixFmt;

    // Initiate video streamer
    int ret = videoStreamer->initiate();
    if (!ret) {
        LOGE("Failed to initiate video player.");
        delete videoStreamer;
        return nullptr;
    }

    return videoStreamer;
}






