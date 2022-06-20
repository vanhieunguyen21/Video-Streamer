#include "jni.h"
#include "ffmpeg/Muxer.h"
#include "ffmpeg/MuxerBuilder.h"
#include "ffmpeg/Demuxer.h"
#include "streamer/MediaStreamer.h"
#include "streamer/MediaStreamerBuilder.h"
#include "JNIHelper.h"

#define LOG_TAG "MediaPlayerJNI"

static MediaStreamer *mediaStreamer = nullptr;
static AudioStreamer *audioStreamer = nullptr;
static VideoStreamer *videoStreamer = nullptr;
static char *url = nullptr;
static char *outUrl = nullptr;
static std::mutex mutex; // Mutex to prevent deleting renderer while drawing
static Demuxer *demuxer = nullptr;
static Muxer *muxer = nullptr;
static RendererES3 *renderer = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_com_example_videostreamer_MediaStreamer_create(JNIEnv *env, jobject thiz, jstring jurl, jstring jouturl) {
    // Get url in C string
    const char *utf = env->GetStringUTFChars(jurl, nullptr);
    url = new char[strlen(utf) + 1];
    strcpy(url, utf);
    // Release C string
    env->ReleaseStringUTFChars(jurl, utf);

    utf = env->GetStringUTFChars(jouturl, nullptr);
    outUrl = new char[strlen(utf) + 1];
    strcpy(outUrl, utf);
    // Release C string
    env->ReleaseStringUTFChars(jouturl, utf);


    // Create demuxer
    demuxer = new Demuxer(url);
    if (demuxer->mState != DemuxerState::READY) {
        LOGE("Demuxer is not initiated.");
        return;
    }

    MediaStreamerBuilder builder;
    builder.setAudioTimeBase(demuxer->getAudioTimebase())
            ->setSrcSampleRate(demuxer->getSampleRate())
            ->setSrcChannelLayout(demuxer->getChannelLayout())
            ->setSrcNbSamples(demuxer->getNbSamples())
            ->setSrcSampleFmt(demuxer->getSampleFormat());

    builder.setVideoTimeBase(demuxer->getVideoTimebase())
            ->setSrcWidth(demuxer->getWidth())
            ->setSrcHeight(demuxer->getHeight())
            ->setSrcPixelFormat(demuxer->getPixelFormat())
            ->setRenderer(&renderer);

    mediaStreamer = builder.buildMediaStreamer();
    audioStreamer = mediaStreamer->mAudioStreamer;
    videoStreamer = mediaStreamer->mVideoStreamer;

    demuxer->addAudioSink(mediaStreamer);
    demuxer->addVideoSink(mediaStreamer);

    MuxerBuilder muxerBuilder;
    muxerBuilder.setFileName(outUrl)
            ->setAudioTimeBase(demuxer->getAudioTimebase())
            ->setSrcSampleRate(demuxer->getSampleRate())
            ->setSrcChannelLayout(demuxer->getChannelLayout())
            ->setSrcNbSamples(demuxer->getNbSamples())
            ->setSrcSampleFmt(demuxer->getSampleFormat())
            ->setVideoTimeBase(demuxer->getVideoTimebase())
            ->setSrcWidth(demuxer->getWidth())
            ->setSrcHeight(demuxer->getHeight())
            ->setSrcPixelFormat(demuxer->getPixelFormat());

    muxer = muxerBuilder.buildMuxer();

    demuxer->addAudioSink(muxer);
    demuxer->addVideoSink(muxer);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_videostreamer_MediaStreamer_start(JNIEnv *env, jobject thiz) {
    if (!demuxer) return;
    demuxer->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_videostreamer_MediaStreamer_pause(JNIEnv *env, jobject thiz) {
    if (demuxer) demuxer->pause();
    if (audioStreamer) audioStreamer->pause();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_videostreamer_MediaStreamer_resume(JNIEnv *env, jobject thiz) {
    if (demuxer && demuxer->mState == DemuxerState::PAUSED) demuxer->resume();
    if (audioStreamer) audioStreamer->resume();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_videostreamer_MediaStreamer_stop(JNIEnv *env, jobject thiz) {
    std::unique_lock<std::mutex> lck(mutex);
    delete demuxer;
    demuxer = nullptr;
    delete audioStreamer;
    audioStreamer = nullptr;
    delete videoStreamer;
    videoStreamer = nullptr;
    delete mediaStreamer;
    mediaStreamer = nullptr;
    if (muxer) {
        muxer->stop();
        muxer->release();
    }
    delete muxer;
    muxer = nullptr;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_videostreamer_MediaStreamer_clean(JNIEnv *env, jobject thiz) {
    delete[] url;
    delete demuxer;
    demuxer = nullptr;
    delete audioStreamer;
    audioStreamer = nullptr;
    delete videoStreamer;
    videoStreamer = nullptr;
}

// GLES Renderer function
extern "C"
JNIEXPORT void JNICALL
Java_com_example_videostreamer_GLES3JNILib_00024Companion_init(JNIEnv *env, jobject thiz) {
    if (renderer) {
        delete renderer;
        renderer = nullptr;
    }
    renderer = new RendererES3();
    if (!renderer->init()) {
        LOGE("Failed to initiate GLES renderer.");
        return;
    }

    checkGlError("init");
}

// GLES Renderer function
extern "C"
JNIEXPORT void JNICALL
Java_com_example_videostreamer_GLES3JNILib_00024Companion_resize(JNIEnv *env, jobject thiz, jint width, jint height) {
    if (renderer) {
        renderer->resize(width, height);
    }
}

// GLES Renderer function
extern "C"
JNIEXPORT void JNICALL
Java_com_example_videostreamer_GLES3JNILib_00024Companion_draw(JNIEnv *env, jobject thiz) {
    std::unique_lock<std::mutex> lck(mutex);
    if (videoStreamer && renderer) {
        videoStreamer->render();
    } else if (renderer) {
        renderer->clearSurface();
    }
}