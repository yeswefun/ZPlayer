#include <jni.h>
#include <string>

#include "ZConstants.h"
#include "ZJniCall.h"
#include "ZFFmpeg.h"

ZJniCall *zJniCall = NULL;
ZFFmpeg *zFFmpeg = NULL;
JavaVM *javaVM = NULL;

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGE("shared library loaded!");
    javaVM = vm;
    JNIEnv *env = NULL;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("error: failed to GetEnv, [%s:%d]", __FILE__, __LINE__);
        return JNI_ERR;
    }
    return JNI_VERSION_1_4;
}

/*
    可以使用goto, 但是此处不用
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_z_p00_player_ZPlayer_nPlay(JNIEnv *env, jobject obj, jstring url) {

    const char *nurl = env->GetStringUTFChars(url, NULL);
    LOGE("(native)file path: %s", nurl);

    zJniCall = new ZJniCall(NULL, env, obj);
    zFFmpeg = new ZFFmpeg(zJniCall, nurl);
    zFFmpeg->play();

//    if (zFFmpeg) {
//        delete zFFmpeg;
//        zFFmpeg = NULL;
//    }
//
//    if (zJniCall) {
//        delete zJniCall;
//        zJniCall = NULL;
//    }

    env->ReleaseStringUTFChars(url, nurl);
}