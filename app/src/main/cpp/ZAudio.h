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
#include "ZPacketQueue.h"
#include "ZPlayerState.h"
#include "ZMedia.h"

class ZAudio : public ZMedia {
public:
    SwrContext *pSwrCtx = NULL;
    uint8_t *pResampleBuffer = NULL;

    ZAudio(ZJniCall *zJniCall, ZPlayerState *pPlayerState, int audioStreamIndex);
    ~ZAudio();
    void releaseZAudio();

    int resampleAudio();
    void initOpenSLES();
    void play();

protected:
    void analyzeStreamInternal(bool isMainThread, AVFormatContext *pFmtCtx);
};


#endif //P00_ZAUDIO_H
