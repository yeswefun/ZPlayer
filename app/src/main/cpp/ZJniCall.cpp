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
    this->playerOnPreparedMid = jniEnv->GetMethodID(playerClass, "onPrepared", "()V");
}

ZJniCall::~ZJniCall() {
    if (playerObject) {
        jniEnv->DeleteGlobalRef(playerObject);
        playerObject = NULL;
    }
}

/*
出现异常的处理方式
    1. 回调java层
    2. 释放native层资源

注: 暂时不使用goto来处理, goto之后的代码不能再声明新变量 - 恶心
 */
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

void ZJniCall::callPlayerOnPrepared(bool isMainThread) {
    if (isMainThread) {
        jniEnv->CallVoidMethod(playerObject, playerOnPreparedMid);
    } else {
        JNIEnv *env = NULL;
        if (javaVm->AttachCurrentThread(&env, NULL) != JNI_OK) {
            LOGE("error: failed to AttachCurrentThread, [%s:%d]", __FILE__, __LINE__);
            return;
        }
        env->CallVoidMethod(playerObject, playerOnPreparedMid);
        javaVm->DetachCurrentThread();
    }
}
