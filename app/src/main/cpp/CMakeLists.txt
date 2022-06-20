cmake_minimum_required(VERSION 3.18.1)

project("videostreamer")

set(ffmpeg_DIR ${CMAKE_SOURCE_DIR}/ffmpeg)

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION
        ${ffmpeg_DIR}/lib/${ANDROID_ABI}/libavutil.so)

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION
        ${ffmpeg_DIR}/lib/${ANDROID_ABI}/libavcodec.so)

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION
        ${ffmpeg_DIR}/lib/${ANDROID_ABI}/libavformat.so)

add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION
        ${ffmpeg_DIR}/lib/${ANDROID_ABI}/libswscale.so)

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION
        ${ffmpeg_DIR}/lib/${ANDROID_ABI}/libswresample.so)

add_library(x264 SHARED IMPORTED)
set_target_properties(x264 PROPERTIES IMPORTED_LOCATION
        ${ffmpeg_DIR}/lib/${ANDROID_ABI}/libx264.so)

include_directories(${ffmpeg_DIR}/lib/${ANDROID_ABI}/include)

find_package(oboe REQUIRED CONFIG)

include_directories(common)
include_directories(ffmpeg)
include_directories(gles)
include_directories(streamer)

set(
        FFMPEG_SRC
        ffmpeg/Muxer.cpp ffmpeg/Muxer.h
        ffmpeg/MuxerBuilder.cpp ffmpeg/MuxerBuilder.h
        ffmpeg/Demuxer.cpp ffmpeg/Demuxer.h
        ffmpeg/SampleBuffer.cpp ffmpeg/SampleBuffer.h
        ffmpeg/FFmpegHelper.cpp ffmpeg/FFmpegHelper.h
)

set(
        GLES_SRC
        gles/NDKHelper.h
        gles/RendererES3.h
        gles/GLES3Helper.cpp gles/GLES3Helper.h
        gles/RendererES3.cpp
)

set(
        COMMON_SRC
        common/Sink.h
        common/JNILogHelper.h
        common/JNIHelper.cpp common/JNIHelper.h
        common/TimeUtils.cpp common/TimeUtils.h
)

set(
        STREAMER_SRC
        streamer/MediaStreamer.h streamer/MediaStreamer.cpp
        streamer/MediaStreamerBuilder.h streamer/MediaStreamerBuilder.cpp
        streamer/FrameBuffer.h streamer/FrameBuffer.cpp
        streamer/AudioStreamerBuilder.h streamer/AudioStreamerBuilder.cpp
        streamer/AudioStreamer.h streamer/AudioStreamer.cpp
        streamer/VideoStreamerBuilder.h streamer/VideoStreamerBuilder.cpp
        streamer/VideoStreamer.h streamer/VideoStreamer.cpp
)

add_library(
        videostreamer SHARED
        MediaStreamerJNI.cpp
        ${FFMPEG_SRC}
        ${GLES_SRC}
        ${COMMON_SRC}
        ${STREAMER_SRC}
)

target_link_libraries(
        videostreamer
        avutil avcodec avformat swscale swresample x264
        log jnigraphics EGL GLESv3 oboe::oboe
)
