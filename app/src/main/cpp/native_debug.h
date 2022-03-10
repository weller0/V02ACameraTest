#include <android/log.h>

#define LOG_TAG "S_NDK_CAMERA"
#define LOGI(format, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[%s:%s:%d]" \
format , strrchr(__FILE__, '/'), __func__, __LINE__, ##__VA_ARGS__)
#define LOGD(format, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[%s:%s:%d]" \
format , strrchr(__FILE__, '/'), __func__, __LINE__, ##__VA_ARGS__)
#define LOGW(format, ...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "[%s:%s:%d]" \
format , strrchr(__FILE__, '/'), __func__, __LINE__, ##__VA_ARGS__)
#define LOGE(format, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[%s:%s:%d]" \
format , strrchr(__FILE__, '/'), __func__, __LINE__, ##__VA_ARGS__)
#define ASSERT(cond, fmt, ...)                                \
  if (!(cond)) {                                              \
    __android_log_assert(#cond, LOG_TAG, fmt, ##__VA_ARGS__); \
  }
