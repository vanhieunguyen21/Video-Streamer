#include "../common/JNILogHelper.h"
#include "Demuxer.h"

#define LOG_TAG "Demuxer"

Demuxer::Demuxer(const char *url) {
    this->mUrl = new char[strlen(url) + 1];
    strcpy(this->mUrl, url);
    mState = DemuxerState::INITIATE;

    initiateDemuxer();
}

Demuxer::~Demuxer() {
    if (mState == DemuxerState::RUNNING || mState == DemuxerState::PAUSED) stop();
    removeAllSinks();
    release();
    delete[] mUrl;

//    delete mVideoStream;
//    delete mAudioStream;
}

void Demuxer::logPacket(AVPacket *pkt) {
    AVRational *time_base = &mFmtCtx->streams[pkt->stream_index]->time_base;
    LOGI("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s size:%d stream_index:%d",
         av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
         av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
         av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
         pkt->size, pkt->stream_index);
}

void Demuxer::initiateDemuxer() {
    LOGV("Initializing demuxer...");
    int ret;

    mFmtCtx = avformat_alloc_context();
    // Open input url and allocate format context
    LOGV("Opening input url '%s'", mUrl);
    ret = avformat_open_input(&mFmtCtx, mUrl, nullptr, nullptr);
    if (ret < 0) {
        LOGE("Cannot open input '%s'", mUrl);
        return;
    }

    // Retrieve stream information
    LOGV("Retrieving stream information...");
    ret = avformat_find_stream_info(mFmtCtx, nullptr);
    if (ret < 0) {
        LOGE("Cannot find stream information.");
        return;
    }

    if (mFmtCtx->duration != 0) {
        mHasDuration = true;
        mDuration = mFmtCtx->duration;
    }

    // Open video decoder
    mVideoStream = new DecodeStream();
    if (openCodecContext(mVideoStream, AVMEDIA_TYPE_VIDEO)) {
        mVideoStream->mWidth = mVideoStream->mCodecCtx->width;
        mVideoStream->mHeight = mVideoStream->mCodecCtx->height;
        mVideoStream->mPixFmt = mVideoStream->mCodecCtx->pix_fmt;

        mVideoStream->mTimebase = mVideoStream->mStream->time_base;
    }

    // Open audio decoder
    mAudioStream = new DecodeStream();
    if (openCodecContext(mAudioStream, AVMEDIA_TYPE_AUDIO)) {
        mAudioStream->mSampleRate = mAudioStream->mCodecCtx->sample_rate;
        mAudioStream->mChannelLayout = mAudioStream->mCodecCtx->channel_layout;
        mAudioStream->mNbSamples = mAudioStream->mCodecCtx->frame_size;
        mAudioStream->mSampleFmt = mAudioStream->mCodecCtx->sample_fmt;

        mAudioStream->mTimebase = mAudioStream->mStream->time_base;
    }

    if (!mVideoStream && !mAudioStream) {
        LOGE("No audio or video stream found, aborting.");
        return;
    }

    mFrame = av_frame_alloc();
    if (!mFrame) {
        LOGE("Could not allocate frame.");
        return;
    }

    mPacket = av_packet_alloc();
    if (!mPacket) {
        LOGE("Could not allocate packet.");
        return;
    }

    mState = DemuxerState::READY;
}

