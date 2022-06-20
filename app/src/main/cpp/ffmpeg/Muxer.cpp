#include "Muxer.h"
#include "../common/JNILogHelper.h"

#define LOG_TAG "Muxer"

Muxer::Muxer(const char *fileName, const char *audioEncoder, const char *videoEncoder) {
    mFileName = new char[strlen(fileName) + 1];
    mVideoEncoderName = new char[strlen(videoEncoder) + 1];
    mAudioEncoderName = new char[strlen(audioEncoder) + 1];
    strcpy(mFileName, fileName);
    strcpy(mVideoEncoderName, videoEncoder);
    strcpy(mAudioEncoderName, audioEncoder);
}

Muxer::~Muxer() {
    delete mFileName;
    delete mVideoEncoderName;
    delete mAudioEncoderName;

    delete mVideoSt;
    delete mAudioSt;
}

bool Muxer::initiate() {
    LOGV("Initiating muxer...");
    int ret;

    // Allocate the output media context
    avformat_alloc_output_context2(&mFmtCtx, nullptr, nullptr, mFileName);
    if (!mFmtCtx) {
        LOGD("Could not deduce output format from file extension: using MPEG.");
        avformat_alloc_output_context2(&mFmtCtx, nullptr, "mpeg", mFileName);
    }
    if (!mFmtCtx) {
        LOGE("Could not allocate output context.");
        return false;
    }
    const AVOutputFormat *fmt = mFmtCtx->oformat;

    // Find the audio encoder if provided, use default if not provided and add audio stream
    // Then open audio stream and allocate the necessary encode buffers
    if (mHasAudio) {
        if (mAudioEncoderName) {
            mAudioSt->mCodec = avcodec_find_encoder_by_name(mAudioEncoderName);
            if (!mAudioSt->mCodec) {
                LOGE("Could not find encoder for '%s', use default encoder.", mAudioEncoderName);
            }
        }
        if (!mAudioEncoderName || !mAudioSt->mCodec) {
            // Use default codec
            mAudioSt->mCodec = avcodec_find_encoder(fmt->audio_codec);
            if (!mAudioSt->mCodec) {
                LOGE("Could not find encoder for '%s'", avcodec_get_name(fmt->audio_codec));
                return false;
            }
        }

        ret = addStream(mAudioSt);
        if (ret == 0) return false;

        ret = openAudio(mAudioSt);
        if (ret == 0) return false;
    }

    // Find the video encoder if provided, use default if not provided and add video stream
    // Then open video stream and allocate the necessary encode buffers
    if (mHasVideo) {
        if (mVideoEncoderName) {
            mVideoSt->mCodec = avcodec_find_encoder_by_name(mVideoEncoderName);
            if (!mVideoSt->mCodec) {
                LOGE("Could not find encoder for '%s', use default encoder.", mVideoEncoderName);
            }
        }
        if (!mVideoEncoderName || !mVideoSt->mCodec) {
            // Use default encoder
            mVideoSt->mCodec = avcodec_find_encoder(fmt->video_codec);
            if (!mVideoSt->mCodec) {
                LOGE("Could not find encoder for '%s'", avcodec_get_name(fmt->video_codec));
                return false;
            }
        }

        ret = addStream(mVideoSt);
        if (ret == 0) return false;

        ret = openVideo(mVideoSt);
        if (ret == 0) return false;
    }

    av_dump_format(mFmtCtx, 0, mFileName, 1);

    // Open the output file, if needed
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&mFmtCtx->pb, mFileName, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("Could not open '%s': %s", mFileName, av_err2str(ret));
            return false;
        }
    }

    // Write the stream header, if any
    ret = avformat_write_header(mFmtCtx, nullptr);
    if (ret < 0) {
        LOGE("Error occurred when opening output file: %s", av_err2str(ret));
        return false;
    }

    // Check if resampler is needed
    OutputStream *st = mAudioSt;
    if (!(st->mSrcSampleRate == st->mDstSampleRate &&
          st->mSrcChannelLayout == st->mDstChannelLayout &&
          st->mSrcSampleFmt == st->mDstSampleFmt)) {
        ret = createResampler();
        if (!ret) return false;
    }

    // Check if scaler is needed
    st = mVideoSt;
    if (!(st->mSrcWidth == st->mDstWidth &&
          st->mSrcHeight == st->mDstHeight &&
          st->mSrcPixFmt == st->mDstPixFmt)) {
        ret = createScaler();
        if (!ret) return false;
    }

    mState = MuxerState::READY;
    return true;
}

