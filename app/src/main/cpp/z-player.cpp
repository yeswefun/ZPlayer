#include <jni.h>
#include <string>

#include "ZConstants.h"
#include "ZJniCall.h"
#include "ZFFmpeg.h"

ZJniCall *zJniCall = NULL;
ZFFmpeg *zFFmpeg = NULL;

/*
    可以使用goto, 但是此处不用
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_z_p00_player_ZPlayer_nPlay(JNIEnv *env, jobject obj, jstring url) {

    const char *nurl = env->GetStringUTFChars(url, NULL);
    LOGE("(native)file path: %s", nurl);

    zJniCall = new ZJniCall(NULL, env);
    zFFmpeg = new ZFFmpeg(zJniCall, nurl);
    zFFmpeg->play();

    if (zFFmpeg) {
        delete zFFmpeg;
        zFFmpeg = NULL;
    }

    if (zJniCall) {
        delete zJniCall;
        zJniCall = NULL;
    }

    env->ReleaseStringUTFChars(url, nurl);
}