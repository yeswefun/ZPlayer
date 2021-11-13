//
// Created by lin on 2021/11/13.
//

#include "ZFFmpeg.h"


ZFFmpeg::ZFFmpeg(ZJniCall *zJniCall, const char *url) {
    this->zJniCall = zJniCall;
    this->url = (char *)url;
}

ZFFmpeg::~ZFFmpeg() {
    releaseZFFmpeg();
}

void ZFFmpeg::releaseZFFmpeg() {
    if (pResampleBuffer) {
        free(pResampleBuffer);
        pResampleBuffer = NULL;
    }

    if (pSwrCtx) {
        swr_free(&pSwrCtx);
    }

    if (pCodecCtx) {
        avcodec_free_context(&pCodecCtx);
    }

    if (pFmtCtx) {
        avformat_close_input(&pFmtCtx);
    }
}

void* handlePlay(void *arg) {
    ZFFmpeg *zfFmpeg = static_cast<ZFFmpeg *>(arg);
    zfFmpeg->prepare();
    return NULL;
}

void ZFFmpeg::play() {
    pthread_t playThread;
    pthread_create(&playThread, NULL, handlePlay, this);
    pthread_detach(playThread);
}

void ZFFmpeg::callPlayerOnError(int code, const char *text) {
    releaseZFFmpeg();
    zJniCall->callPlayerOnError(code, text);
}

void ZFFmpeg::prepare() {
    int ret = -1;
    if ((ret = avformat_open_input(&pFmtCtx, url, NULL, NULL)) != 0) {
        callPlayerOnError(ERR_AVFORMAT_OPEN_INPUT, av_err2str(ret));
        return;
    }

    if ((ret = avformat_find_stream_info(pFmtCtx, NULL)) < 0) {
        callPlayerOnError(ERR_AVFORMAT_FIND_STREAM_INFO, av_err2str(ret));
        return;
    }

    // 1. 输出音频metadata
    AVDictionaryEntry *pEntry = NULL;
    while ((pEntry = av_dict_get(pFmtCtx->metadata, "", pEntry, AV_DICT_IGNORE_SUFFIX)) != NULL) {
        LOGE("%s : %s", pEntry->key, pEntry->value);
    }

    // 2. 输出音频的采样率和通道数
    int audioStreamIndex = -1;
    if ((ret = av_find_best_stream(pFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0))) {
        LOGE("error: av_find_best_stream, %d, %s", ret, av_err2str(ret));
        callPlayerOnError(ERR_AV_FIND_BEST_STREAM, av_err2str(ret));
        return;
    }
    audioStreamIndex = ret;

    AVCodecParameters *pCodecPar = pFmtCtx->streams[audioStreamIndex]->codecpar;
    AVCodec *pCodec = avcodec_find_decoder(pCodecPar->codec_id);
    if (pCodec == NULL) {
        callPlayerOnError(ERR_AVCODEC_FIND_DECODER, "error: avcodec_find_decoder");
        return;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (pCodecCtx == NULL) {
        callPlayerOnError(ERR_AVCODEC_ALLOC_CONTEXT3, "error: avcodec_alloc_context3");
        return;
    }

    if ((ret = avcodec_parameters_to_context(pCodecCtx, pCodecPar)) < 0) {
        callPlayerOnError(ERR_AVCODEC_PARAMETERS_TO_CONTEXT, av_err2str(ret));
        return;
    }

    if ((ret = avcodec_open2(pCodecCtx, pCodec, NULL)) < 0) {
        callPlayerOnError(ERR_AVCODEC_OPEN2, av_err2str(ret));
        return;
    }

    LOGE("sample_rate: %d, channels: %d", pCodecCtx->sample_rate, pCodecCtx->channels);

    /*
        6. 解决音频噪音问题 - 音频重采样
        struct SwrContext *swr_alloc_set_opts(struct SwrContext *s,
          int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
          int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
          int log_offset, void *log_ctx);
     */
    int64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int out_sample_rate = 44100;

    int64_t in_ch_layout = pCodecCtx->channel_layout;
    enum AVSampleFormat in_sample_fmt = pCodecCtx->sample_fmt;
    int in_sample_rate = pCodecCtx->sample_rate;

    pSwrCtx = swr_alloc_set_opts(NULL,
                                 out_ch_layout, out_sample_fmt, out_sample_rate,
                                 in_ch_layout, in_sample_fmt, in_sample_rate,
                                 0, NULL);
    if (pSwrCtx == NULL) {
        callPlayerOnError(ERR_SWR_ALLOC_SET_OPTS, "error: swr_alloc_set_opts");
        return;
    }

    if ((ret = swr_init(pSwrCtx)) < 0) {
        callPlayerOnError(ERR_SWR_INIT, "error: swr_init");
        return;
    }

    // 5. 解决内存上涨问题
    int out_channels = av_get_channel_layout_nb_channels(out_ch_layout);
    int dataSize = av_samples_get_buffer_size(NULL, out_channels, pCodecCtx->frame_size, out_sample_fmt, 0);

    // AudioTrack播放音频的数据缓冲区
    jbyteArray pcmByteArray = zJniCall->jniEnv->NewByteArray(dataSize);
    jbyte *pcmData = zJniCall->jniEnv->GetByteArrayElements(pcmByteArray, NULL);

    // 重采样的数据缓冲区
    pResampleBuffer = (uint8_t *)malloc(dataSize);

    // 3. 解码音频帧
    int frameIndex = 0;
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();
    while (av_read_frame(pFmtCtx, pPacket) == 0) {
        if (pPacket->stream_index == audioStreamIndex) {
            if (avcodec_send_packet(pCodecCtx, pPacket) == 0) {
                if (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                    ++frameIndex;
                    LOGE("frame: %d", frameIndex);

                    swr_convert(pSwrCtx, &pResampleBuffer, pFrame->nb_samples, (const uint8_t **)pFrame->data, pFrame->nb_samples);

                    memcpy(pcmData, pResampleBuffer, dataSize);
                    zJniCall->jniEnv->ReleaseByteArrayElements(pcmByteArray, pcmData, JNI_COMMIT);
                    zJniCall->callAudioTrackWrite(pcmByteArray, 0, dataSize);
                }
            }
        }
        av_packet_unref(pPacket);
        av_frame_unref(pFrame);
    }
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);

    // 解决内存上涨问题
    zJniCall->jniEnv->ReleaseByteArrayElements(pcmByteArray, pcmData, 0);
    zJniCall->jniEnv->DeleteLocalRef(pcmByteArray);
}