void Muxer::logPacket(AVPacket *pkt) {
    AVRational *time_base = &mFmtCtx->streams[pkt->stream_index]->time_base;
    LOGI("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d",
         av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
         av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
         av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
         pkt->stream_index);
}

int Muxer::addStream(OutputStream *ost) {
    LOGV("Adding %s stream...", av_get_media_type_string(ost->mCodec->type));
    AVCodecContext *codecCtx;
    const AVCodec *codec = ost->mCodec;
    int i;

    ost->mStartTimestamp = 0;
    ost->mPacket = av_packet_alloc();
    if (!ost->mPacket) {
        LOGE("Could not allocate AVPacket");
        return 0;
    }

    ost->mStream = avformat_new_stream(mFmtCtx, nullptr);
    if (!ost->mStream) {
        LOGE("Could not allocate stream");
        return 0;
    }

    ost->mStream->id = (int32_t) mFmtCtx->nb_streams - 1;
    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        LOGE("Could not alloc an encoding context");
        return 0;
    }
    ost->mCodecCtx = codecCtx;

    switch (codec->type) {
        case AVMEDIA_TYPE_AUDIO:
            codecCtx->codec_id = codec->id;
            // Set sample format
            codecCtx->sample_fmt = codec->sample_fmts ? codec->sample_fmts[0] : ost->mDstSampleFmt;
            // Set bit rate if available
            if (ost->mBitRate != 0) codecCtx->bit_rate = ost->mBitRate;
            // Set sample rate
            codecCtx->sample_rate = ost->mDstSampleRate;
            // Check if current sample rate is supported by codec
            if (codec->supported_samplerates) {
                codecCtx->sample_rate = codec->supported_samplerates[0];
                for (i = 0; codec->supported_samplerates[i]; i++) {
                    if (codec->supported_samplerates[i] == ost->mDstSampleRate) {
                        codecCtx->sample_rate = ost->mDstSampleRate;
                        break;
                    }
                }
                // Reassign sample rate in case there is any change to it
                ost->mDstSampleRate = codecCtx->sample_rate;
            }
            // Set channel layout and nb channels
            codecCtx->channel_layout = ost->mDstChannelLayout;
            // Check if current channel layout is supported by codec
            if (codec->channel_layouts) {
                codecCtx->channel_layout = codec->channel_layouts[0];
                for (i = 0; codec->channel_layouts[i]; i++) {
                    if (codec->channel_layouts[i] == ost->mDstChannelLayout) {
                        codecCtx->channel_layout = ost->mDstChannelLayout;
                        break;
                    }
                }
                // Reassign channel layout in case there is any change to it
                ost->mDstChannelLayout = codecCtx->channel_layout;
            }
            codecCtx->channels = av_get_channel_layout_nb_channels(codecCtx->channel_layout);

            if (ost->mTimeBase.num != 0 && ost->mTimeBase.den != 1) {
                ost->mStream->time_base = ost->mTimeBase;
                codecCtx->time_base = ost->mTimeBase;
            } else {
                // Use default time base
                ost->mStream->time_base = (AVRational) {1, 1000};
                codecCtx->time_base = ost->mStream->time_base;
            }

            break;

        case AVMEDIA_TYPE_VIDEO:
            codecCtx->codec_id = codec->id;
            // Set bit rate if available
            if (ost->mBitRate != 0) codecCtx->bit_rate = ost->mBitRate;
            // Set width and height
            codecCtx->width = ost->mDstWidth;
            codecCtx->height = ost->mDstHeight;
            // Set pixel format
            codecCtx->pix_fmt = ost->mDstPixFmt;
            /* timebase: This is the fundamental unit of time (in seconds) in terms
            * of which mFrame timestamps are represented. For fixed-fps content,
            * timebase should be 1/framerate and timestamp increments should be
            * identical to 1. */
            if (ost->mTimeBase.num != 0 && ost->mTimeBase.den != 1) {
                ost->mStream->time_base = ost->mTimeBase;
                codecCtx->time_base = ost->mTimeBase;
            } else {
                // Use default time base
                ost->mStream->time_base = (AVRational) {1, 1000};
                codecCtx->time_base = ost->mStream->time_base;
            }

            codecCtx->gop_size = 12;
            if (codecCtx->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
                // just for testing, we also add B frames
                codecCtx->max_b_frames = 2;
            }
            if (codecCtx->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
                /* Needed to avoid using macroblocks in which some coeffs overflow.
                 * This does not happen with normal video, it just happens here as
                 * the motion of the chroma plane does not match the luma plane. */
                codecCtx->mb_decision = 2;
            }
            break;
        default:
            break;
    }

    // Some formats want stream headers to be separate.
    if (mFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    return 1;
}

