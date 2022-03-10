#include "NdkCamera.h"

#define PREVIEW_WIDTH       640
#define PREVIEW_HEIGHT      480

uint8_t *buffer = nullptr;

void onCameraErrorState(void *ctx, ACameraDevice *device, int error) {
    LOGD("onCameraErrorState error:%d, %p", error, device);
}

void onImageAvailable(void *ctx, AImageReader *reader) {
    auto *state = static_cast<CameraState *>(ctx);
    AImage *image;
    int status = AImageReader_acquireNextImage(reader, &image);
    if (status != AMEDIA_OK) {
        return;
    }
    int32_t planeNum = 0;
    status = AImage_getNumberOfPlanes(image, &planeNum);
    if (status != AMEDIA_OK || planeNum <= 0) {
        return;
    }
    if (buffer == nullptr) {
        buffer = (uint8_t *) malloc(PREVIEW_WIDTH * PREVIEW_HEIGHT);
    }
    int32_t format = 0, size = 0;
    int64_t timestamp = 0;
    AImage_getFormat(image, &format);
    AImage_getTimestamp(image, &timestamp);
    AImage_getPlaneData(image, 0, &buffer, &size);
    if (state != nullptr) {
        state->callback(state->cameraId, buffer, size,
                        PREVIEW_WIDTH, PREVIEW_HEIGHT, format, timestamp);
    }
    AImage_delete(image);
}

void onSessionClosed(void *ctx, ACameraCaptureSession *session) {
    LOGD("onSessionClosed");
}

void onSessionReady(void *ctx, ACameraCaptureSession *session) {
    LOGD("onSessionReady");
    ACameraCaptureSession_setRepeatingRequest(session, nullptr, 1, nullptr, nullptr);
}

void onSessionActive(void *ctx, ACameraCaptureSession *session) {
    LOGD("onSessionActive");
}

NdkCamera::NdkCamera() {
    LOGD("[NdkCamera]+");
    pCameraManager = ACameraManager_create();
}

NdkCamera::~NdkCamera() {
    LOGD("[NdkCamera]-");
}

ACameraDevice_StateCallbacks *NdkCamera::getDeviceListener(CameraState *state) {
    static ACameraDevice_StateCallbacks cameraDeviceListener = {
            .context = state,
            .onDisconnected = nullptr,
            .onError = ::onCameraErrorState,
    };
    return &cameraDeviceListener;
}

AImageReader_ImageListener *NdkCamera::getImageListener(CameraState *state) {
    static AImageReader_ImageListener imageListener = {
            .context = state,
            .onImageAvailable = ::onImageAvailable,
    };
    return &imageListener;
}

ACameraCaptureSession_stateCallbacks *NdkCamera::getSessionListener(CameraState *state) {
    static ACameraCaptureSession_stateCallbacks stateCallbacks = {
            .context = state,
            .onClosed = ::onSessionClosed,
            .onReady = ::onSessionReady,
            .onActive = ::onSessionActive,
    };
    return &stateCallbacks;
}

int NdkCamera::open(const char *cameraId, OnImageAvailable cb) {
    LOGD("[NdkCamera]open %s", cameraId);
    int status;
    auto *state = new CameraState();
    state->callback = cb;
    state->cameraId = strdup(cameraId);
    LOGD("onImageAvailable %p", &state);
    status = createImageReader(state);
    if (status != AMEDIA_OK) {
        LOGW("[NdkCamera]createImageReader failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]imageReader=%p", state->imageReader);
    status = ACameraManager_openCamera(pCameraManager, cameraId, getDeviceListener(state),
                                       &state->device);
    if (status != ACAMERA_OK || state->device == nullptr) {
        LOGW("[NdkCamera]ACameraManager_openCamera failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]device=%p", state->device);
    cameraStates.push(state);
    startPreview(state);
    return 0;
}

int NdkCamera::startPreview(CameraState *state) {
    if (!state->device) {
        return ACAMERA_ERROR_CAMERA_DEVICE;
    }
    LOGD("[NdkCamera]startPreview");
    int status;
    // prepare surface/window
    ANativeWindow *outputNativeWindow;
    LOGD("[NdkCamera]imageReader=%p", state->imageReader);
    status = AImageReader_getWindow(state->imageReader, &outputNativeWindow);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]AImageReader_getWindow failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]outputNativeWindow=%p", outputNativeWindow);
    //ANativeWindow_acquire(outputNativeWindow);

    ACaptureSessionOutputContainer *outputContainer;
    status = ACaptureSessionOutputContainer_create(&outputContainer);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACaptureSessionOutputContainer_create failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]outputContainer=%p", outputContainer);

    ACaptureSessionOutput *sessionOutput;
    status = ACaptureSessionOutput_create(outputNativeWindow, &sessionOutput);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACaptureSessionOutput_create failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]sessionOutput=%p", sessionOutput);
    status = ACaptureSessionOutputContainer_add(outputContainer, sessionOutput);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACaptureSessionOutputContainer_add failed : %d", status);
        return status;
    }

    ACameraCaptureSession *captureSession;
    status = ACameraDevice_createCaptureSession(state->device, outputContainer,
                                                getSessionListener(state), &captureSession);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACameraDevice_createCaptureSession failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]captureSession=%p", captureSession);

    ACaptureRequest *request;
    status = ACameraDevice_createCaptureRequest(state->device, TEMPLATE_PREVIEW, &request);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACameraDevice_createCaptureRequest failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]request=%p", request);

    ACameraOutputTarget *target;
    status = ACameraOutputTarget_create(outputNativeWindow, &target);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACameraOutputTarget_create failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]target=%p", target);

    status = ACaptureRequest_addTarget(request, target);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACaptureRequest_addTarget failed : %d", status);
        return status;
    }
    int seqId;
    ACameraCaptureSession_setRepeatingRequest(captureSession, nullptr, 1, &request, &seqId);
    return 0;
}

int NdkCamera::close(const char *cameraId) {
    LOGD("[NdkCamera]close %s", cameraId);
    return 0;
}

int NdkCamera::createImageReader(CameraState *state) {
    media_status_t status;
    status = AImageReader_new(PREVIEW_WIDTH, PREVIEW_HEIGHT, AIMAGE_FORMAT_RAW_PRIVATE,
                              5, &state->imageReader);
    if (status != AMEDIA_OK) {
        LOGW("[NdkCamera]AImageReader_new failed : %d", status);
        return status;
    }
    status = AImageReader_setImageListener(state->imageReader, getImageListener(state));
    if (status != AMEDIA_OK) {
        LOGW("[NdkCamera]AImageReader_setImageListener failed : %d", status);
        return status;
    }
    return AMEDIA_OK;
}