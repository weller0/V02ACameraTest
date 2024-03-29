#include "NdkCamera.h"

#define PREVIEW_WIDTH       640
#define PREVIEW_HEIGHT      480

uint8_t *buffer = nullptr;

void onCameraErrorState(void *ctx, ACameraDevice *device, int error) {
    LOGD("onCameraErrorState error:%d, %p", error, device);
}

void onImageAvailable(void *ctx, AImageReader *reader) {
    auto *camera = static_cast<NdkCamera *>(ctx);
    camera->read(camera->findCameraState(reader));
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

uint64_t elapsedRealtimeNanos() {
    struct timespec ts{};
    if (clock_gettime(CLOCK_BOOTTIME, &ts) != 0) {
        return 0;
    }
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

NdkCamera::NdkCamera() {
    LOGD("[NdkCamera]+");
    pCameraManager = ACameraManager_create();
}

NdkCamera::~NdkCamera() {
    LOGD("[NdkCamera]-");
    close();
}

ACameraDevice_StateCallbacks *NdkCamera::getDeviceListener(NdkCamera *camera) {
    static ACameraDevice_StateCallbacks cameraDeviceListener = {
            .context = camera,
            .onDisconnected = nullptr,
            .onError = ::onCameraErrorState,
    };
    return &cameraDeviceListener;
}

AImageReader_ImageListener *NdkCamera::getImageListener(NdkCamera *camera) {
    static AImageReader_ImageListener imageListener = {
            .context = camera,
            .onImageAvailable = ::onImageAvailable,
    };
    return &imageListener;
}

ACameraCaptureSession_stateCallbacks *NdkCamera::getSessionListener(NdkCamera *camera) {
    static ACameraCaptureSession_stateCallbacks stateCallbacks = {
            .context = camera,
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
    state->physicalCameraId = nullptr;
    status = ACameraManager_openCamera(pCameraManager, cameraId, getDeviceListener(this),
                                       &state->device);
    if (status != ACAMERA_OK || state->device == nullptr) {
        LOGW("[NdkCamera]ACameraManager_openCamera failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]device=%p", state->device);

    prepareCamera(state);

    const char *const *physicalCameraIds;
    size_t physicalCameraIdCnt = 0;
    status = enumeratePhysicalCamera(state, &physicalCameraIds, &physicalCameraIdCnt);
    if (status != AMEDIA_OK) {
        LOGW("[NdkCamera]enumeratePhysicalCamera failed : %d", status);
        return status;
    }

    char *cid[physicalCameraIdCnt];
    for (size_t j = 0; j < physicalCameraIdCnt; j++) {
        cid[j] = strdup(physicalCameraIds[j]);
        LOGI("physical cam id : %s(%zu)", cid[j], strlen(cid[j]));
    }
    if (state->isLogicalMultiCamera) {
        status = createImageReader(state);
        if (status != AMEDIA_OK) {
            LOGW("[NdkCamera]createImageReader failed : %d", status);
            return status;
        }
        addCamera(state);
        for (int i = 0; i < physicalCameraIdCnt; i++) {
            CameraState *subState = state->copy(state);
            subState->physicalCameraId = cid[i];
            status = createImageReader(subState);
            if (status != AMEDIA_OK) {
                LOGW("[NdkCamera]createImageReader failed : %d", status);
                return status;
            }
            addCamera(subState);
            cameraStates.insert(subState);
        }
    } else {
        status = createImageReader(state);
        if (status != AMEDIA_OK) {
            LOGW("[NdkCamera]createImageReader failed : %d", status);
            return status;
        }
        addCamera(state);
    }
    cameraStates.insert(state);
    LOGD("cameraStates.size=%zu", cameraStates.size());
    startPreview(state);
    return AMEDIA_OK;
}

int NdkCamera::enumeratePhysicalCamera(CameraState *state, const char *const **physicalCameraIds,
                                       size_t *physicalCameraIdCnt) {
    ACameraMetadata *staticMetadata;
    state->isLogicalMultiCamera = false;
    camera_status_t status = ACameraManager_getCameraCharacteristics(
            pCameraManager, state->cameraId, &staticMetadata);
    if (status != ACAMERA_OK || staticMetadata == nullptr) {
        LOGW("[NdkCamera]getCameraCharacteristics failed : %d", status);
        return status;
    }
    bool supportLogicalCamera = false;
    ACameraMetadata_const_entry entry;
    ACameraMetadata_getConstEntry(
            staticMetadata, ACAMERA_REQUEST_AVAILABLE_CAPABILITIES, &entry);
    for (uint32_t j = 0; j < entry.count; j++) {
        if (entry.data.u8[j] == ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA) {
            supportLogicalCamera = true;
            break;
        }
    }
    if (supportLogicalCamera) {
        bool isLogicalCamera = ACameraMetadata_isLogicalMultiCamera(staticMetadata,
                                                                    physicalCameraIdCnt,
                                                                    physicalCameraIds);
        LOGI("isLogicalCamera: %d, physicalCameraIdCnt: %zu", isLogicalCamera,
             *physicalCameraIdCnt);
        state->isLogicalMultiCamera = isLogicalCamera && (*physicalCameraIdCnt) > 0;
        for (size_t j = 0; j < (*physicalCameraIdCnt); j++) {
            LOGI("physical cam id : %s", (*physicalCameraIds)[j]);
        }
    }
    ACameraMetadata_free(staticMetadata);
    return ACAMERA_OK;
}

int NdkCamera::prepareCamera(CameraState *state) {
    if (!state->device) {
        return ACAMERA_ERROR_CAMERA_DEVICE;
    }
    LOGD("[NdkCamera]>>>>>>>>>> prepareCamera <<<<<<<<<<");
    int status = ACaptureSessionOutputContainer_create(&state->outputContainer);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACaptureSessionOutputContainer_create failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]outputContainer=%p", state->outputContainer);

    status = ACameraDevice_createCaptureRequest(state->device, TEMPLATE_PREVIEW,
                                                &state->captureRequest);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACameraDevice_createCaptureRequest failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]request=%p", state->captureRequest);
    return ACAMERA_OK;
}

int NdkCamera::addCamera(CameraState *state) {
    LOGD("[NdkCamera]>>>>>>>>>> addCamera cam id : %s <<<<<<<<<<", state->physicalCameraId);
    // prepare surface/window
    ANativeWindow *outputNativeWindow;
    LOGD("[NdkCamera]imageReader=%p", state->imageReader);
    int status = AImageReader_getWindow(state->imageReader, &outputNativeWindow);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]AImageReader_getWindow failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]outputNativeWindow=%p", outputNativeWindow);
    ANativeWindow_acquire(outputNativeWindow);

    // 不能使用ACaptureSessionPhysicalOutput_create
    status = ACaptureSessionOutput_create(outputNativeWindow, &state->sessionOutput);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACaptureSessionOutput_create failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]sessionOutput=%p", state->sessionOutput);
    status = ACaptureSessionOutputContainer_add(state->outputContainer, state->sessionOutput);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACaptureSessionOutputContainer_add failed : %d", status);
        return status;
    }

    status = ACameraOutputTarget_create(outputNativeWindow, &state->outputTarget);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACameraOutputTarget_create failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]outputTarget=%p", state->outputTarget);

    status = ACaptureRequest_addTarget(state->captureRequest, state->outputTarget);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACaptureRequest_addTarget failed : %d", status);
        return status;
    }
    return ACAMERA_OK;
}

