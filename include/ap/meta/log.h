#pragma once

#include <android/log.h>

#define LOGI(str, ...)                                                         \
    __android_log_print(ANDROID_LOG_INFO, "shasha_ap", str, ##__VA_ARGS__)
#define LOGD(str, ...)                                                         \
    __android_log_print(ANDROID_LOG_DEBUG, "shasha_ap", str, ##__VA_ARGS__)
#define LOGW(str, ...)                                                         \
    __android_log_print(ANDROID_LOG_WARN, "shasha_ap", str, ##__VA_ARGS__)
#define LOGE(str, ...)                                                         \
    __android_log_print(ANDROID_LOG_ERROR, "shasha_ap", str, ##__VA_ARGS__)