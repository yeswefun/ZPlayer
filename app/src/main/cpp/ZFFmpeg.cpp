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

    if (url) {
        free(url);
        url = NULL;
    }

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
    zfFmpeg->prepare(false);
    return NULL;
}

void ZFFmpeg::play() {
    pthread_t playThread;
    pthread_create(&playThread, NULL, handlePlay, this);
    pthread_detach(playThread);
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
    audioStreamIndex = -1;
    if ((ret = av_find_best_stream(pFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0))) {
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
        callPlayerOnError(isMainThread, ERR_SWR_ALLOC_SET_OPTS, "error: swr_alloc_set_opts");
        return;
    }

    if ((ret = swr_init(pSwrCtx)) < 0) {
        callPlayerOnError(isMainThread, ERR_SWR_INIT, "error: swr_init");
        return;
    }

    // 5. 解决内存上涨问题
    int out_channels = av_get_channel_layout_nb_channels(out_ch_layout);
    int dataSize = av_samples_get_buffer_size(NULL, out_channels, pCodecCtx->frame_size, out_sample_fmt, 0);
    // 重采样的数据缓冲区
    pResampleBuffer = (uint8_t *)malloc(dataSize);

    this->initOpenSLES();
}


void playerCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    ZFFmpeg *zFFmpeg = static_cast<ZFFmpeg *>(pContext);
    int dataSize = zFFmpeg->resampleAudio();
    (*caller)->Enqueue(caller, zFFmpeg->pResampleBuffer, dataSize);
}

void ZFFmpeg::initOpenSLES() {
    /*OpenSLES OpenGLES 都是自带的
    XXXES 与 XXX 之间可以说是基本没有区别，区别就是 XXXES 是 XXX 的精简
    而且他们都有一定规则，命名规则 slXXX() , glXXX3f*/

    // 3.1 创建引擎接口对象
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine;
    slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    // realize the engine
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    // get the engine interface, which is needed in order to create other objects
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);

    // 3.2 设置混音器
    static SLObjectItf outputMixObject = NULL;
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                     &outputMixEnvironmentalReverb);
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
    (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb,
                                                                      &reverbSettings);

    // 3.3 创建播放器
    SLObjectItf pPlayer = NULL;
    SLPlayItf pPlayItf = NULL;
    SLDataLocator_AndroidSimpleBufferQueue simpleBufferQueue = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2
    };
    SLDataFormat_PCM formatPcm = {
            SL_DATAFORMAT_PCM,
            2,
            SL_SAMPLINGRATE_44_1,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN
    };
    SLDataSource audioSrc = {&simpleBufferQueue, &formatPcm};
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, NULL};
    SLInterfaceID interfaceIds[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_PLAYBACKRATE};
    SLboolean interfaceRequired[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    (*engineEngine)->CreateAudioPlayer(engineEngine, &pPlayer, &audioSrc, &audioSnk, 3,
                                       interfaceIds, interfaceRequired);
    (*pPlayer)->Realize(pPlayer, SL_BOOLEAN_FALSE);
    (*pPlayer)->GetInterface(pPlayer, SL_IID_PLAY, &pPlayItf);

    // 3.4 设置缓存队列和回调函数 - this是每次回调传入的参数
    SLAndroidSimpleBufferQueueItf playerBufferQueue;
    (*pPlayer)->GetInterface(pPlayer, SL_IID_BUFFERQUEUE, &playerBufferQueue);
    (*playerBufferQueue)->RegisterCallback(playerBufferQueue, playerCallback, this);

    // 3.5 设置播放状态
    (*pPlayItf)->SetPlayState(pPlayItf, SL_PLAYSTATE_PLAYING);

    // 3.6 调用回调函数
    playerCallback(playerBufferQueue, this);
}

int ZFFmpeg::resampleAudio() {
    int dataSize = 0;
    // 3. 解码音频帧
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();
    while (av_read_frame(pFmtCtx, pPacket) == 0) {
        if (pPacket->stream_index == audioStreamIndex) {
            if (avcodec_send_packet(pCodecCtx, pPacket) == 0) {
                if (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                    // number of samples output per channel
                    dataSize = swr_convert(pSwrCtx, &pResampleBuffer, pFrame->nb_samples, (const uint8_t **)pFrame->data, pFrame->nb_samples);
                    break;
                }
            }
        }
        av_packet_unref(pPacket);
        av_frame_unref(pFrame);
    }
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    // 两个通道，每个通道的采样点dataSize，每个采样点存储为2个字节
    return dataSize * 2 * 2;
}