int Demuxer::openCodecContext(DecodeStream *dst, AVMediaType type) {
    LOGV("Opening %s codec context...", av_get_media_type_string(type));
    int ret;
    AVStream *stream;
    const AVCodec *codec;

    ret = av_find_best_stream(mFmtCtx, type, -1, -1, nullptr, 0);
    if (ret < 0) {
        LOGE("Cannot find %s stream in input mUrl '%s'", av_get_media_type_string(type), mUrl);
        return 0;
    } else {
        LOGI("%s stream index: %d", av_get_media_type_string(type), ret);
        dst->mStreamIdx = ret;
        dst->mStream = mFmtCtx->streams[dst->mStreamIdx];
        stream = dst->mStream;

        // Find decoder for the stream
        LOGV("Finding decoder for %s", avcodec_get_name(stream->codecpar->codec_id));
        codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) {
            LOGE("Cannot find codec %s", avcodec_get_name(stream->codecpar->codec_id));
            return 0;
        }

        // Allocate codec context for decoder
        dst->mCodecCtx = avcodec_alloc_context3(codec);
        if (!dst->mCodecCtx) {
            LOGE("Failed to allocate codec context %s", avcodec_get_name(codec->id));
            return 0;
        }

        // Copy codec parameters from input stream to output codec context
        ret = avcodec_parameters_to_context(dst->mCodecCtx, stream->codecpar);
        if (ret < 0) {
            LOGE("Failed to copy %s codec parameters to decoder context.", av_get_media_type_string(type));
            return 0;
        }

        // Init the decoder
        ret = avcodec_open2(dst->mCodecCtx, codec, nullptr);
        if (ret < 0) {
            LOGE("Failed to open codec %s", avcodec_get_name(codec->id));
            return 0;
        }
    }

    LOGV("%s decoder opened", av_get_media_type_string(type));
    return 1;
}

int Demuxer::decodePacket(DecodeStream *dst, AVPacket *pkt) {
    int ret;

//    if (pkt) logPacket(pkt);

    // Submit the packet to decoder
    ret = avcodec_send_packet(dst->mCodecCtx, pkt);
    if (ret < 0) {
        LOGE("Error submitting a packet for decoding: %s", av_err2str(ret));
        return 0;
    }

    // Get all available frames from the decoder
    while (ret >= 0) {
        if (mState == DemuxerState::STOPPED) return 0;
        ret = avcodec_receive_frame(dst->mCodecCtx, mFrame);
        if (ret < 0) {
            // Those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) return 0;

            LOGE("Error during decoding: %s", av_err2str(ret));
            return ret;
        }

        // Maximum number of times to try to write frame into sinks
        int numRetries;
        // Write the frame data to video/audio sinks
        if (dst->mCodecCtx->codec->type == AVMEDIA_TYPE_VIDEO) {
            VideoSinkNode *videoSink = mVideoSinks;
            while (videoSink != nullptr) {
                numRetries = 30;
                // Try to write frame into sink, retry after a while if not succeeded
                while (!videoSink->sink->onVideoFrame(mFrame) && numRetries){
                    if (mState == DemuxerState::STOPPED) return 0;
                    if (mState == DemuxerState::RUNNING) numRetries--;
                    usleep(10000); // Sleep for 10ms
                }
                videoSink = videoSink->next;
            }
        } else {
            AudioSinkNode *audioSink = mAudioSinks;
            while (audioSink != nullptr) {
                numRetries = 30;
                // Try to write frame into sink, retry after a while if not succeeded
                while (!audioSink->sink->onAudioFrame(mFrame) && numRetries){
                    if (mState == DemuxerState::STOPPED) return 0;
                    if (mState == DemuxerState::RUNNING) numRetries--;
                    usleep(10000); // Sleep for 10ms
                }
                audioSink = audioSink->next;
            }
        }

        av_frame_unref(mFrame);
    }

    return 0;
}

int Demuxer::decodePacket(DecodeStream *dst) {
    return decodePacket(dst, mPacket);
}

void Demuxer::flushDecoder(DecodeStream *dst) {
    decodePacket(dst, nullptr);
}

int Demuxer::getSampleRate() const {
    if (mAudioStream) return mAudioStream->mSampleRate;
    return 0;
}

uint64_t Demuxer::getChannelLayout() const {
    if (mAudioStream) return mAudioStream->mChannelLayout;
    return 0;
}

int Demuxer::getNbSamples() const {
    if (mAudioStream) return mAudioStream->mNbSamples;
    return 0;
}

AVSampleFormat Demuxer::getSampleFormat() const {
    if (mAudioStream) return mAudioStream->mSampleFmt;
    return AV_SAMPLE_FMT_NONE;
}

