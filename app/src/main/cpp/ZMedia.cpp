//
// Created by lin on 2021/11/21.
//

#include "ZMedia.h"

ZMedia::ZMedia(ZJniCall *zJniCall, ZPlayerState *pPlayerState, int streamIndex) {
    this->zJniCall = zJniCall;
    this->streamIndex = streamIndex;
    this->pPlayerState = pPlayerState;
    this->pPacketQueue = new ZPacketQueue();
}

ZMedia::~ZMedia() {
    releaseZMedia();
}

void ZMedia::releaseZMedia() {
    if (pPacketQueue) {
        delete pPacketQueue;
        pPacketQueue = NULL;
    }
    if (pCodecCtx) {
        avcodec_free_context(&pCodecCtx);
    }
}

void ZMedia::callPlayerOnError(bool isMainThread, int code, const char *text) {
    releaseZMedia();
    zJniCall->callPlayerOnError(isMainThread, code, text);
}

void ZMedia::analyzeStream(bool isMainThread, AVFormatContext *pFmtCtx) {
    analyzeStreamCommon(isMainThread, pFmtCtx);
    analyzeStreamInternal(isMainThread, pFmtCtx);
}

void ZMedia::analyzeStreamCommon(bool isMainThread, AVFormatContext *pFmtCtx) {

    AVCodecParameters *pCodecPar = pFmtCtx->streams[streamIndex]->codecpar;
    AVCodec *pCodec = avcodec_find_decoder(pCodecPar->codec_id);
    if (pCodec == NULL) {
        callPlayerOnError(isMainThread, ERR_AVCODEC_FIND_DECODER, "error: avcodec_find_decoder");
        return;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (pCodecCtx == NULL) {
        callPlayerOnError(isMainThread, ERR_AVCODEC_ALLOC_CONTEXT3, "error: avcodec_alloc_context3");
        return;
    }

    int ret = -1;
    if ((ret = avcodec_parameters_to_context(pCodecCtx, pCodecPar)) < 0) {
        callPlayerOnError(isMainThread, ERR_AVCODEC_PARAMETERS_TO_CONTEXT, av_err2str(ret));
        return;
    }

    if ((ret = avcodec_open2(pCodecCtx, pCodec, NULL)) < 0) {
        callPlayerOnError(isMainThread, ERR_AVCODEC_OPEN2, av_err2str(ret));
        return;
    }
}



