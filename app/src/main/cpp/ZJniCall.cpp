//
// Created by lin on 2021/11/13.
//

#include "ZJniCall.h"

ZJniCall::ZJniCall(JavaVM *javaVm, JNIEnv *jniEnv, jobject playerObject) {
    this->javaVm = javaVm;
    this->jniEnv = jniEnv;

    this->playerObject = jniEnv->NewGlobalRef(playerObject);
    jclass playerClass = jniEnv->GetObjectClass(playerObject);
    this->playerOnErrorMid = jniEnv->GetMethodID(playerClass, "onError", "(ILjava/lang/String;)V");
}

ZJniCall::~ZJniCall() {
    if (playerObject) {
        jniEnv->DeleteGlobalRef(playerObject);
        playerObject = NULL;
    }
}

void ZJniCall::callPlayerOnError(bool isMainThread, int code, const char *text) {
    if (isMainThread) {
        jstring jtext = jniEnv->NewStringUTF(text);
        jniEnv->CallVoidMethod(playerObject, playerOnErrorMid, code, jtext);
        jniEnv->DeleteLocalRef(jtext);
    } else {
        JNIEnv *env = NULL;
        if (javaVm->AttachCurrentThread(&env, NULL) != JNI_OK) {
            LOGE("error: failed to AttachCurrentThread, [%s:%d]", __FILE__, __LINE__);
            return;
        }
        jstring jtext = env->NewStringUTF(text);
        env->CallVoidMethod(playerObject, playerOnErrorMid, code, jtext);
        env->DeleteLocalRef(jtext);
        javaVm->DetachCurrentThread();
    }
}
