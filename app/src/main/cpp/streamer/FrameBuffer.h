#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

extern "C" {
#include <libavutil/pixdesc.h>
#include "libavutil/frame.h"
}

#include "semaphore"
#include "mutex"
#include "condition_variable"

class FrameNode {
    friend class FrameBuffer;

public:
    const AVMediaType mType = AVMEDIA_TYPE_UNKNOWN; // Type of frame this node is holding

    AVFrame *mFrame = nullptr; // The frame this node is holding

    FrameNode *mNextPtr = nullptr; // Pointer to next node

    FrameNode(int width, int height, AVPixelFormat pixFmt);

    FrameNode(uint64_t channelLayout, int nbSamples, AVSampleFormat sampleFmt);

    ~FrameNode();
};

/** A circular queue that holds every processed frame for audio/video player */
class FrameBuffer {
private:
    std::atomic_int mCount = {0};
    const int mSize;
    int mThreshold; // Minimum number of frames to get out of buffering state
    std::atomic_bool mIsBuffering = {true};

    const AVMediaType mType = AVMEDIA_TYPE_UNKNOWN;

    // Audio type {
    AVSampleFormat mSampleFmt = AV_SAMPLE_FMT_NONE;
    uint64_t mChannelLayout = 0;
    int mNbSamples = 0;
    // } Audio type

    // Video type {
    AVPixelFormat mPixFmt = AV_PIX_FMT_NONE;
    int mWidth = 0;
    int mHeight = 0;
    // } Video type

    // Stores the pointer of the first object containing data in the list
    FrameNode *mHeadPtr = nullptr;
    // Stores the pointer of the next object for data to be written into
    FrameNode *mTailPtr = nullptr;
private:
    /** Allocate memory to 'size' frames */
    void allocateBuffer();

public:
    /** Constructor to create a picture buffer of 'size' */
    FrameBuffer(int size, int width, int height, AVPixelFormat pixFmt);

    /** Constructor to create an audio buffer of 'size' */
    FrameBuffer(int size, uint64_t channelLayout, int nbSamples, AVSampleFormat sampleFmt);

    ~FrameBuffer();

    bool isFull();

    /** Put a frame into the buffer, this method will block if buffer is full.
     * @return true if frame successfully put into buffer
     *         false if not */
    bool putFrame(AVFrame *inFrame);

    /** Try to take a frame out of buffer, do nothing if buffer is empty.
     * @return true if a buffer was put into outFrame
     *         false if buffer is empty, nothing was done */
    bool takeFrame(AVFrame *outFrame);

    /** Take a frame out of buffer which is right before given pts.
     * This method will block if buffer is empty.
     * @return true if a buffer was put into outFrame
     *         false if there is no frame before pts in the buffer */
    bool takeFrame(AVFrame *outFrame, int64_t pts);

    /** Reset the frame buffer.
     * This will only set counter to 0, change head and tail pointer to start
     * and won't allocate or deallocate anything. */
    void reset();

};

#endif // FRAME_BUFFER_H
