//
// Created by lin on 2021/11/14.
//

#include "ZAudio.h"

ZAudio::ZAudio(ZJniCall *zJniCall, ZPlayerState *pPlayerState, int audioStreamIndex)
        : ZMedia(zJniCall, pPlayerState, audioStreamIndex) {
}

ZAudio::~ZAudio() {
    releaseZAudio();
}

void ZAudio::releaseZAudio() {

    ZMedia::releaseZMedia();

    if (pResampleBuffer) {
        free(pResampleBuffer);
        pResampleBuffer = NULL;
    }

    if (pSwrCtx) {
        swr_free(&pSwrCtx);
    }
}

void playerCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    ZAudio *pAudio = static_cast<ZAudio *>(pContext);
    int dataSize = pAudio->resampleAudio();
    (*caller)->Enqueue(caller, pAudio->pResampleBuffer, dataSize);
}

void ZAudio::initOpenSLES() {
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

int ZAudio::resampleAudio() {
    int dataSize = 0;
    AVPacket *pPacket = NULL;
    AVFrame *pFrame = av_frame_alloc();
    while (pPlayerState != NULL && !pPlayerState->isExit) {
        pPacket = pPacketQueue->pop();
        if (avcodec_send_packet(pCodecCtx, pPacket) == 0) {
            if (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                // number of samples output per channel
                dataSize = swr_convert(pSwrCtx, &pResampleBuffer, pFrame->nb_samples,
                                       (const uint8_t **) pFrame->data, pFrame->nb_samples);
                break;
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

void *handlePlayAudio(void *arg) {
    ZAudio *pAudio = static_cast<ZAudio *>(arg);
    pAudio->initOpenSLES();
    return NULL;
}

void ZAudio::play() {
    pthread_t playThread;
    pthread_create(&playThread, NULL, handlePlayAudio, this);
    pthread_detach(playThread);
}

void ZAudio::analyzeStreamInternal(bool isMainThread, AVFormatContext *pFmtCtx) {
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

    int ret = -1;
    if ((ret = swr_init(pSwrCtx)) < 0) {
        callPlayerOnError(isMainThread, ERR_SWR_INIT, "error: swr_init");
        return;
    }

    // 5. 解决内存上涨问题
    int out_channels = av_get_channel_layout_nb_channels(out_ch_layout);
    int dataSize = av_samples_get_buffer_size(NULL, out_channels, pCodecCtx->frame_size,
                                              out_sample_fmt, 0);

    // 重采样的数据缓冲区
    pResampleBuffer = (uint8_t *) malloc(dataSize);

    LOGE("sample_rate: %d, channels: %d", pCodecCtx->sample_rate, pCodecCtx->channels);
}

/*
    encoded packet -> decoded frame -> frame->data == pcm data

    canvas -> surface view -> dead loop -> every 16ms -> render once

    AudioTrack

    pFrame->data(c@data) -> write(java@data)

    一秒数据的大小 != 一帧数据的大小, 一秒包含多个帧

    一帧pcm的数据大小
        采样点个数: pFrame->nb_samples, 44100个采样点
        通道数: pFrame->channels, 2个通道
        每个通道16位, 即2个字节

        44100 * 2 * 2 (bytes)
 */

/*
E/JNI_TAG: frame: 1
    frame - channels: 2, nb_samples: 47, sample_fmt: 8
    codecCtx - channels: 2, nb_samples: 1152, sample_fmt: 8
    dataSize: 4608
E/JNI_TAG: frame: 2
    frame - channels: 2, nb_samples: 1152, sample_fmt: 8
    codecCtx - channels: 2, nb_samples: 1152, sample_fmt: 8
    dataSize: 4608
E/JNI_TAG: frame: 3
    frame - channels: 2, nb_samples: 1152, sample_fmt: 8
    codecCtx - channels: 2, nb_samples: 1152, sample_fmt: 8
    dataSize: 4608

LOGE("frame - channels: %d, nb_samples: %d, sample_fmt: %d", frame->channels, frame->nb_samples, codecCtx->sample_fmt);
LOGE("codecCtx - channels: %d, nb_samples: %d, sample_fmt: %d", codecCtx->channels, codecCtx->frame_size, codecCtx->sample_fmt);
LOGE("dataSize: %d", dataSize);
 */
