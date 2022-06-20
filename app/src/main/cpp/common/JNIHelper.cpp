#include "JNIHelper.h"
#include "JNILogHelper.h"

#define LOG_TAG "JNIHelper"

static const char mediaPlayerClassName[] = "com/example/videostreamer/MediaStreamer";

JavaVM *jvm = nullptr;
jobject mediaPlayerClass;

extern "C"
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGD("Loading JNI library");
    jvm = vm; // Cache the java VM pointer

    JNIEnv *env = getEnv();
    // Load the class
    mediaPlayerClass = env->NewGlobalRef(env->FindClass(mediaPlayerClassName));

    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    JNIEnv *env = getEnv();
    // Delete global reference
    env->DeleteGlobalRef(mediaPlayerClass);
}

JNIEnv *getEnv() {
    JNIEnv *env;
    int status = jvm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (status < 0) {
        status = jvm->AttachCurrentThread(&env, nullptr);
        if (status < 0) {
            return nullptr;
        }
    }
    return env;
}