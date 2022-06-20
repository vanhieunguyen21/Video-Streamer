#include "FrameBuffer.h"
#include "../common/JNILogHelper.h"

#define LOG_TAG "FrameBuffer"

FrameNode::FrameNode(int width, int height, AVPixelFormat pixFmt) : mType(AVMEDIA_TYPE_VIDEO) {
    mFrame = av_frame_alloc();
    if (!mFrame) {
        LOGE("Cannot allocate frame data");
        return;
    }

    mFrame->width = width;
    mFrame->height = height;
    mFrame->format = pixFmt;
    int ret = av_frame_get_buffer(mFrame, 0);
    if (ret < 0) {
        LOGE("Cannot get frame buffer: %s", av_err2str(ret));
        return;
    }
}

FrameNode::FrameNode(uint64_t channelLayout, int nbSamples, AVSampleFormat sampleFmt) : mType(AVMEDIA_TYPE_AUDIO) {
    mFrame = av_frame_alloc();
    if (!mFrame) {
        LOGE("Cannot allocate frame data");
        return;
    }

    mFrame->channel_layout = channelLayout;
    mFrame->format = sampleFmt;
    mFrame->nb_samples = nbSamples;
    int ret = av_frame_get_buffer(mFrame, 0);
    if (ret < 0) {
        LOGE("Cannot get frame buffer: %s", av_err2str(ret));
        return;
    }
}

FrameNode::~FrameNode() {
    av_frame_free(&mFrame);
}

FrameBuffer::FrameBuffer(int size, int width, int height, AVPixelFormat pixFmt) :
        mSize(size), mType(AVMEDIA_TYPE_VIDEO) {
    mThreshold = size / 5;
    mWidth = width;
    mHeight = height;
    mPixFmt = pixFmt;

    allocateBuffer();
}

FrameBuffer::FrameBuffer(int size, uint64_t channelLayout, int nbSamples, AVSampleFormat sampleFmt) :
        mSize(size), mType(AVMEDIA_TYPE_AUDIO) {
    mThreshold = size / 5;
    mChannelLayout = channelLayout;
    mNbSamples = nbSamples;
    mSampleFmt = sampleFmt;

    allocateBuffer();
}

void FrameBuffer::allocateBuffer() {
    int i;
    for (i = 0; i < mSize; i++) {
        FrameNode *newNode;
        if (mType == AVMEDIA_TYPE_VIDEO) newNode = new FrameNode(mWidth, mHeight, mPixFmt);
        else newNode = new FrameNode(mChannelLayout, mNbSamples, mSampleFmt);

        if (mHeadPtr == nullptr) {
            mHeadPtr = newNode;
            mTailPtr = newNode;
            mHeadPtr->mNextPtr = mHeadPtr;
        } else {
            mTailPtr->mNextPtr = newNode;
            newNode->mNextPtr = mHeadPtr;
            mTailPtr = newNode;
        }
    }

    mTailPtr = mHeadPtr;
}

FrameBuffer::~FrameBuffer() {
    FrameNode *tmp = mHeadPtr;
    FrameNode *next = tmp->mNextPtr;
    // Break the circular chain
    tmp->mNextPtr = nullptr;
    tmp = next;

    while (tmp != nullptr) {
        next = tmp->mNextPtr;
        delete tmp;
        tmp = next;
    }
}

bool FrameBuffer::isFull() {
    return mCount == mSize;
}

bool FrameBuffer::putFrame(AVFrame *inFrame) {
    // Check frame parameters before putting in
    if (mType == AVMEDIA_TYPE_VIDEO) {
        if (inFrame->width != mWidth || inFrame->height != mHeight || inFrame->format != mPixFmt) {
            LOGE("Invalid format, expected %d %d %s, got %d %d %s",
                 mWidth, mHeight, av_get_pix_fmt_name(mPixFmt),
                 inFrame->width, inFrame->height, av_get_pix_fmt_name((AVPixelFormat) inFrame->format));
            return -1;
        }
    } else if (mType == AVMEDIA_TYPE_AUDIO) {
        if (inFrame->channel_layout != mChannelLayout || inFrame->format != mSampleFmt || inFrame->nb_samples != mNbSamples) {
            LOGE("Invalid format, expected %lld %s %d, got %lld %s %d",
                 mChannelLayout, av_get_sample_fmt_name(mSampleFmt), mNbSamples,
                 inFrame->channel_layout, av_get_sample_fmt_name((AVSampleFormat) inFrame->format), inFrame->nb_samples);
            return -1;
        }
    } else {
        LOGE("Unknown frame type.");
        return -1;
    }

    if (mCount == mSize) return false;

    AVFrame *frame = mTailPtr->mFrame;
    mTailPtr = mTailPtr->mNextPtr;

    av_frame_copy(frame, inFrame);
    frame->time_base = inFrame->time_base;
    frame->pts = inFrame->pts;

    mCount++;

    // Get out of buffering state if there are enough frames
    if (mIsBuffering && mCount >= mThreshold) mIsBuffering = false;

    return true;
}

bool FrameBuffer::takeFrame(AVFrame *outFrame) {

    if (mIsBuffering) return false;
    if (mCount == 0) return false;

    AVFrame *frame = mHeadPtr->mFrame;
    av_frame_copy(outFrame, frame);
    outFrame->pts = frame->pts;
    // Move head to next frame
    mHeadPtr = mHeadPtr->mNextPtr;

    mCount--;
    if (mCount == 0) mIsBuffering = true;

    return true;
}

bool FrameBuffer::takeFrame(AVFrame *outFrame, int64_t pts) {

    if (mIsBuffering) return false;
    if (mCount == 0) return false;

    AVFrame *frame = mHeadPtr->mFrame;
    FrameNode *next;
    // Find the nearest smaller frame with given pts
    // If current frame if after pts, return nothing
    if (frame->pts > pts) return false;
    // If equal then return that frame (barely happens)
    if (frame->pts == pts) {
        av_frame_copy(outFrame, frame);
        outFrame->pts = frame->pts;
        // Move head to next frame
        mHeadPtr = mHeadPtr->mNextPtr;

        mCount--;
        if (mCount == 0) mIsBuffering = true;
        return true;
    }

    next = mHeadPtr->mNextPtr;
    while (true) {
        // Check if there is a next frame
        // There is no next frame, return current frame
        if (mCount == 0) {
            av_frame_copy(outFrame, frame);
            outFrame->pts = frame->pts;
            // Move head to next frame
            mHeadPtr = mHeadPtr->mNextPtr;
            mIsBuffering = true;
            return true;
        } else {
            AVFrame *nextFrame = next->mFrame;
            // If next frame is after pts, return current frame
            if (nextFrame->pts > pts) {
                av_frame_copy(outFrame, frame);
                outFrame->pts = frame->pts;
                // Move head to next frame
                mHeadPtr = mHeadPtr->mNextPtr;
                mCount--;
                if (mCount == 0) mIsBuffering = true;
                return true;
            } else {
                // Move head to next frame
                mHeadPtr = mHeadPtr->mNextPtr;
                frame = nextFrame;
                next = next->mNextPtr;
                mCount--;
            }
        }
    }
}

void FrameBuffer::reset() {
    mCount = 0;
    mIsBuffering = true;
    mTailPtr = mHeadPtr;
}