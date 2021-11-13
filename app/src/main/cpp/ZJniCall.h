//
// Created by lin on 2021/11/13.
//

#ifndef P00_ZJNICALL_H
#define P00_ZJNICALL_H

#include <jni.h>

class ZJniCall {
public:
    jobject audioTrackObject;
    jmethodID audioTrackWriteMid;

    JNIEnv *jniEnv;
    JavaVM *javaVm;

    ZJniCall(JavaVM *javaVm, JNIEnv *jniEnv);
    ~ZJniCall();
    void callAudioTrackWrite(jbyteArray audioData, int offsetInBytes, int sizeInBytes);

private:
    void createAudioObject();
};


#endif //P00_ZJNICALL_H
