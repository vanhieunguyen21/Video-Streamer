#include "MediaStreamer.h"
#include "JNILogHelper.h"

#define LOG_TAG "MediaStreamer"

MediaStreamer::MediaStreamer() {
    mCurrentTsMs = new std::atomic_int64_t{0};
}

MediaStreamer::~MediaStreamer() {
    delete mCurrentTsMs;
}

void MediaStreamer::pause() {
    switch (mState.load()) {
        case MediaStreamerState::INITIATE:
        case MediaStreamerState::READY:
        case MediaStreamerState::PAUSED:
        case MediaStreamerState::STOPPED:
            LOGE("Failed to pause. Illegal state: %d", mState.load());
            break;
        case MediaStreamerState::RUNNING:
            mState = MediaStreamerState::PAUSED;
            break;
    }
}

void MediaStreamer::resume() {
    switch (mState.load()) {
        case MediaStreamerState::INITIATE:
        case MediaStreamerState::READY:
        case MediaStreamerState::RUNNING:
        case MediaStreamerState::STOPPED:
            LOGE("Failed to resume. Illegal state: %d", mState.load());
            break;
        case MediaStreamerState::PAUSED:
            mState = MediaStreamerState::RUNNING;
            break;
    }
}

void MediaStreamer::stop() {
    mState = MediaStreamerState::STOPPED;
}

int MediaStreamer::onAudioFrame(AVFrame *frame) {
    if (mAudioStreamer) return mAudioStreamer->onAudioFrame(frame);
    return 1;
}

int MediaStreamer::onVideoFrame(AVFrame *frame) {
    if (mVideoStreamer) return mVideoStreamer->onVideoFrame(frame);
    return 1;
}