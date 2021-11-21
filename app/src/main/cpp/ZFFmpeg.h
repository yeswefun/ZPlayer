//
// Created by lin on 2021/11/13.
//

#ifndef P00_ZFFMPEG_H
#define P00_ZFFMPEG_H

extern "C" {
#include "libavformat/avformat.h"
#include <pthread.h>
}

#include "ZConstants.h"
#include "ZJniCall.h"
#include "ZAudio.h"

class ZFFmpeg {
public:
    AVFormatContext *pFmtCtx = NULL;
    ZJniCall *zJniCall;
    char *url = NULL;

    ZPlayerState *pPlayerState = NULL;

    ZAudio *pAudio = NULL;

    ZFFmpeg(ZJniCall *zJniCall, const char *url);
    ~ZFFmpeg();
    void releaseZFFmpeg();
    void play();
    void callPlayerOnError(bool isMainThread, int code, const char *text);
    void prepareAsync();
    void prepare(bool isMainThread);
    void initReadPacket();
};


#endif //P00_ZFFMPEG_H
