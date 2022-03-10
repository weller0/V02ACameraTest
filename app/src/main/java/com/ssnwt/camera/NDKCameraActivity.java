package com.ssnwt.camera;

import android.content.Context;
import android.graphics.Bitmap;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.widget.ImageView;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import java.nio.ByteBuffer;
import java.util.Arrays;

public class NDKCameraActivity extends AppCompatActivity {
    private static final String TAG = "SSNWT_" + NDKCameraActivity.class.getSimpleName();
    private ImageView mCameraView1, mCameraView2, mCameraView3, mCameraView4;

    @Override protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_camera);
        catCamera2();
        mCameraView1 = findViewById(R.id.camera_view_1);
        mCameraView2 = findViewById(R.id.camera_view_2);
        mCameraView3 = findViewById(R.id.camera_view_3);
        mCameraView4 = findViewById(R.id.camera_view_4);
        openCamera("0");
    }

    @Override protected void onDestroy() {
        super.onDestroy();
        closeCamera("0");
    }

    private void catCamera2() {
        CameraManager cameraManager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        try {
            for (String cameraId : cameraManager.getCameraIdList()) {
                Log.d(TAG, ">>>>>>>>>>>>> cameraId=" + cameraId);
                CameraCharacteristics characteristics =
                    cameraManager.getCameraCharacteristics(cameraId);
                StreamConfigurationMap map =
                    characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                int[] formats = map.getOutputFormats();
                Log.d(TAG, "formats=" + Arrays.toString(formats));
                for (int format : formats) {
                    Log.d(TAG, format + ":" + Arrays.toString(map.getOutputSizes(format)));
                }
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    // Called by jni
    ByteBuffer buffer = null;

    private void onImageAvailable(String cameraId, byte[] data, int size,
        int w, int h, int format, long timestamp) {
        if (data == null) return;
        Log.d(TAG, "[" + cameraId
            + "] size:" + size
            + " (" + w + ", " + h
            + "), format:" + format
            + ", timestamp:" + timestamp
            + ", delta:" + (SystemClock.elapsedRealtimeNanos() - timestamp));
        if (buffer == null) buffer = ByteBuffer.allocate(w * h);
        runOnUiThread(() -> {
            buffer.clear();
            buffer.put(data);
            buffer.position(0);
            Bitmap bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ALPHA_8);
            bmp.copyPixelsFromBuffer(buffer);
            if ("0".equals(cameraId)) {
                mCameraView1.setImageBitmap(bmp);
            } else if ("1".equals(cameraId)) {
                mCameraView2.setImageBitmap(bmp);
            } else if ("2".equals(cameraId)) {
                mCameraView3.setImageBitmap(bmp);
            } else if ("3".equals(cameraId)) {
                mCameraView4.setImageBitmap(bmp);
            }
        });
    }

    private native void openCamera(String id);

    private native void closeCamera(String id);

    static {
        System.loadLibrary("ndk_camera");
    }
}
