#ifndef VIDEO_SINK_H
#define VIDEO_SINK_H

extern "C" {
#include "libavutil/frame.h"
}
#include "semaphore"

class Sink {
public:
    int64_t mId = 0;
};

class VideoSink : public Sink {
public:
    virtual int onVideoFrame(AVFrame *frame) = 0;
};

class AudioSink : public Sink {
public:
    virtual int onAudioFrame(AVFrame *frame) = 0;
};

typedef struct VideoSinkNode {
    VideoSink *sink = nullptr;
    struct VideoSinkNode *next = nullptr;

    VideoSinkNode(VideoSink *sink) {
        this->sink = sink;
    }
} VideoSinkNode;

typedef struct AudioSinkNode {
    AudioSink *sink = nullptr;
    struct AudioSinkNode *next = nullptr;

    AudioSinkNode(AudioSink *sink){
        this->sink = sink;
    }
} AudioSinkNode;

#endif