#ifndef MEDIA_STREAMER_BUILDER_H
#define MEDIA_STREAMER_BUILDER_H
#include "MediaStreamer.h"

class MediaStreamerBuilder : public AudioStreamerBuilder, public VideoStreamerBuilder {
private:
    bool mHasAudio = true;
    bool mHasVideo = true;

    AudioStreamer *mAudioStreamer = nullptr;
    VideoStreamer *mVideoStreamer = nullptr;

    void cleanUp();
public:
    MediaStreamerBuilder *setHasAudio(bool hasAudio);

    MediaStreamerBuilder *setHasVideo(bool hasVideo);

    MediaStreamer *buildMediaStreamer();
};


#endif //MEDIA_STREAMER_BUILDER_H
