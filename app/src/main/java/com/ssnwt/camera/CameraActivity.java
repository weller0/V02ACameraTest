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
import com.ssnwt.camera.camera.OnFrameAvailableListener;
import com.ssnwt.camera.camera.RawCamera;
import java.nio.ByteBuffer;
import java.util.Arrays;

public class CameraActivity extends AppCompatActivity implements OnFrameAvailableListener {
    private static final String TAG = "SSNWT_" + CameraActivity.class.getSimpleName();
    private RawCamera mRawCamera;
    private ImageView mCameraView1, mCameraView2, mCameraView3, mCameraView4;

    @Override protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_camera);
        mCameraView1 = findViewById(R.id.camera_view_1);
        mCameraView2 = findViewById(R.id.camera_view_2);
        mCameraView3 = findViewById(R.id.camera_view_3);
        mCameraView4 = findViewById(R.id.camera_view_4);
        mRawCamera = new RawCamera();
        catCamera2();
        mRawCamera.open(this, "0", this);
        mRawCamera.open(this, "1", this);
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

    @Override
    public void onFrameAvailable(String cameraId, ByteBuffer buffer, int w, int h, int format,
        long timestamp) {
        if (buffer == null) return;
        Log.d(TAG, "[" + cameraId
            + "], size:" + buffer.remaining()
            + ", (" + w + ", " + h
            + "), format:" + format
            + ", timestamp:" + timestamp
            + ", delta:" + (SystemClock.elapsedRealtimeNanos() - timestamp));
        Bitmap bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ALPHA_8);
        bmp.copyPixelsFromBuffer(buffer);
        if ("0".equals(cameraId)) {
            mCameraView1.setImageBitmap(bmp);
        } else if ("1".equals(cameraId)) {
            mCameraView2.setImageBitmap(bmp);
        }
    }
}