AVFrame *Muxer::allocateAudioFrame() {
    return FFmpegHelper::allocateAudioFrame(mAudioSt->mDstSampleRate, mAudioSt->mDstChannelLayout,
                                            mAudioSt->mDstNbSamples, mAudioSt->mDstSampleFmt);
}

AVFrame *Muxer::allocatePictureFrame() {
    return FFmpegHelper::allocatePictureFrame(mVideoSt->mDstWidth, mVideoSt->mDstHeight, mVideoSt->mDstPixFmt);
}

int Muxer::openAudio(OutputStream *ost) {
    LOGV("Opening audio stream...");
    AVCodecContext *codecCtx;
    int ret;

    codecCtx = ost->mCodecCtx;

    // Open the codec
    ret = avcodec_open2(codecCtx, ost->mCodec, nullptr);
    if (ret < 0) {
        LOGE("Could not open audio codec: %s", av_err2str(ret));
        return 0;
    }

    if (codecCtx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) ost->mDstNbSamples = 10000;
    else ost->mDstNbSamples = codecCtx->frame_size;

    // Allocate and init a re-usable frame
    ost->mFrame = allocateAudioFrame();
    if (!ost->mFrame) return 0;

    // Create sample buffer
    ost->mSampleBuffer = new SampleBuffer(codecCtx->channels, codecCtx->sample_fmt, codecCtx->frame_size);

    // Copy the stream parameters to the muxer
    ret = avcodec_parameters_from_context(ost->mStream->codecpar, codecCtx);
    if (ret < 0) {
        LOGE("Could not copy the stream parameters.");
        return 0;
    }

    return 1;
}

int Muxer::openVideo(OutputStream *ost) {
    LOGV("Opening video stream...");
    int ret;
    AVCodecContext *codecCtx = ost->mCodecCtx;

    // Open the codec
    ret = avcodec_open2(codecCtx, ost->mCodec, nullptr);

    if (ret < 0) {
        LOGE("Could not open video codec: %s", av_err2str(ret));
        return 0;
    }

    // Allocate and init a re-usable frame
    ost->mFrame = allocatePictureFrame();
    if (!ost->mFrame) return 0;

    // Copy the stream parameters to the muxer
    ret = avcodec_parameters_from_context(ost->mStream->codecpar, codecCtx);
    if (ret < 0) {
        LOGE("Could not copy the stream parameters.");
        return 0;
    }
    return 1;
}

int Muxer::createResampler() {
    LOGV("Creating resampler...");
    OutputStream *ost = mAudioSt;

    int ret;
    // Create re-sampler context
    ost->mSwrCtx = swr_alloc();
    if (!ost->mSwrCtx) {
        LOGE("Could not allocate resampler context.");
        return 0;
    }

    // Set options
    av_opt_set_int(ost->mSwrCtx, "in_channel_count", av_get_channel_layout_nb_channels(ost->mSrcChannelLayout), 0);
    av_opt_set_int(ost->mSwrCtx, "in_sample_rate", ost->mSrcSampleRate, 0);
    av_opt_set_sample_fmt(ost->mSwrCtx, "in_sample_fmt", ost->mSrcSampleFmt, 0);
    av_opt_set_int(ost->mSwrCtx, "out_channel_count", av_get_channel_layout_nb_channels(ost->mDstChannelLayout), 0);
    av_opt_set_int(ost->mSwrCtx, "out_sample_rate", ost->mDstSampleRate, 0);
    av_opt_set_sample_fmt(ost->mSwrCtx, "out_sample_fmt", ost->mDstSampleFmt, 0);

    // Initialize the resampling context
    if ((ret = swr_init(ost->mSwrCtx)) < 0) {
        LOGE("Failed to initialize the resampling context: %s", av_err2str(ret));
        return 0;
    }

    // Allocate tmp audio mFrame for source data
    ost->mTmpFrame = FFmpegHelper::allocateAudioFrame(ost->mSrcSampleRate, ost->mSrcChannelLayout, ost->mSrcNbSamples, ost->mSrcSampleFmt);
    if (!ost->mTmpFrame) return 0;

    mIsResamplerCreated = 1;
    return 1;
}

