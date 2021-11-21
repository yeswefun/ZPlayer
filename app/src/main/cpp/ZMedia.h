//
// Created by lin on 2021/11/21.
//

#ifndef P00_ZMEDIA_H
#define P00_ZMEDIA_H

extern "C" {
#include <pthread.h>
#include "libavformat/avformat.h"
};
#include "ZJniCall.h"
#include "ZPlayerState.h"
#include "ZPacketQueue.h"

class ZMedia {
public:
    int streamIndex = -1;
    AVCodecContext *pCodecCtx;
    ZJniCall *zJniCall;

    ZPlayerState *pPlayerState;
    ZPacketQueue *pPacketQueue = NULL;

    ZMedia(ZJniCall *zJniCall, ZPlayerState *pPlayerState, int streamIndex);
    ~ZMedia();
    void releaseZMedia();

    void callPlayerOnError(bool isMainThread, int code, const char *text);

    // 子类调用
    void analyzeStream(bool isMainThread, AVFormatContext *pFmtCtx);

    virtual void play() = 0;

protected:
    void analyzeStreamCommon(bool isMainThread, AVFormatContext *pFmtCtx);
    virtual void analyzeStreamInternal(bool isMainThread, AVFormatContext *pFmtCtx) = 0;
};


#endif //P00_ZMEDIA_H
