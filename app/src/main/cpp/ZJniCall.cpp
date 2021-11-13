//
// Created by lin on 2021/11/13.
//

#include "ZJniCall.h"

ZJniCall::ZJniCall(JavaVM *javaVm, JNIEnv *jniEnv) {
    this->javaVm = javaVm;
    this->jniEnv = jniEnv;
    createAudioObject();
}

ZJniCall::~ZJniCall() {

}

/*
使用AudioTrack播放pcm音频数据
    启动播放循环 - play
    往播放缓冲区写入数据 - write

public AudioTrack(int streamType, int sampleRateInHz, int channelConfig,
                int audioFormat, int bufferSizeInBytes, int mode)

static public int getMinBufferSize(int sampleRateInHz, int channelConfig, int audioFormat)

void play()

public int write(@NonNull byte[] audioData, int offsetInBytes, int sizeInBytes)
 */
void ZJniCall::createAudioObject() {

    int streamType = 3;             // 1. STREAM_MUSIC = 3
    int sampleRateInHz = 44100;     // 2. TODO: why 44100?
    int channelConfig = 0x4 | 0x8;  // 3. CHANNEL_OUT_STEREO = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT)
    int audioFormat = 2;            // 4. ENCODING_PCM_16BIT = 2
    int mode = 1;                   // 6. MODE_STREAM = 1, 一次0，多次1

    jclass audioTrackClass = jniEnv->FindClass("android/media/AudioTrack");

    // 5. bufferSizeInBytes
    jmethodID audioTrackGetMinBufferSizeMid = jniEnv->GetStaticMethodID(audioTrackClass, "getMinBufferSize", "(III)I");
    int bufferSizeInBytes = jniEnv->CallStaticIntMethod(audioTrackClass, audioTrackGetMinBufferSizeMid, sampleRateInHz, channelConfig, audioFormat);

    // init
    jmethodID audioTrackInitMid = jniEnv->GetMethodID(audioTrackClass, "<init>", "(IIIIII)V");
    audioTrackObject = jniEnv->NewObject(audioTrackClass, audioTrackInitMid,
                                              streamType, sampleRateInHz, channelConfig, audioFormat, bufferSizeInBytes, mode);

    // write
    audioTrackWriteMid = jniEnv->GetMethodID(audioTrackClass, "write", "([BII)I");

    // play - 启动播放循环
    jmethodID audioTrackPlayMid = jniEnv->GetMethodID(audioTrackClass, "play", "()V");
    jniEnv->CallVoidMethod(audioTrackObject, audioTrackPlayMid);
}

void ZJniCall::callAudioTrackWrite(jbyteArray audioData, int offsetInBytes, int sizeInBytes) {
    jniEnv->CallIntMethod(audioTrackObject, audioTrackWriteMid, audioData, offsetInBytes, sizeInBytes);
}
