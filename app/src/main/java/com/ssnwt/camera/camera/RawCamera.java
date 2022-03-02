package com.ssnwt.camera.camera;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.media.Image;
import android.media.ImageReader;
import android.util.Log;
import android.view.Surface;
import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class RawCamera {
    private static final String TAG = "SSNWT_" + RawCamera.class.getSimpleName();
    private List<CameraState> mCameraStates;
    private ImageReader.OnImageAvailableListener mOnRawImageAvailableListener = reader -> {
        Image image = reader.acquireNextImage();
        Image.Plane[] planes = image.getPlanes();
        if (planes != null && planes.length > 0) {
            CameraState state = getCameraState(reader);
            if (state != null) {
                state.getOnFrameAvailableListener().onFrameAvailable(state.getCameraId(),
                    planes[0].getBuffer(), image.getWidth(), image.getHeight(),
                    image.getFormat(), image.getTimestamp());
            }
        }
        image.close();
    };

    public RawCamera() {
        mCameraStates = new ArrayList<>();
    }

    public void open(Context context, String cameraId, OnFrameAvailableListener listener) {
        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.CAMERA)
            != PackageManager.PERMISSION_GRANTED) {
            Log.w(TAG, "No camera permission.");
            return;
        }
        CameraManager cameraManager =
            (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
        try {
            Log.d(TAG, "openCamera=" + cameraId);
            final ImageReader reader = prepareRawCamera(640, 480);
            final CameraState state = new CameraState(cameraId, reader, listener);
            mCameraStates.add(state);
            cameraManager.openCamera(cameraId, new CameraDevice.StateCallback() {
                @Override public void onOpened(@NonNull CameraDevice camera) {
                    Log.d(TAG, "onOpened=" + camera);
                    state.setCameraDevice(camera);
                    try {
                        startPreview(state, Collections.singletonList(reader.getSurface()));
                    } catch (CameraAccessException e) {
                        e.printStackTrace();
                    }
                }

                @Override public void onDisconnected(@NonNull CameraDevice camera) {

                }

                @Override public void onError(@NonNull CameraDevice camera, int error) {

                }
            }, null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    public void close(String cameraId) {
        CameraState state = getCameraState(cameraId);
        if (state != null) {
            state.close();
        }
    }

    private void startPreview(CameraState state, List<Surface> surfaces)
        throws CameraAccessException {
        Log.d(TAG, "startPreview");
        CameraDevice device = state.getCameraDevice();
        if (device == null) {
            throw new NullPointerException("startPreview:getCameraDevice:Do not found camera.");
        }
        CaptureRequest.Builder builder = device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
        builder.set(CaptureRequest.FLASH_MODE, CaptureRequest.FLASH_MODE_OFF);
        for (Surface surface : surfaces) {
            builder.addTarget(surface);
        }
        device.createCaptureSession(surfaces, new CameraCaptureSession.StateCallback() {
            @Override public void onConfigured(@NonNull CameraCaptureSession session) {
                Log.d(TAG, "onConfigured");
                try {
                    session.setRepeatingRequest(builder.build(), null, null);
                } catch (CameraAccessException e) {
                    e.printStackTrace();
                }
            }

            @Override public void onConfigureFailed(@NonNull CameraCaptureSession session) {

            }
        }, null);
    }

    private ImageReader prepareRawCamera(int w, int h) {
        ImageReader reader = ImageReader.newInstance(w, h, ImageFormat.RAW_PRIVATE, 5);
        reader.setOnImageAvailableListener(mOnRawImageAvailableListener, null);
        return reader;
    }

    private CameraState getCameraState(String cameraId) {
        if (cameraId != null) {
            for (CameraState state : mCameraStates) {
                if (cameraId.equals(state.getCameraId())) {
                    return state;
                }
            }
        }
        return null;
    }

    private CameraState getCameraState(ImageReader reader) {
        if (reader != null) {
            for (CameraState state : mCameraStates) {
                if (reader == state.getRawImageReader()) {
                    return state;
                }
            }
        }
        return null;
    }
}
