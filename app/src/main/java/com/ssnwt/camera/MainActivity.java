package com.ssnwt.camera;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraDevice.StateCallback;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.util.Log;
import android.util.Size;
import android.view.Surface;
import android.view.TextureView;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import androidx.core.app.ActivityCompat;
import java.io.IOException;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class MainActivity extends AppCompatActivity implements
    TextureView.SurfaceTextureListener {
    private static final String TAG = "S_Camera_Activity";
    private TextureView mTextureView;
    private CameraManager mCameraManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mTextureView = findViewById(R.id.preview);
        mTextureView.setSurfaceTextureListener(this);
        mCameraManager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
    }

    private void openCamera1() {
        for (int i = 0; i < Camera.getNumberOfCameras(); i++) {
            Camera.CameraInfo info = new Camera.CameraInfo();
            Camera.getCameraInfo(i, info);
            Log.d(TAG, "facing=" + info.facing + ", orientation=" + info.orientation);
        }
        Camera camera = Camera.open(0);
        Camera.Parameters parameters = camera.getParameters();
        List<Camera.Size> sizes = parameters.getSupportedPreviewSizes();
        for (Camera.Size size : sizes) {
            Log.d(TAG, "size:(" + size.width + ", " + size.height + ").");
        }
        parameters.setPreviewSize(sizes.get(0).width, sizes.get(0).height);
        parameters.setPreviewFormat(ImageFormat.NV21);
        camera.setParameters(parameters);
        try {
            camera.setPreviewTexture(mTextureView.getSurfaceTexture());
        } catch (IOException e) {
            e.printStackTrace();
        }
        Log.d(TAG, "startPreview 111");
        camera.startPreview();
        Log.d(TAG, "startPreview 222");
    }

    private String catCamera2() {
        String camera = null;
        try {
            for (String cameraId : mCameraManager.getCameraIdList()) {
                Log.d(TAG, "cameraId=" + cameraId);
                CameraCharacteristics characteristics =
                    mCameraManager.getCameraCharacteristics(cameraId);
                StreamConfigurationMap map =
                    characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                int[] formats = map.getOutputFormats();
                Log.d(TAG, "formats=" + Arrays.toString(formats));
                //for (int format : formats) {
                //    for (Size size : map.getOutputSizes(format)) {
                //        Log.d(TAG, "size=" + size);
                //    }
                //}
                if (camera == null) camera = cameraId;
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        return camera;
    }

    private void openCamera2() {
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
            != PackageManager.PERMISSION_GRANTED) {
            return;
        }
        String camera = catCamera2();
        //prepareRawCamera(camera);
        try {
            Log.d(TAG, "openCamera=0");
            mCameraManager.openCamera(camera, new StateCallback() {
                @Override public void onOpened(@NonNull CameraDevice camera) {
                    Log.d(TAG, "onOpened=" + camera);
                    try {
                        startPreview2(camera);
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

    private void startPreview2(CameraDevice camera) throws CameraAccessException {
        Log.d(TAG, "startPreview");
        CaptureRequest.Builder builder = camera.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
        builder.set(CaptureRequest.FLASH_MODE, CaptureRequest.FLASH_MODE_OFF);
        Surface surface = new Surface(mTextureView.getSurfaceTexture());
        builder.addTarget(surface);
        List<Surface> surfaces = Arrays.asList(surface);
        if (mRawImageReader != null) {
            builder.addTarget(mRawImageReader.getSurface());
            surfaces.add(mRawImageReader.getSurface());
        }
        camera.createCaptureSession(surfaces, new CameraCaptureSession.StateCallback() {
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

    private ImageReader.OnImageAvailableListener mOnRawImageAvailableListener = reader -> {
        Image image = reader.acquireNextImage();
        Image.Plane[] planes = image.getPlanes();
        if (planes != null && planes.length > 0) {
            Log.d(TAG, "onFrame:(" + image.getWidth()
                + ", " + image.getHeight()
                + "), limit=" + planes[0].getBuffer().limit()
                + ", timestamp=" + image.getTimestamp()
                + ", PixelStride=" + planes[0].getPixelStride()
                + ", RowStride=" + planes[0].getRowStride());
        }
        image.close();
    };
    private ImageReader mRawImageReader;

    private void prepareRawCamera(String camera) {
        CameraCharacteristics characteristics = null;
        try {
            characteristics = mCameraManager.getCameraCharacteristics(camera);
            if (contains(characteristics.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES),
                CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_RAW)) {
                StreamConfigurationMap map = characteristics.get(
                    CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                Size largestRaw = Collections.max(
                    Arrays.asList(map.getOutputSizes(ImageFormat.RAW_SENSOR)),
                    new CompareSizesByArea());
                mRawImageReader = ImageReader.newInstance(largestRaw.getWidth(),
                    largestRaw.getHeight(), ImageFormat.RAW_SENSOR, /*maxImages*/ 5);
                mRawImageReader.setOnImageAvailableListener(mOnRawImageAvailableListener, null);
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onSurfaceTextureAvailable(@NonNull SurfaceTexture surface, int width, int height) {
        openCamera2();
        //openCamera1();
    }

    @Override public void onSurfaceTextureSizeChanged(@NonNull SurfaceTexture surface, int width,
        int height) {

    }

    @Override public boolean onSurfaceTextureDestroyed(@NonNull SurfaceTexture surface) {
        return false;
    }

    @Override public void onSurfaceTextureUpdated(@NonNull SurfaceTexture surface) {
        //Log.d(TAG, "onSurfaceTextureUpdated");
    }

    private static boolean contains(int[] modes, int mode) {
        if (modes == null) {
            return false;
        }
        for (int i : modes) {
            if (i == mode) {
                return true;
            }
        }
        return false;
    }

    static class CompareSizesByArea implements Comparator<Size> {
        @Override
        public int compare(Size lhs, Size rhs) {
            // We cast here to ensure the multiplications won't overflow
            return Long.signum((long) lhs.getWidth() * lhs.getHeight() -
                (long) rhs.getWidth() * rhs.getHeight());
        }
    }
}