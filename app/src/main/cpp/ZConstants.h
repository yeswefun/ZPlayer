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


#endif //P00_ZCONSTANTS_H
