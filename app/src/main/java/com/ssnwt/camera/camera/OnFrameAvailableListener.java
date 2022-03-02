package com.ssnwt.camera.camera;

import java.nio.ByteBuffer;

public interface OnFrameAvailableListener {
    void onFrameAvailable(String cameraId, ByteBuffer buffer, int w, int h, int format,
        long timestamp);
}
