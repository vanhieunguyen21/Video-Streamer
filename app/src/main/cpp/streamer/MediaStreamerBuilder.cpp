#include "MediaStreamerBuilder.h"
#include "JNILogHelper.h"

#define LOG_TAG "MediaStreamerBuilder"

void MediaStreamerBuilder::cleanUp() {
    delete mAudioStreamer;
    delete mVideoStreamer;
}

MediaStreamerBuilder *MediaStreamerBuilder::setHasAudio(bool hasAudio) {
    mHasAudio = hasAudio;
    return this;
}

MediaStreamerBuilder *MediaStreamerBuilder::setHasVideo(bool hasVideo) {
    mHasVideo = hasVideo;
    return this;
}

MediaStreamer *MediaStreamerBuilder::buildMediaStreamer() {
    if (mHasAudio) {
        mAudioStreamer = buildAudioStreamer();
        if (!mAudioStreamer) {
            cleanUp();
            return nullptr;
        }
    }

    if (mHasVideo) {
        mVideoStreamer = buildVideoStreamer();
        if (!mVideoStreamer) {
            cleanUp();
            return nullptr;
        }
    }

    auto *mediaStreamer = new MediaStreamer();

    if (mHasAudio) {
        mediaStreamer->mAudioStreamer = mAudioStreamer;
        mAudioStreamer->mCurrentTsMs = mediaStreamer->mCurrentTsMs;
    }
    if (mHasVideo) {
        mediaStreamer->mVideoStreamer = mVideoStreamer;
        mVideoStreamer->mCurrentTsMs = mediaStreamer->mCurrentTsMs;
    }

    mediaStreamer->mState = MediaStreamerState::READY;
    mediaStreamer->mState = MediaStreamerState::RUNNING;

    return mediaStreamer;
}