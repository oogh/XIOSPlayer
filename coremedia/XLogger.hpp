//
// Created by Oogh on 2020/3/19.
//

#ifndef XLOGGER_HPP
#define XLOGGER_HPP

#include "XPlatform.hpp"

#define TAG "XMedia"

#if PLATFORM_ANDROID
#include <android/log.h>
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#elif PLATFORM_IOS || PLATFORM_MAC
#include <cstdio>
#define LOGD(...) printf(__VA_ARGS__)
#define LOGW(...) printf(__VA_ARGS__)
#define LOGI(...) printf(__VA_ARGS__)
#define LOGE(...) fprintf(stderr, __VA_ARGS__)
#endif

#endif //XLOGGER_HPP