int NdkCamera::startPreview(CameraState *state) {
    LOGD("[NdkCamera]>>>>>>>>>> startPreview <<<<<<<<<<");
    int status = ACameraDevice_createCaptureSession(state->device,
                                                    state->outputContainer,
                                                    getSessionListener(this),
                                                    &state->captureSession);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACameraDevice_createCaptureSession failed : %d", status);
        return status;
    }
    LOGD("[NdkCamera]captureSession=%p", state->captureSession);
    // config
    const int32_t range[] = {60, 60};
    ACaptureRequest_setEntry_i32(
            state->captureRequest,
            ACAMERA_CONTROL_AE_TARGET_FPS_RANGE,
            2,
            range);

    const int64_t frame_duration[] = {16000000};
    ACaptureRequest_setEntry_i64(
            state->captureRequest,
            ACAMERA_SENSOR_FRAME_DURATION,
            1,
            frame_duration);

    /*const uint8_t ae_mode[] = {0};
    ACaptureRequest_setEntry_u8(
            state->captureRequest,
            ACAMERA_CONTROL_AE_MODE,
            1,
            ae_mode);

    const int64_t exposure[] = {2000000};
    ACaptureRequest_setEntry_i64(
            state->captureRequest,
            ACAMERA_SENSOR_EXPOSURE_TIME,
            1,
            exposure);

    const int32_t sensitivity[] = {1000};
    ACaptureRequest_setEntry_i32(
            state->captureRequest,
            ACAMERA_SENSOR_SENSITIVITY,
            1,
            sensitivity);*/
    int seqId;
    // 不能使用ACameraCaptureSession_logicalCamera_setRepeatingRequest
    status = ACameraCaptureSession_setRepeatingRequest(state->captureSession, nullptr, 1,
                                                       &state->captureRequest, &seqId);
    if (status != ACAMERA_OK) {
        LOGW("[NdkCamera]ACameraCaptureSession_setRepeatingRequest failed : %d", status);
        return status;
    }
    return 0;
}

