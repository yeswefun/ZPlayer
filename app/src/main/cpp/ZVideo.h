//
// Created by lin on 2021/11/21.
//

#ifndef P00_ZVIDEO_H
#define P00_ZVIDEO_H

extern "C" {
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

// target_link_libraries -> android
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "ZMedia.h"

class ZVideo : public ZMedia {
public:

    SwsContext *pSwsCtx = NULL;
    int frameSize = -1;
    uint8_t *pFrameBuffer = NULL;
    AVFrame *pFrameRGBA = NULL;

    jobject surface = NULL;

    ZVideo(ZJniCall *zJniCall, ZPlayerState *pPlayerState, int videoStreamIndex);
    ~ZVideo();
    void releaseZVideo();

    void play();
    void playVideoWithSurface();
    void setSurface(jobject surface);

protected:
    void analyzeStreamInternal(bool isMainThread, AVFormatContext *pFmtCtx);
};


#endif //P00_ZVIDEO_H
