#include <jni.h>
#include "NdkCamera.h"

NdkCamera *pCamera = nullptr;
jmethodID mtdOnImageAvailable = nullptr;
jobject objActivity = nullptr;
JavaVM *mVm = nullptr;
extern "C" {
JNIEnv *getJNIEnv() {
    JNIEnv *env = NULL;
    if (mVm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        int status = mVm->AttachCurrentThread(&env, 0);
        if (status < 0) {
            return NULL;
        }
    }
    return env;
}
void onImageAvailable(const char *id, uint8_t *data, int size,
                      int w, int h, int format, int64_t timestamp) {
    JNIEnv *env = getJNIEnv();
    jstring sId = env->NewStringUTF(id);
    jbyteArray bData = env->NewByteArray(size);
    env->SetByteArrayRegion(bData, 0, size, (jbyte *) data);

    env->CallVoidMethod(objActivity, mtdOnImageAvailable,
                        sId, bData, size, w, h, format, timestamp);

//    env->ReleaseByteArrayElements(bData, (jbyte *) data, JNI_FALSE);
    env->DeleteLocalRef(bData);
    env->DeleteLocalRef(sId);
}
JNIEXPORT void JNICALL
Java_com_ssnwt_camera_NDKCameraActivity_openCamera(JNIEnv *env, jobject thiz, jstring id) {
    // TODO: implement openCamera()
    if (mtdOnImageAvailable == nullptr) {
        env->GetJavaVM(&mVm);
        objActivity = env->NewGlobalRef(thiz);
        jclass Activity = env->GetObjectClass(thiz);
        mtdOnImageAvailable = env->GetMethodID(Activity, "onImageAvailable",
                                               "(Ljava/lang/String;[BIIIIJ)V");
        env->DeleteLocalRef(Activity);
    }
    if (!pCamera) pCamera = new NdkCamera();
    env->GetStringChars(id, JNI_FALSE);
    pCamera->open((char *) env->GetStringChars(id, JNI_FALSE), onImageAvailable);
}

JNIEXPORT void JNICALL
Java_com_ssnwt_camera_NDKCameraActivity_closeCamera(JNIEnv *env, jobject thiz, jstring id) {
    // TODO: implement closeCamera()
    pCamera->close((char *) env->GetStringChars(id, JNI_FALSE));
}
}