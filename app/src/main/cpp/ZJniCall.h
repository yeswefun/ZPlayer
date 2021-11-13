//
// Created by lin on 2021/11/13.
//

#ifndef P00_ZJNICALL_H
#define P00_ZJNICALL_H

#include <jni.h>
#include "ZConstants.h"

class ZJniCall {
public:
    jobject audioTrackObject;
    jmethodID audioTrackWriteMid;

    jobject playerObject = NULL;
    jmethodID playerOnErrorMid;

    JNIEnv *jniEnv;
    JavaVM *javaVm;

    ZJniCall(JavaVM *javaVm, JNIEnv *jniEnv, jobject playerObject);
    ~ZJniCall();
    void callAudioTrackWrite(jbyteArray audioData, int offsetInBytes, int sizeInBytes);
    void callPlayerOnError(bool isMainThread, int code, const char *text);

private:
    void createAudioObject();
};


#endif //P00_ZJNICALL_H
