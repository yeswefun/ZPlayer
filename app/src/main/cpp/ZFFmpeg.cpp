//
// Created by lin on 2021/11/13.
//

#include "ZFFmpeg.h"


ZFFmpeg::ZFFmpeg(ZJniCall *zJniCall, const char *url) {
    this->zJniCall = zJniCall;
    this->url = (char *)malloc(strlen(url) + 1);
    strcpy(this->url, url);
}

ZFFmpeg::~ZFFmpeg() {
    releaseZFFmpeg();
}

void ZFFmpeg::releaseZFFmpeg() {

    if (pPlayerState) {
        delete pPlayerState;
        pPlayerState = NULL;
    }

    if (pVideo) {
        delete pVideo;
        pVideo = NULL;
    }

    if (pAudio) {
        delete pAudio;
        pAudio = NULL;
    }

    if (url) {
        free(url);
        url = NULL;
    }

    if (pFmtCtx) {
        avformat_close_input(&pFmtCtx);
    }
}

void ZFFmpeg::initReadPacket() {
    while (pPlayerState != NULL && !pPlayerState->isExit) {
        AVPacket *pPacket = av_packet_alloc();
        if (av_read_frame(pFmtCtx, pPacket) == 0) {
            if (pPacket->stream_index == pAudio->streamIndex) {
                LOGE("read packet - audio");
                pAudio->pPacketQueue->push(pPacket);
            } else if (pPacket->stream_index == pVideo->streamIndex) {
                LOGE("read packet - video");
                pVideo->pPacketQueue->push(pPacket);
            } else {
                av_packet_free(&pPacket);
            }
        } else {
            LOGE("finished read packet ***************");
            break;
        }
    }
}

void* handleReadPacket(void *arg) {
    ZFFmpeg *zFFmpeg = static_cast<ZFFmpeg *>(arg);
    zFFmpeg->initReadPacket();
    return NULL;
}

void ZFFmpeg::play() {

    pthread_t readPacketThread;
    pthread_create(&readPacketThread, NULL, handleReadPacket, this);
    pthread_detach(readPacketThread);

    if (pAudio) {
        pAudio->play();
    }

    if (pVideo) {
        pVideo->play();
    }
}

void ZFFmpeg::callPlayerOnError(bool isMainThread, int code, const char *text) {
    releaseZFFmpeg();
    zJniCall->callPlayerOnError(isMainThread, code, text);
}

void ZFFmpeg::prepare(bool isMainThread) {

    int ret = -1;
    if ((ret = avformat_open_input(&pFmtCtx, url, NULL, NULL)) != 0) {
        callPlayerOnError(isMainThread, ERR_AVFORMAT_OPEN_INPUT, av_err2str(ret));
        return;
    }

    if ((ret = avformat_find_stream_info(pFmtCtx, NULL)) < 0) {
        callPlayerOnError(isMainThread, ERR_AVFORMAT_FIND_STREAM_INFO, av_err2str(ret));
        return;
    }

    // 1. 输出音频metadata
    AVDictionaryEntry *pEntry = NULL;
    while ((pEntry = av_dict_get(pFmtCtx->metadata, "", pEntry, AV_DICT_IGNORE_SUFFIX)) != NULL) {
        LOGE("%s : %s", pEntry->key, pEntry->value);
    }

    // 初始化播放器状态
    pPlayerState = new ZPlayerState();

    // 初始音频播放器
    int audioStreamIndex = -1;
    if ((ret = av_find_best_stream(pFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0)) < 0) {
        LOGE("error: av_find_best_stream, %d, %s", ret, av_err2str(ret));
        callPlayerOnError(isMainThread, ERR_AV_FIND_BEST_STREAM, av_err2str(ret));
        return;
    }
    audioStreamIndex = ret;
    pAudio = new ZAudio(zJniCall, pPlayerState, audioStreamIndex);
    pAudio->analyzeStream(isMainThread, pFmtCtx);

    // 初始视频播放器
    int videoStreamIndex = -1;
    if ((ret = av_find_best_stream(pFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0) {
        LOGE("error: av_find_best_stream, %d, %s", ret, av_err2str(ret));
        callPlayerOnError(isMainThread, ERR_AV_FIND_BEST_STREAM, av_err2str(ret));
        return;
    }
    videoStreamIndex = ret;
    pVideo = new ZVideo(zJniCall, pPlayerState, videoStreamIndex);
    pVideo->analyzeStream(isMainThread, pFmtCtx);

    // 通知java层, native层已经准备完毕
    zJniCall->callPlayerOnPrepared(isMainThread);
}

void* handlePrepareAsync(void *arg) {
    ZFFmpeg *zFFmpeg = static_cast<ZFFmpeg *>(arg);
    zFFmpeg->prepare(false);
    return NULL;
}

void ZFFmpeg::prepareAsync() {
    pthread_t prepareAsyncThread;
    pthread_create(&prepareAsyncThread, NULL, handlePrepareAsync, this);
    pthread_detach(prepareAsyncThread);
}

void ZFFmpeg::setSurface(jobject surface) {
    if (pVideo) {
        pVideo->setSurface(surface);
    }
}
