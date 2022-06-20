#ifndef MEDIA_STREAMER_H
#define MEDIA_STREAMER_H

#include "AudioStreamer.h"
#include "AudioStreamerBuilder.h"
#include "VideoStreamer.h"
#include "VideoStreamerBuilder.h"

enum class MediaStreamerState {
    INITIATE, READY, RUNNING, PAUSED, STOPPED
};

class MediaStreamer : public AudioSink, public VideoSink {
    friend class MediaStreamerBuilder;

private:
    std::atomic<MediaStreamerState> mState = {MediaStreamerState::INITIATE};
    std::atomic_int64_t *mCurrentTsMs = nullptr;

    MediaStreamer();

public:
    AudioStreamer *mAudioStreamer = nullptr;
    VideoStreamer *mVideoStreamer = nullptr;

    ~MediaStreamer();

    void pause();

    void resume();

    void stop();

    /** Callback when there is an incoming audio frame, pass it to audio streamer. */
    int onAudioFrame(AVFrame *frame) override;

    /** Callback when there is an incoming video frame, pass it to video streamer. */
    int onVideoFrame(AVFrame *frame) override;
};

#endif //MEDIA_STREAMER_H
