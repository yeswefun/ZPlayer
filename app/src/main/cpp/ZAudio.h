//
// Created by lin on 2021/11/14.
//

#ifndef P00_ZAUDIO_H
#define P00_ZAUDIO_H

extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <pthread.h>
}

#include "ZConstants.h"
#include "ZJniCall.h"

class ZAudio {
public:
    AVFormatContext *pFmtCtx = NULL;
    AVCodecContext *pCodecCtx = NULL;
    SwrContext *pSwrCtx = NULL;

    uint8_t *pResampleBuffer = NULL;
    int audioStreamIndex = -1;
    ZJniCall *zJniCall;

    ZAudio(ZJniCall *zJniCall, AVFormatContext *pFmtCtx, AVCodecContext *pCodecCtx, int audioStreamIndex);
    ~ZAudio();
    void releaseZAudio();

    void initOpenSLES();
    int resampleAudio();
    void analyzeAudioStream(bool isMainThread);

    void play();
};


#endif //P00_ZAUDIO_H
