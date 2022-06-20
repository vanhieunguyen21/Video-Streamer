#ifndef VIDEO_STREAMER_BUILDER_H
#define VIDEO_STREAMER_BUILDER_H

#include "VideoStreamer.h"

class VideoStreamerBuilder {
protected:
    // OpenGLES Renderer
    RendererES3 **mRenderer;

    AVRational mVideoTimeBase = {0, 1};
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
    /** Set renderer of streamer */
    VideoStreamerBuilder *setRenderer(RendererES3 **renderer);

    /** Set video stream time base. */
    VideoStreamerBuilder *setVideoTimeBase(AVRational timebase);
    /** Set input picture width. */
    VideoStreamerBuilder *setSrcWidth(int width);

    /** Set input picture height. */
    VideoStreamerBuilder *setSrcHeight(int height);

    /** Set input picture format. */
    VideoStreamerBuilder *setSrcPixelFormat(AVPixelFormat pixFmt);

    /** Set output picture width.
     * If this value is not set, use input value instead. */
    VideoStreamerBuilder *setWidth(int width);

    /** Set output picture height.
     * If this value is not set, use input value instead. */
    VideoStreamerBuilder *setHeight(int height);

    /** Set output picture format.
     * If this value is not set, use default value instead. */
    VideoStreamerBuilder *setPixelFormat(GLenum pixFmt);

    /** Set output internal picture format.
     * If this value is not set, use default value instead.*/
    VideoStreamerBuilder *setInternalPixelFormat(GLint internalPixFmt);

    /** Build video streamer from given parameters.
     * @return steamer or nullptr if failed to build streamer */
    VideoStreamer *buildVideoStreamer();
};


#endif //VIDEO_STREAMER_BUILDER_H
