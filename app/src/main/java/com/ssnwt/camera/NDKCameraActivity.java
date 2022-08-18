package com.ssnwt.camera;

import android.content.Context;
import android.graphics.Bitmap;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.widget.ImageView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.IntBuffer;
import java.util.Arrays;
import java.util.Objects;

public class NDKCameraActivity extends AppCompatActivity {
    private static final String TAG = "SSNWT_" + NDKCameraActivity.class.getSimpleName();
    private ImageView mCameraView1, mCameraView2, mCameraView3, mCameraView4;
    private int mSaveCamera6, mSaveCamera1, mSaveCamera2, mSaveCamera3;

    @Override protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_camera);
        catCamera2();
        mCameraView1 = findViewById(R.id.camera_view_1);
        mCameraView2 = findViewById(R.id.camera_view_2);
        mCameraView3 = findViewById(R.id.camera_view_3);
        mCameraView4 = findViewById(R.id.camera_view_4);
        openCamera("6");

        findViewById(R.id.btn_save).setOnClickListener(view -> {
            mSaveCamera6 = 2;
            mSaveCamera1 = 2;
            mSaveCamera2 = 2;
            mSaveCamera3 = 2;
            deleteOld();
        });

        //main.sendEmptyMessage(0);
    }

    @Override protected void onDestroy() {
        super.onDestroy();
        closeCamera("6");
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

    void deleteOld() {
        File folder = getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
        if (folder == null || !folder.isDirectory()) return;
        for (File file : Objects.requireNonNull(folder.listFiles())) {
            if (file.getName().startsWith("save_")) {
                file.delete();
            }
        }
    }

    /**
     * <pre>
     * adb shell setprop persist.debug_stop_qvr_service 1
     * adb shell setprop persist.wfd.single.disable 1
     * adb shell setprop persist.ssnwtvr.display.enable false
     *
     * 保存路径：/sdcard/Android/data/com.ssnwt.camera/files/Download
     * </>
     */
    void save(String cameraId, byte[] data, int size, int width, int height, long timestamp) {
        new Thread(() -> {
            String path = getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS) + File.separator
                + "save_" + cameraId + "_" + timestamp + ".png";
            Log.d(TAG, "Save path:" + path);

            Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            for (int h = 0; h < height; h++) {
                for (int w = 0; w < width; w++) {
                    byte color = data[h * width + w];
                    bmp.setPixel(w, h, 0xff000000 | color << 16 | color << 8 | color);
                }
            }

            File file = new File(path);
            try {
                BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(file));
                bmp.compress(Bitmap.CompressFormat.PNG, 80, bos);
                bos.flush();
                bos.close();
            } catch (IOException e) {
                Log.d(TAG, "IOException", e);
            } finally {
                if (bmp != null && !bmp.isRecycled()) bmp.recycle();
            }
        }).start();
    }

    // Called by jni
    IntBuffer buffer = null;

    private void onImageAvailable(String cameraId, byte[] data, int size,
        int w, int h, int format, long timestamp) {
        if (data == null) return;
        if (mSaveCamera6 > 0 && "6".equals(cameraId)) {
            mSaveCamera6--;
            save(cameraId, data, size, w, h, timestamp);
        } else if (mSaveCamera1 > 0 && "1".equals(cameraId)) {
            mSaveCamera1--;
            save(cameraId, data, size, w, h, timestamp);
        } else if (mSaveCamera2 > 0 && "2".equals(cameraId)) {
            mSaveCamera2--;
            save(cameraId, data, size, w, h, timestamp);
        } else if (mSaveCamera3 > 0 && "3".equals(cameraId)) {
            mSaveCamera3--;
            save(cameraId, data, size, w, h, timestamp);
        }
        runOnUiThread(() -> {
            if (buffer == null) buffer = IntBuffer.allocate(w * h);
            buffer.clear();
            for (int i = 0; i < w * h; i++) {
                buffer.put(0xff000000 | data[i] << 16 | data[i] << 8 | data[i]);
            }
            buffer.rewind();
            Bitmap bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
            bmp.copyPixelsFromBuffer(buffer);
            if ("0".equals(cameraId)) {
                mCameraView1.setImageBitmap(bmp);
            } else if ("1".equals(cameraId)) {
                mCameraView2.setImageBitmap(bmp);
            } else if ("2".equals(cameraId)) {
                mCameraView3.setImageBitmap(bmp);
            } else if ("3".equals(cameraId)) {
                mCameraView4.setImageBitmap(bmp);
            } else if ("6".equals(cameraId)) {
                mCameraView1.setImageBitmap(bmp);
            }
        });
    }

    private Handler main = new Handler(Looper.getMainLooper()) {
        @Override public void handleMessage(@NonNull Message msg) {
            readCameraData();
            sendEmptyMessageDelayed(0, 16);
        }
    };

    private native void openCamera(String id);

    private native void readCameraData();

    private native void closeCamera(String id);

    static {
        System.loadLibrary("ndk_camera");
    }
}