AVRational Demuxer::getAudioTimebase() const {
    if (mAudioStream) return mAudioStream->mTimebase;
    return av_make_q(0, 1);
}

int Demuxer::getWidth() const {
    if (mVideoStream) return mVideoStream->mWidth;
    return 0;
}

int Demuxer::getHeight() const {
    if (mVideoStream) return mVideoStream->mHeight;
    return 0;
}

AVPixelFormat Demuxer::getPixelFormat() const {
    if (mVideoStream) return mVideoStream->mPixFmt;
    return AV_PIX_FMT_NONE;
}

AVRational Demuxer::getVideoTimebase() const {
    if (mVideoStream) return mVideoStream->mTimebase;
    return av_make_q(0, 1);
}

int64_t Demuxer::getDuration() const {
    if (mHasDuration) return mDuration;
    return 0;
}

void Demuxer::addVideoSink(VideoSink *videoSink) {
    if (!videoSink) return;
    std::unique_lock<std::mutex> lck(mMutex);
    if (mVideoSinks == nullptr) mVideoSinks = new VideoSinkNode(videoSink);
    else {
        auto *node = new VideoSinkNode(videoSink);
        node->next = nullptr;
        mVideoSinks->next = node;
    }
}

void Demuxer::addAudioSink(AudioSink *audioSink) {
    if (!audioSink) return;
    std::unique_lock<std::mutex> lck(mMutex);
    if (mAudioSinks == nullptr) mAudioSinks = new AudioSinkNode(audioSink);
    else {
        auto *node = new AudioSinkNode(audioSink);
        node->next = nullptr;
        mAudioSinks->next = node;
    }
}

void Demuxer::removeVideoSink(int id) {
    std::unique_lock<std::mutex> lck(mMutex);
    if (mVideoSinks == nullptr) return;
    VideoSinkNode *node = mVideoSinks;
    if (node->sink->mId == id) {
        VideoSinkNode *tmp = mVideoSinks;
        mVideoSinks = mVideoSinks->next;
        delete tmp;
    }
    while (node->next != nullptr) {
        if (node->next->sink->mId == id) {
            VideoSinkNode *tmp = node->next;
            node->next = node->next->next;
            delete tmp;
            return;
        }
        node = node->next;
    }
}

void Demuxer::removeAudioSink(int id) {
    std::unique_lock<std::mutex> lck(mMutex);
    if (mAudioSinks == nullptr) return;
    AudioSinkNode *node = mAudioSinks;
    if (node->sink->mId == id) {
        AudioSinkNode *tmp = mAudioSinks;
        mAudioSinks = mAudioSinks->next;
        delete (tmp);
    }
    while (node->next != nullptr) {
        if (node->next->sink->mId == id) {
            AudioSinkNode *tmp = node->next;
            node->next = node->next->next;
            delete (tmp);
            return;
        }
        node = node->next;
    }
}

void Demuxer::removeAllSinks() {
    LOGV("Removing all sinks...");
    if (mVideoSinks != nullptr) {
        VideoSinkNode *tmp;
        while (mVideoSinks != nullptr) {
            tmp = mVideoSinks;
            mVideoSinks = mVideoSinks->next;
            delete tmp;
        }
    }

    if (mAudioSinks != nullptr) {
        AudioSinkNode *tmp;
        while (mAudioSinks != nullptr) {
            tmp = mAudioSinks;
            mAudioSinks = mAudioSinks->next;
            delete tmp;
        }
    }
    LOGV("All sinks removed.");
}

void Demuxer::start() {
    LOGV("Starting demuxer...");
    std::unique_lock<std::mutex> lck(mMutex);
    switch (mState) {
        case DemuxerState::INITIATE:
            LOGE("Demuxer is not yet ready, initiate first.");
            break;
        case DemuxerState::READY:
            mState = DemuxerState::RUNNING;
            // Create new demuxing
            pthread_create(&mThread, nullptr, Demuxer::threadDemux, this);
            break;
        case DemuxerState::PAUSED:
        case DemuxerState::RUNNING:
            LOGE("Demuxing is already running.");
            break;
        case DemuxerState::STOPPED:
            LOGE("Demuxer stopped, cannot start again.");
            break;
    }
}

