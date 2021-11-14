//
// Created by lin on 2021/11/13.
//

#ifndef P00_ZJNICALL_H
#define P00_ZJNICALL_H

#include <jni.h>
#include "ZConstants.h"

class ZJniCall {
public:
    jobject playerObject = NULL;
    jmethodID playerOnErrorMid;
    jmethodID playerOnPreparedMid;

    JNIEnv *jniEnv;
    JavaVM *javaVm;

    ZJniCall(JavaVM *javaVm, JNIEnv *jniEnv, jobject playerObject);
    ~ZJniCall();
    void callPlayerOnError(bool isMainThread, int code, const char *text);
    void callPlayerOnPrepared(bool isMainThread);
};


#endif //P00_ZJNICALL_H
