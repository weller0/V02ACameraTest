//
// Created by arena on 2022/3/4.
//

#ifndef CAMERA_NDKCAMERA_H
#define CAMERA_NDKCAMERA_H

#include <jni.h>
#include <queue>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraMetadataTags.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>
#include "native_debug.h"

using namespace std;

typedef void (*OnImageAvailable)(const char *id, uint8_t *data, int size,
                                 int w, int h, int format, int64_t timestamp);

typedef struct _CameraState {
    char *cameraId;
    ACameraDevice *device;
    AImageReader *imageReader;
    OnImageAvailable callback;
} CameraState;

class NdkCamera {
public:
    NdkCamera();

    ~NdkCamera();

    int open(const char *cameraId, OnImageAvailable cb);

    int close(const char *cameraId);

private:
    int createImageReader(CameraState *state);

    int startPreview(CameraState *state);

    ACameraDevice_StateCallbacks *getDeviceListener(CameraState *state);

    ACameraCaptureSession_stateCallbacks *getSessionListener(CameraState *state);

    AImageReader_ImageListener *getImageListener(CameraState *state);

    ACameraManager *pCameraManager;
    queue<CameraState *> cameraStates;
};

#endif //CAMERA_NDKCAMERA_H
