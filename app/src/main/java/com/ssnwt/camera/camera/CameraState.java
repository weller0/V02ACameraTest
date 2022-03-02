package com.ssnwt.camera.camera;

import android.hardware.camera2.CameraDevice;
import android.media.ImageReader;

public class CameraState {
    private String mCameraId;
    private CameraDevice mCameraDevice;
    private ImageReader mRawImageReader;
    private OnFrameAvailableListener mOnFrameAvailableListener;

    public CameraState(String cameraId, ImageReader rawImageReader,
        OnFrameAvailableListener onFrameAvailableListener) {
        mCameraId = cameraId;
        mRawImageReader = rawImageReader;
        mOnFrameAvailableListener = onFrameAvailableListener;
    }

    public String getCameraId() {
        return mCameraId;
    }

    public void setCameraId(String cameraId) {
        mCameraId = cameraId;
    }

    public CameraDevice getCameraDevice() {
        return mCameraDevice;
    }

    public void setCameraDevice(CameraDevice cameraDevice) {
        mCameraDevice = cameraDevice;
    }

    public ImageReader getRawImageReader() {
        return mRawImageReader;
    }

    public void setRawImageReader(ImageReader rawImageReader) {
        mRawImageReader = rawImageReader;
    }

    public OnFrameAvailableListener getOnFrameAvailableListener() {
        return mOnFrameAvailableListener;
    }

    public void setOnFrameAvailableListener(
        OnFrameAvailableListener onFrameAvailableListener) {
        mOnFrameAvailableListener = onFrameAvailableListener;
    }

    public void close() {
        if (mRawImageReader != null) {
            mRawImageReader.close();
            mRawImageReader = null;
        }
        if (mCameraDevice != null) {
            mCameraDevice.close();
            mCameraDevice = null;
        }
        mOnFrameAvailableListener = null;
        mCameraId = null;
    }
}
