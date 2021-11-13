//
// Created by lin on 2021/11/13.
//

#ifndef P00_ZFFMPEG_H
#define P00_ZFFMPEG_H

extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <pthread.h>
}

#include "ZConstants.h"
#include "ZJniCall.h"

class ZFFmpeg {
public:
    AVFormatContext *pFmtCtx = NULL;
    AVCodecContext *pCodecCtx = NULL;
    SwrContext *pSwrCtx = NULL;
    uint8_t *pResampleBuffer = NULL;

    ZJniCall *zJniCall;
    char *url = NULL;

    int audioStreamIndex = -1;

    ZFFmpeg(ZJniCall *zJniCall, const char *url);
    ~ZFFmpeg();

    void play();
    void callPlayerOnError(bool isMainThread, int code, const char *text);

    void releaseZFFmpeg();

    void prepare(bool isMainThread);

    void initOpenSLES();

    int resampleAudio();
};


#endif //P00_ZFFMPEG_H
