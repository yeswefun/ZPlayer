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

    if (pAudio) {
        delete pAudio;
        pAudio = NULL;
    }

    if (url) {
        free(url);
        url = NULL;
    }

    if (pCodecCtx) {
        avcodec_free_context(&pCodecCtx);
    }

    if (pFmtCtx) {
        avformat_close_input(&pFmtCtx);
    }
}

void ZFFmpeg::play() {
    if (pAudio) {
        pAudio->play();
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

    // 2. 输出音频的采样率和通道数
    int audioStreamIndex = -1;
    if ((ret = av_find_best_stream(pFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0)) < 0) {
        LOGE("error: av_find_best_stream, %d, %s", ret, av_err2str(ret));
        callPlayerOnError(isMainThread, ERR_AV_FIND_BEST_STREAM, av_err2str(ret));
        return;
    }
    audioStreamIndex = ret;

    AVCodecParameters *pCodecPar = pFmtCtx->streams[audioStreamIndex]->codecpar;
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

    if ((ret = avcodec_parameters_to_context(pCodecCtx, pCodecPar)) < 0) {
        callPlayerOnError(isMainThread, ERR_AVCODEC_PARAMETERS_TO_CONTEXT, av_err2str(ret));
        return;
    }

    if ((ret = avcodec_open2(pCodecCtx, pCodec, NULL)) < 0) {
        callPlayerOnError(isMainThread, ERR_AVCODEC_OPEN2, av_err2str(ret));
        return;
    }

    LOGE("sample_rate: %d, channels: %d", pCodecCtx->sample_rate, pCodecCtx->channels);

    // 初始音频播放器
    pAudio = new ZAudio(zJniCall, pFmtCtx, pCodecCtx, audioStreamIndex);
    pAudio->analyzeAudioStream(isMainThread);

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
