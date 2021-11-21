#include <jni.h>
#include <string>

// target_link_libraries -> android
#include <android/native_window.h>
#include <android/native_window_jni.h>

extern "C" {
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

#include "ZConstants.h"
#include "ZJniCall.h"
#include "ZFFmpeg.h"

ZJniCall *zJniCall = NULL;
ZFFmpeg *zFFmpeg = NULL;
JavaVM *javaVM = NULL;

/*
    重写so共享库被加载时, 自动调用的一个方法

    TODO: 了解动态注册
 */
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGE("shared library loaded!");
    javaVM = vm;
    JNIEnv *env = NULL;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("error: failed to GetEnv, [%s:%d]", __FILE__, __LINE__);
        return JNI_ERR;
    }
    return JNI_VERSION_1_4;
}

/*
    可以使用goto, 但是此处不用
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_z_p00_player_ZPlayer_nPlay(JNIEnv *env, jobject obj) {
    if (zFFmpeg) {
        zFFmpeg->play();
    }
}

/*
    网络流相关 - 注意添加权限呀
    avformat_network_init()
    avformat_network_deinit()
*/
extern "C"
JNIEXPORT void JNICALL
Java_com_z_p00_player_ZPlayer_nPrepareAsync(JNIEnv *env, jobject obj, jstring url) {
    if (zFFmpeg == NULL) {
        const char *nurl = env->GetStringUTFChars(url, NULL);
        LOGE("(native)file path: %s", nurl);
        zJniCall = new ZJniCall(javaVM, env, obj);
        zFFmpeg = new ZFFmpeg(zJniCall, nurl);
        zFFmpeg->prepareAsync();
        env->ReleaseStringUTFChars(url, nurl);
    }
}

/*
    nStop

    if (zFFmpeg) {
        delete zFFmpeg;
        zFFmpeg = NULL;
    }

    if (zJniCall) {
        delete zJniCall;
        zJniCall = NULL;
    }
 */

extern "C"
JNIEXPORT void JNICALL
Java_com_z_p00_MainActivity_decodeVideo
(JNIEnv *env, jobject obj, jobject surface, jstring url) {

    const char *nurl = env->GetStringUTFChars(url, NULL);
    LOGE("(native)file path: %s", nurl);

    int ret = -1;
    AVFormatContext *pFmtCtx = NULL;

    if ((ret = avformat_open_input(&pFmtCtx, nurl, NULL, NULL)) < 0) {
        LOGE("error: avformat_open_input, %d, %s", ret, av_err2str(ret));
        return;
    }

    if ((ret = avformat_find_stream_info(pFmtCtx, NULL)) < 0) {
        LOGE("error: avformat_find_stream_info, %d, %s", ret, av_err2str(ret));
        return;
    }

    AVDictionaryEntry *entry = NULL;
    while ((entry = av_dict_get(pFmtCtx->metadata, "", entry, AV_DICT_IGNORE_SUFFIX)) != NULL) {
        LOGE("%s : %s", entry->key, entry->value);
    }

    ret = av_find_best_stream(pFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0) {
        LOGE("error: av_find_best_stream, %d, %s", ret, av_err2str(ret));
        return;
    }
    int videoStreamIndex = ret;

    AVCodecParameters *pCodecPar = pFmtCtx->streams[videoStreamIndex]->codecpar;
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

    LOGE("width: %d, height: %d", pCodecCtx->width, pCodecCtx->height);

    // 使用SurfaceView播放视频数据
    ANativeWindow *pNativeWindow = ANativeWindow_fromSurface(env, surface);

    // 设置窗口的宽，高，像素格式
    ANativeWindow_setBuffersGeometry(pNativeWindow, pCodecCtx->width, pCodecCtx->height, WINDOW_FORMAT_RGBA_8888);

    // SurfaceView的播放缓冲
    ANativeWindow_Buffer outputBuffer;

    // 对视频进行缩放处理
    SwsContext *pSwsCtx = sws_getContext(
            pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,        // src
            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGBA, // dst
            SWS_BILINEAR, NULL, NULL, NULL);

    // 转换后的格式所对应的内存大小
    int frameSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height, 1);
    uint8_t *pFrameBuffer = (uint8_t *)malloc(frameSize);
    // 为pFrameRGB挂上buffer, 这个buffer是用于存缓冲数据
    AVFrame *pFrameRGBA = av_frame_alloc();
    av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize,    // dst, TODO: 这个函数到底做了什么?
                         pFrameBuffer,                               // src
                         AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height, 1);

    int frameIndex = 0;
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();
    while (av_read_frame(pFmtCtx, pPacket) == 0) {
        if (pPacket->stream_index == videoStreamIndex) {
            if (avcodec_send_packet(pCodecCtx, pPacket) == 0) {
                if (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                    ++frameIndex;
                    LOGE("frame: %d", frameIndex);

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
        }
        av_packet_unref(pPacket);
        av_frame_unref(pFrame);
    }
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);

    if (pFrameRGBA) {
        av_frame_free(&pFrameRGBA);
    }

    if (pFrameBuffer) {
        free(pFrameBuffer);
        pFrameBuffer = NULL;
    }

    if (pSwsCtx) {
        sws_freeContext(pSwsCtx);
        pSwsCtx = NULL;
    }

    if (pCodecCtx) {
        avcodec_free_context(&pCodecCtx);
    }

    if (pFmtCtx) {
        avformat_close_input(&pFmtCtx);
    }

    env->ReleaseStringUTFChars(url, nurl);
}