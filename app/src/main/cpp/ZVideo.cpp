//
// Created by lin on 2021/11/21.
//

#include "ZVideo.h"

ZVideo::ZVideo(ZJniCall *zJniCall, ZPlayerState *pPlayerState, int videoStreamIndex)
: ZMedia(zJniCall, pPlayerState, videoStreamIndex) {

}

ZVideo::~ZVideo() {
    releaseZVideo();
}

void ZVideo::releaseZVideo() {

    ZMedia::releaseZMedia();

    if (surface) {
        zJniCall->jniEnv->DeleteGlobalRef(surface);
        surface = NULL;
    }

    if (pSwsCtx) {
        sws_freeContext(pSwsCtx);
        pSwsCtx = NULL;
    }

    if (pFrameBuffer) {
        free(pFrameBuffer);
        pFrameBuffer = NULL;
    }

    if (pFrameRGBA) {
        av_frame_free(&pFrameRGBA);
    }
}

void *handlePlayVideo(void *arg) {
    ZVideo *pVideo = static_cast<ZVideo *>(arg);
    pVideo->playVideoWithSurface();
    return NULL;
}

void ZVideo::play() {
    pthread_t playThread;
    pthread_create(&playThread, NULL, handlePlayVideo, this);
    pthread_detach(playThread);
}

void ZVideo::playVideoWithSurface() {

    // 使用SurfaceView播放视频数据
    //ANativeWindow *pNativeWindow = ANativeWindow_fromSurface(env, surface);
    JNIEnv *env = NULL;
    if (zJniCall->javaVm->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOGE("error: failed to AttachCurrentThread, [%s:%d]", __FILE__, __LINE__);
        return;
    }
    ANativeWindow *pNativeWindow = ANativeWindow_fromSurface(env, surface);
    zJniCall->javaVm->DetachCurrentThread();

    // 设置窗口的宽，高，像素格式
    ANativeWindow_setBuffersGeometry(pNativeWindow, pCodecCtx->width, pCodecCtx->height, WINDOW_FORMAT_RGBA_8888);

    // SurfaceView的播放缓冲
    ANativeWindow_Buffer outputBuffer;

    AVPacket *pPacket = NULL;
    AVFrame *pFrame = av_frame_alloc();
    while (pPlayerState != NULL && !pPlayerState->isExit) {
        pPacket = pPacketQueue->pop();
        if (avcodec_send_packet(pCodecCtx, pPacket) == 0) {
            if (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                // 视频缩放处理
                sws_scale(pSwsCtx,
                          pFrame->data, pFrame->linesize, 0, pFrame->height,  // src
                          pFrameRGBA->data, pFrameRGBA->linesize  // dst
                );

                ANativeWindow_lock(pNativeWindow, &outputBuffer, NULL);
                memcpy(outputBuffer.bits, pFrameBuffer, frameSize);
                ANativeWindow_unlockAndPost(pNativeWindow);
            }
        }
        av_packet_unref(pPacket);
        av_frame_unref(pFrame);
    }
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
}

void ZVideo::analyzeStreamInternal(bool isMainThread, AVFormatContext *pFmtCtx) {

    // 对视频进行缩放处理
    pSwsCtx = sws_getContext(
            pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,        // src
            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGBA, // dst
            SWS_BILINEAR, NULL, NULL, NULL);

    // 转换后的格式所对应的内存大小
    frameSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height, 1);
    pFrameBuffer = (uint8_t *)malloc(frameSize);

    // 为pFrameRGB挂上buffer, 这个buffer是用于存缓冲数据
    pFrameRGBA = av_frame_alloc();
    av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize,    // dst, TODO: 这个函数到底做了什么?
                         pFrameBuffer,                               // src
                         AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height, 1);
}

void ZVideo::setSurface(jobject surface) {
    JNIEnv *env = NULL;
    if (zJniCall->javaVm->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOGE("error: failed to AttachCurrentThread, [%s:%d]", __FILE__, __LINE__);
        return;
    }
    this->surface = env->NewGlobalRef(surface);
    //TODO: 会不会有问题呀?
    //zJniCall->javaVm->DetachCurrentThread();

    //TODO: why cannot be?
    //this->surface = zJniCall->jniEnv->NewGlobalRef(obj);
}
