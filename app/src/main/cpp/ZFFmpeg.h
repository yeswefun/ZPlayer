//
// Created by lin on 2021/11/13.
//

#ifndef P00_ZFFMPEG_H
#define P00_ZFFMPEG_H

extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
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
    char *url;

    ZFFmpeg(ZJniCall *zJniCall, const char *url);
    ~ZFFmpeg();

    void play();
    void callPlayerOnError(int code, const char *text);

    void releaseZFFmpeg();
};


#endif //P00_ZFFMPEG_H
