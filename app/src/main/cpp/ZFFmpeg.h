//
// Created by lin on 2021/11/13.
//

#ifndef P00_ZFFMPEG_H
#define P00_ZFFMPEG_H

#include "ZAudio.h"
#include "ZVideo.h"

class ZFFmpeg {
public:
    AVFormatContext *pFmtCtx = NULL;
    ZJniCall *zJniCall;
    char *url = NULL;

    ZPlayerState *pPlayerState = NULL;

    ZAudio *pAudio = NULL;
    ZVideo *pVideo = NULL;

    ZFFmpeg(ZJniCall *zJniCall, const char *url);
    ~ZFFmpeg();
    void releaseZFFmpeg();
    void play();
    void callPlayerOnError(bool isMainThread, int code, const char *text);
    void prepareAsync();
    void prepare(bool isMainThread);
    void initReadPacket();

    void setSurface(jobject surface);
};


#endif //P00_ZFFMPEG_H
