//
// Created by lin on 2021/11/13.
//

#ifndef P00_ZCONSTANTS_H
#define P00_ZCONSTANTS_H


// --- log
#include <android/log.h>

#ifndef LOG_TAG
#define LOG_TAG "JNI_TAG"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif
// --- log


// -- error
#define ERR_AVFORMAT_OPEN_INPUT 10000
#define ERR_AVFORMAT_FIND_STREAM_INFO 10001
#define ERR_AV_FIND_BEST_STREAM 10002
#define ERR_AVCODEC_FIND_DECODER 10003
#define ERR_AVCODEC_ALLOC_CONTEXT3 10004
#define ERR_AVCODEC_PARAMETERS_TO_CONTEXT 10005
#define ERR_AVCODEC_OPEN2 10006
#define ERR_SWR_ALLOC_SET_OPTS 10007
#define ERR_SWR_INIT 10008
// -- error

#endif //P00_ZCONSTANTS_H
