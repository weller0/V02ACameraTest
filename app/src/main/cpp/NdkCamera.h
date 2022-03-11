//
// Created by arena on 2022/3/4.
//

#ifndef CAMERA_NDKCAMERA_H
#define CAMERA_NDKCAMERA_H

#include <jni.h>
#include <set>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraMetadataTags.h>
#include <camera/NdkCameraMetadata.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>
#include "native_debug.h"

using namespace std;

typedef void (*OnImageAvailable)(const char *id, uint8_t *data, int size,
                                 int w, int h, int format, int64_t timestamp);

typedef struct _CameraState {
    char *cameraId;
    char *physicalCameraId;
    ACameraDevice *device;
    bool isLogicalMultiCamera;
    AImageReader *imageReader;
    OnImageAvailable callback;
    ACaptureRequest *captureRequest;
    ACaptureSessionOutputContainer *outputContainer;
    ACameraCaptureSession *captureSession;
    // logical camera
    ACaptureSessionOutput *sessionOutput;
    ACameraOutputTarget *outputTarget;

    _CameraState *copy(_CameraState *oldState) {
        _CameraState *newState = new _CameraState();
        newState->cameraId = oldState->cameraId;
        newState->device = oldState->device;
        newState->isLogicalMultiCamera = oldState->isLogicalMultiCamera;
        newState->callback = oldState->callback;
        newState->captureRequest = oldState->captureRequest;
        newState->outputContainer = oldState->outputContainer;
        //newState->imageReader = oldState->imageReader;
        return newState;
    }
} CameraState;

class NdkCamera {
public:
    NdkCamera();

    ~NdkCamera();

    int open(const char *cameraId, OnImageAvailable cb);

    int close();

    int read();

private:
    int read(CameraState *state);

    int createImageReader(CameraState *state);

    int prepareCamera(CameraState *state);

    int addCamera(CameraState *state);

    int startPreview(CameraState *state);

    int enumeratePhysicalCamera(CameraState *state, const char *const **physicalCameraIds,
                                size_t *physicalCameraIdCnt);

    ACameraDevice_StateCallbacks *getDeviceListener(CameraState *state);

    ACameraCaptureSession_stateCallbacks *getSessionListener(CameraState *state);

    AImageReader_ImageListener *getImageListener(CameraState *state);

    ACameraManager *pCameraManager;
    set<CameraState *> cameraStates;
};

#endif //CAMERA_NDKCAMERA_H
