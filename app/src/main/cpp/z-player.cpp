#include <jni.h>
#include <string>

#include <android/log.h>

#ifndef LOG_TAG
#define LOG_TAG "JNI_TAG"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif



extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}


/*
使用AudioTrack播放pcm音频数据
    启动播放循环 - play
    往播放缓冲区写入数据 - write

public AudioTrack(int streamType, int sampleRateInHz, int channelConfig,
                int audioFormat, int bufferSizeInBytes, int mode)

static public int getMinBufferSize(int sampleRateInHz, int channelConfig, int audioFormat)

void play()

public int write(@NonNull byte[] audioData, int offsetInBytes, int sizeInBytes)
 */
jobject createAudioObject(JNIEnv *env) {

    int streamType = 3;             // 1. STREAM_MUSIC = 3
    int sampleRateInHz = 44100;     // 2. TODO: why 44100?
    int channelConfig = 0x4 | 0x8;  // 3. CHANNEL_OUT_STEREO = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT)
    int audioFormat = 2;            // 4. ENCODING_PCM_16BIT = 2
    int mode = 1;                   // 6. MODE_STREAM = 1, 一次0，多次1

    jclass audioTrackClass = env->FindClass("android/media/AudioTrack");

    // 5. bufferSizeInBytes
    jmethodID audioTrackGetMinBufferSizeMid = env->GetStaticMethodID(audioTrackClass, "getMinBufferSize", "(III)I");
    int bufferSizeInBytes = env->CallStaticIntMethod(audioTrackClass, audioTrackGetMinBufferSizeMid, sampleRateInHz, channelConfig, audioFormat);

    jmethodID audioTrackInitMid = env->GetMethodID(audioTrackClass, "<init>", "(IIIIII)V");
    jobject audioTrackObject = env->NewObject(audioTrackClass, audioTrackInitMid,
            streamType, sampleRateInHz, channelConfig, audioFormat, bufferSizeInBytes, mode);

    // 启动播放循环
    jmethodID audioTrackPlayMid = env->GetMethodID(audioTrackClass, "play", "()V");
    env->CallVoidMethod(audioTrackObject, audioTrackPlayMid);

    return audioTrackObject;
}

/*
    可以使用goto, 但是此处不用
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_z_p00_player_ZPlayer_nPlay(JNIEnv *env, jobject obj, jstring url) {

    const char *nurl = env->GetStringUTFChars(url, NULL);
    LOGE("(native)file path: %s", nurl);

    int ret = -1;
    AVFormatContext *pFmtCtx = NULL;

    if ((ret = avformat_open_input(&pFmtCtx, nurl, NULL, NULL)) != 0) {
        LOGE("error: avformat_open_input, %d, %s", ret, av_err2str(ret));
        return;
    }

    if ((ret = avformat_find_stream_info(pFmtCtx, NULL)) < 0) {
        LOGE("error: avformat_find_stream_info, %d, %s", ret, av_err2str(ret));
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
        return;
    }
    audioStreamIndex = ret;

    AVCodecParameters *pCodecPar = pFmtCtx->streams[audioStreamIndex]->codecpar;
    AVCodec *pCodec = avcodec_find_decoder(pCodecPar->codec_id);
    if (pCodec == NULL) {
        LOGE("error: avcodec_find_decoder");
        return;
    }

    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
    if (pCodecCtx == NULL) {
        LOGE("error: avcodec_alloc_context3");
        return;
    }

    if ((ret = avcodec_parameters_to_context(pCodecCtx, pCodecPar)) < 0) {
        LOGE("error: avcodec_parameters_to_context, %d, %s", ret, av_err2str(ret));
        return;
    }

    if ((ret = avcodec_open2(pCodecCtx, pCodec, NULL)) < 0) {
        LOGE("error: avcodec_open2, %d, %s", ret, av_err2str(ret));
        return;
    }

    LOGE("sample_rate: %d, channels: %d", pCodecCtx->sample_rate, pCodecCtx->channels);

    // 4. 使用AudioTrack播放pcm音频数据
    jobject audioTrackObject = createAudioObject(env);
    jclass audioTrackClass = env->GetObjectClass(audioTrackObject);
    jmethodID audioTrackWriteMid = env->GetMethodID(audioTrackClass, "write", "([BII)I");

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

    SwrContext *pSwrCtx = swr_alloc_set_opts(NULL,
            out_ch_layout, out_sample_fmt, out_sample_rate,
            in_ch_layout, in_sample_fmt, in_sample_rate,
            0, NULL);
    if (pSwrCtx == NULL) {
        LOGE("error: swr_alloc_set_opts");
        return;
    }

    if ((ret = swr_init(pSwrCtx)) < 0) {
        LOGE("error: swr_init");
        return;
    }

    // 5. 解决内存上涨问题
    int out_channels = av_get_channel_layout_nb_channels(out_ch_layout);
    int dataSize = av_samples_get_buffer_size(NULL, out_channels, pCodecCtx->frame_size, out_sample_fmt, 0);

    // AudioTrack播放音频的数据缓冲区
    jbyteArray pcmByteArray = env->NewByteArray(dataSize);
    jbyte *pcmData = env->GetByteArrayElements(pcmByteArray, NULL);

    // 重采样的数据缓冲区
    uint8_t *pResampleBuffer = (uint8_t *)malloc(dataSize);

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
                    env->ReleaseByteArrayElements(pcmByteArray, pcmData, JNI_COMMIT);
                    env->CallIntMethod(audioTrackObject, audioTrackWriteMid, pcmByteArray, 0, dataSize);
                }
            }
        }
        av_packet_unref(pPacket);
        av_frame_unref(pFrame);
    }
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);

    // 解决内存上涨问题
    env->ReleaseByteArrayElements(pcmByteArray, pcmData, 0);
    env->DeleteLocalRef(pcmByteArray);

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

    env->ReleaseStringUTFChars(url, nurl);
}