int Muxer::createScaler(int flag) {
    LOGV("Creating scaler...");
    OutputStream *ost = mVideoSt;

    ost->mSwsCtx = sws_getContext(ost->mSrcWidth, ost->mSrcHeight, ost->mSrcPixFmt,
                                  ost->mDstWidth, ost->mDstHeight, ost->mDstPixFmt,
                                  flag, nullptr, nullptr, nullptr);

    if (!ost->mSwsCtx) {
        LOGE("Impossible to create scale context for the conversion "
             "fmt_:%s s:%dx%d -> fmt_:%s s:%dx%d",
             av_get_pix_fmt_name(ost->mSrcPixFmt), ost->mSrcWidth, ost->mSrcHeight,
             av_get_pix_fmt_name(ost->mDstPixFmt), ost->mDstWidth, ost->mDstHeight);
        return 0;
    }

    // Allocate tmp video frame for source data
    ost->mTmpFrame = FFmpegHelper::allocatePictureFrame(ost->mSrcWidth, ost->mSrcHeight, ost->mSrcPixFmt);
    if (!ost->mTmpFrame) return 0;

    mIsScalerCreated = 1;
    return 1;
}

int Muxer::createScaler() {
    return createScaler(SWS_BICUBIC);
}

int Muxer::writeFrame(OutputStream *ost, AVFrame *frame) {
    AVCodecContext *codecCtx = ost->mCodecCtx;
    const AVStream *stream = ost->mStream;
    AVPacket *packet = ost->mPacket;

    int ret;

    ret = avcodec_send_frame(codecCtx, frame);
    if (ret < 0) {
        LOGE("Error sending a frame to the encoder: %s", av_err2str(ret));
        return 0;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codecCtx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        else if (ret < 0) {
            LOGE("Error encoding a frame: %s", av_err2str(ret));
            return 0;
        }

        // rescale output facket timestamp values from codec to stream timebase
        av_packet_rescale_ts(packet, codecCtx->time_base, stream->time_base);
        packet->stream_index = stream->index;

//        logPacket(packet);
        // Write the compressed frame to the media file.
        ret = av_interleaved_write_frame(mFmtCtx, packet);

        if (ret < 0) {
            LOGE("Error while writing output packet: %s", av_err2str(ret));
            return 0;
        }
    }

    return 1;
}

int Muxer::writeFrame(OutputStream *ost) {
    return writeFrame(ost, ost->mFrame);
}

int Muxer::writeAudioFrame(uint8_t *buffer, int offset, int nbSamples, int64_t frameTimestamp) {
    OutputStream *ost = mAudioSt;
    AVCodecContext *c = ost->mCodecCtx;

    int ret, dstNbSamples;
    AVFrame *frame = ost->mTmpFrame;
    if (!frame) {
        LOGE("Frame is not allocated.");
        return 0;
    }

    av_samples_copy(frame->data, &buffer, 0, offset, nbSamples,
                    av_get_channel_layout_nb_channels(ost->mSrcChannelLayout), ost->mSrcSampleFmt);
    dstNbSamples = (int) av_rescale_rnd(swr_get_delay(ost->mSwrCtx, c->sample_rate) + frame->nb_samples,
                                        c->sample_rate, frame->sample_rate, AV_ROUND_UP);

    /* When we pass a mFrame to the encoder, it may keep a reference to it internally;
     * make sure we do not overwrite it here */
    ret = av_frame_make_writable(ost->mFrame);
    if (ret < 0) {
        LOGE("Cannot make frame writable: %s", av_err2str(ret));
        return 0;
    }

    // Convert to destination format
    uint8_t **buf = ost->mSampleBuffer->prepareBuffer(dstNbSamples);
    ret = swr_convert(ost->mSwrCtx, buf, dstNbSamples, (const uint8_t **) frame->data, frame->nb_samples);
    if (ret < 0) {
        LOGE("Error while converting: %s", av_err2str(ret));
        return ret;
    }
    ost->mSampleBuffer->commit_data(ret);
    frame = ost->mFrame;

    // Write converted samples into format context
    int64_t advancedTime = 0;
    while (ost->mSampleBuffer->available()) {
        ost->mSampleBuffer->getChunk(frame->data, 0);

        if (ost->mStartTimestamp == 0) {
            frame->pts = 0;
            ost->mStartTimestamp = frameTimestamp;
        } else frame->pts = frameTimestamp - ost->mStartTimestamp + advancedTime;
        advancedTime += av_rescale_q(c->frame_size, (AVRational) {1, c->sample_rate}, c->time_base);

        ret = writeFrame(ost);
        if (ret != 0) break;
    }

    return ret;
}