int NdkCamera::read() {
    if (!cameraStates.empty()) {
        for (auto cameraState : cameraStates) {
            read(cameraState);
        }
    }
    return AMEDIA_OK;
}

int NdkCamera::read(CameraState *state) {
    if (state && state->imageReader) {
        AImage *image;
        int status = AImageReader_acquireNextImage(state->imageReader, &image);
        if (status != AMEDIA_OK) {
            LOGW("read fail [%s, %s]", state->cameraId, state->physicalCameraId);
            return AMEDIA_ERROR_UNKNOWN;
        }
        int32_t planeNum = 0;
        status = AImage_getNumberOfPlanes(image, &planeNum);
        if (status != AMEDIA_OK || planeNum <= 0) {
            return AMEDIA_ERROR_UNKNOWN;
        }
        if (buffer == nullptr) {
            buffer = (uint8_t *) malloc(PREVIEW_WIDTH * PREVIEW_HEIGHT);
        }
        int32_t format = 0, size = 0;
        int64_t timestamp = 0;
        AImage_getFormat(image, &format);
        AImage_getTimestamp(image, &timestamp);
        AImage_getPlaneData(image, 0, &buffer, &size);

        LOGD("read frame -> %s timestamp:%ld, frame interval:%ld ms, curr time diff:%ld ms",
             state->physicalCameraId ? state->physicalCameraId : state->cameraId,
             timestamp,
             (timestamp - state->lastFrameTime) / 1000000,
             (elapsedRealtimeNanos() - timestamp) / 1000000);
        state->lastFrameTime = timestamp;

        state->callback(state->physicalCameraId ? state->physicalCameraId : state->cameraId,
                        buffer, size, PREVIEW_WIDTH, PREVIEW_HEIGHT, format, timestamp);
        AImage_delete(image);
    }
    return AMEDIA_OK;
}

CameraState *NdkCamera::findCameraState(AImageReader *reader) {
    if (!cameraStates.empty()) {
        for (auto cameraState : cameraStates) {
            if (cameraState->imageReader == reader) return cameraState;
        }
    }
    return nullptr;
}

int NdkCamera::close() {
    LOGD("[NdkCamera]close");
    for (CameraState *state : cameraStates) {
        if (state->outputTarget) ACameraOutputTarget_free(state->outputTarget);
        if (state->captureRequest) ACaptureRequest_free(state->captureRequest);
        if (state->captureSession) ACameraCaptureSession_close(state->captureSession);
        if (state->sessionOutput) ACaptureSessionOutput_free(state->sessionOutput);
        if (state->outputContainer) ACaptureSessionOutputContainer_free(state->outputContainer);
        if (state->device) ACameraDevice_close(state->device);
    }
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
    status = AImageReader_setImageListener(state->imageReader, getImageListener(this));
    if (status != AMEDIA_OK) {
        LOGW("[NdkCamera]AImageReader_setImageListener failed : %d", status);
        return status;
    }
    return AMEDIA_OK;
}