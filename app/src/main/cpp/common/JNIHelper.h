#ifndef JNI_HELPER_H
#define JNI_HELPER_H

#include "jni.h"

extern JavaVM *jvm;
extern jobject mediaPlayerClass;

extern JNIEnv *getEnv();

#endif //JNI_HELPER_H