void Demuxer::seek(int64_t ts) {
    if (!mHasDuration) {
        LOGE("Streams have no duration. Cannot seek.");
        return;
    }
    mMutex.lock();

    av_seek_frame(mFmtCtx, -1, ts * 1000, AVSEEK_FLAG_BACKWARD);

    // Flush codec buffer
    avcodec_flush_buffers(mAudioStream->mCodecCtx);
    avcodec_flush_buffers(mVideoStream->mCodecCtx);

    mMutex.unlock();
}

void Demuxer::pause() {
    switch (mState) {
        case DemuxerState::INITIATE:
        case DemuxerState::READY:
        case DemuxerState::PAUSED:
        case DemuxerState::STOPPED:
            LOGE("Illegal state: %d", mState.load());
            break;
        case DemuxerState::RUNNING:
            mState = DemuxerState::PAUSED;
            break;
    }
}

void Demuxer::resume() {
    switch (mState) {
        case DemuxerState::INITIATE:
        case DemuxerState::READY:
        case DemuxerState::RUNNING:
        case DemuxerState::STOPPED:
            LOGE("Illegal state: %d", mState.load());
            break;
        case DemuxerState::PAUSED:
            mState = DemuxerState::RUNNING;
            break;
    }
}

void Demuxer::stop() {
    LOGV("Stopping demuxer...");
    mState = DemuxerState::STOPPED;
    // Wait for demuxing thread to stop
    pthread_join(mThread, nullptr);
    LOGV("Demuxer stopped.");
}

void Demuxer::release() {
    LOGV("Releasing demuxer...");
    std::unique_lock<std::mutex> lck(mMutex);
    switch (mState) {
        case DemuxerState::INITIATE:
        case DemuxerState::READY:
        case DemuxerState::PAUSED:
        case DemuxerState::STOPPED:
            avcodec_free_context(&mVideoStream->mCodecCtx);
            avcodec_free_context(&mAudioStream->mCodecCtx);

            av_packet_free(&mPacket);
            av_frame_free(&mFrame);

            removeAllSinks();

            delete mAudioStream;
            delete mVideoStream;
            avformat_close_input(&mFmtCtx);
            delete mFmtCtx;

            LOGV("Demuxer released.");
            break;
        case DemuxerState::RUNNING:
            LOGE("Demuxer is running, stop it first.");
            break;
    }
    LOGV("Demuxer released.");
}

void *Demuxer::threadDemux(void *args) {
    LOGV("Demuxing thread started.");
    auto *demuxer = (Demuxer *) args;
    std::unique_lock<std::mutex> lck(demuxer->mMutex, std::defer_lock);

    int ret;
    while (true) {
        lck.lock();

        if (demuxer->mState == DemuxerState::STOPPED) {
            lck.unlock();
            break;
        }

        if (demuxer->mState == DemuxerState::PAUSED) {
            lck.unlock();
            usleep(10000); // Sleep 10ms
            continue;
        }

        ret = av_read_frame(demuxer->mFmtCtx, demuxer->mPacket);
        if (ret < 0) {
            lck.unlock();
            break;
        }

        // check if the packet belongs to a stream we are interested in, otherwise skip it
        if (demuxer->mPacket->stream_index == demuxer->mVideoStream->mStreamIdx) {
            ret = demuxer->decodePacket(demuxer->mVideoStream);
        } else if (demuxer->mPacket->stream_index == demuxer->mAudioStream->mStreamIdx) {
            ret = demuxer->decodePacket(demuxer->mAudioStream);
        }

        av_packet_unref(demuxer->mPacket);

        if (ret < 0) {
            lck.unlock();
            break;
        }

        lck.unlock();
    }

    if (lck.owns_lock()) lck.unlock();
    demuxer->mState = DemuxerState::STOPPED;
    LOGV("Demuxing thread finished.");
    return nullptr;
}