int Muxer::writeVideoFrame(uint8_t *srcData, int64_t frameTimestamp) {
    OutputStream *ost = mVideoSt;
    AVFrame *frame = ost->mTmpFrame;
    int ret;

    if (mVideoSt->mSwsCtx) {
        av_image_fill_linesizes(frame->linesize, ost->mSrcPixFmt, ost->mSrcWidth);
        av_image_fill_pointers(frame->data, ost->mSrcPixFmt, ost->mSrcHeight, srcData, frame->linesize);

        ret = sws_scale_frame(ost->mSwsCtx, ost->mFrame, ost->mTmpFrame);
        if (ret < 0) {
            LOGE("Cannot scale image: %s", av_err2str(ret));
            return 0;
        }
    } else {
        frame = ost->mFrame;
        av_image_fill_linesizes(frame->linesize, ost->mSrcPixFmt, ost->mSrcWidth);
        av_image_fill_pointers(frame->data, ost->mSrcPixFmt, ost->mSrcHeight, srcData, frame->linesize);
    }

    frame = ost->mFrame;
    if (ost->mStartTimestamp == 0) {
        frame->pts = 0;
        ost->mStartTimestamp = frameTimestamp;
    } else frame->pts = frameTimestamp - ost->mStartTimestamp;

    return writeFrame(ost);
}

int Muxer::onVideoFrame(AVFrame *frame) {
    AVFrame *tmpFrame = frame;
    if (mVideoSt->mSwsCtx) {
        sws_scale_frame(mVideoSt->mSwsCtx, mVideoSt->mFrame, frame);
        tmpFrame = mVideoSt->mFrame;
        tmpFrame->pts = frame->pts;
    }

    return writeFrame(mVideoSt, tmpFrame);
}

int Muxer::onAudioFrame(AVFrame *frame) {
    AVFrame *tmpFrame = frame;
    if (mAudioSt->mSwrCtx) {
        swr_convert_frame(mAudioSt->mSwrCtx, mAudioSt->mFrame, frame);
        tmpFrame = mAudioSt->mFrame;
        tmpFrame->pts = frame->pts;
    }

    return writeFrame(mAudioSt, tmpFrame);
}

int Muxer::flushCodec(OutputStream *ost) {
    return writeFrame(ost, nullptr);
}

int Muxer::start() {
    if (mState == MuxerState::STARTED || mState == MuxerState::STOPPED) {
        LOGE("Illegal state: %d", mState);
        return 0;
    }
    int ret;
    av_dump_format(mFmtCtx, 0, mFileName, 1);

    // Open the output file, if needed
    if (!(mFmtCtx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&mFmtCtx->pb, mFileName, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("Could not open '%s': %s", mFileName, av_err2str(ret));
            return 0;
        }
    }

    // Write the stream header, if any.
    ret = avformat_write_header(mFmtCtx, nullptr);
    if (ret < 0) {
        LOGE("Error occurred when opening output file: %s", av_err2str(ret));
        return 0;
    }

    mState = MuxerState::STARTED;
    return 1;
}

void Muxer::stop() {
    LOGV("Stopping muxer...");

    // Flush any packets left in encoder
    flushCodec(mAudioSt);
    flushCodec(mVideoSt);

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(mFmtCtx);

    if (!(mFmtCtx->oformat->flags & AVFMT_NOFILE)) {
        // Close the output file.
        avio_closep(&mFmtCtx->pb);
    }

    mState = MuxerState::STOPPED;
}

void Muxer::closeStream(OutputStream *ost) {
    if (ost) {
        if (ost->mCodecCtx) avcodec_free_context(&ost->mCodecCtx);
        if (ost->mFrame) av_frame_free(&ost->mFrame);
        if (ost->mTmpFrame) av_frame_free(&ost->mTmpFrame);
        if (ost->mPacket) av_packet_free(&ost->mPacket);
        if (ost->mSwrCtx) swr_free(&ost->mSwrCtx);
        if (ost->mSwsCtx) sws_freeContext(ost->mSwsCtx);
        if (ost->mSampleBuffer) ost->mSampleBuffer->freeBuffer();
    }
}

void Muxer::release() {
    // Close each codec.
    LOGD("Closing audio stream");
    if (mHasAudio) closeStream(mAudioSt);
    LOGD("Closing video stream");
    if (mHasVideo) closeStream(mVideoSt);
    LOGD("Free format context");
    if (mFmtCtx) avformat_free_context(mFmtCtx);
}

