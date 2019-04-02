package com.zdragon.videoio;

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.camera2.CameraManager;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.util.Size;
import android.view.Surface;
import android.view.TextureView;
import java.io.IOException;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MainActivity extends AppCompatActivity {

    private final static String TAG = "VideoIO";
    private final static String MIME_FORMAT = "video/avc"; //support h.264

    private TextureView mCameraTexture;
    private TextureView mDecodeTexture;

    private VideoDecoder mVideoDecoder;
    private VideoEncoder mVideoEncoder;
    //private CameraManager cameraManager;
    private Camera mCamera;
    private int mPreviewWidth;
    private int mPreviewHeight;
    private boolean mStartFirst = false;

    private Camera.PreviewCallback mPreviewCallBack = new Camera.PreviewCallback() {
        @Override
        public void onPreviewFrame(byte[] bytes, Camera camera) {

            int size = mPreviewWidth * mPreviewHeight;
            for (int i = 0 ; i < size/4 ; ++i) {
                byte value = bytes[size + 2 * i];//U
                bytes[size + 2 * i] = bytes[size + 2 * i + 1];
                bytes[size + 2 * i + 1] = value;
            }
             if(mVideoEncoder != null) {
                 for (int i = 0 ; i < 1; ++i)
                     mVideoEncoder.inputFrameToEncoder(bytes);
                Log.d(TAG,"camera to encode");

                if (mStartFirst)
                    return;
                mStartFirst = true;
                mVideoDecoder.startDecoder();
            }
        }
    };

    private TextureView.SurfaceTextureListener mCameraTextureListener = new TextureView.SurfaceTextureListener() {
        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture surfaceTexture, int i, int i1) {
            openCamera(surfaceTexture,i, i1);
            mVideoEncoder = new VideoEncoder(MIME_FORMAT, mPreviewWidth, mPreviewHeight);
            mVideoEncoder.startEncoder();
        }

        @Override
        public void onSurfaceTextureSizeChanged(SurfaceTexture surfaceTexture, int i, int i1) {
        }

        @Override
        public boolean onSurfaceTextureDestroyed(SurfaceTexture surfaceTexture) {
            if(mVideoEncoder != null){
                mVideoEncoder.release();
            }
            closeCamera();
            return true;
        }

        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture surfaceTexture) {
        }
    };

    private TextureView.SurfaceTextureListener mDecodeTextureListener = new TextureView.SurfaceTextureListener() {
        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture surfaceTexture, int i, int i1) {
            System.out.println("----------" + i + " ," + i1);
            mVideoDecoder = new VideoDecoder(MIME_FORMAT, new Surface(surfaceTexture), mPreviewWidth, mPreviewHeight);
            mVideoDecoder.setEncoder(mVideoEncoder);
            mVideoEncoder.setVideoDecoder(mVideoDecoder);
           // mVideoDecoder.startDecoder();
        }

        @Override
        public void onSurfaceTextureSizeChanged(SurfaceTexture surfaceTexture, int i, int i1) {
        }

        @Override
        public boolean onSurfaceTextureDestroyed(SurfaceTexture surfaceTexture) {
            mVideoDecoder.stopDecoder();
            return true;
        }

        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture surfaceTexture) {
        }
    };

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        initView();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
            Log.i("TEST","Granted");
            initView();
        } else {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, 1);//1 can be another integer
        }
    }

    private void initView(){
        mCameraTexture = (TextureView)findViewById(R.id.camera);
        mDecodeTexture = (TextureView)findViewById(R.id.decode);
        mCameraTexture.setSurfaceTextureListener(mCameraTextureListener);
        mDecodeTexture.setSurfaceTextureListener(mDecodeTextureListener);
    }

    private void openCamera(SurfaceTexture texture,int width, int height){
        if(texture == null){
            Log.e(TAG, "openCamera need SurfaceTexture");
            return;
        }

 //       cameraManager = (CameraManager) this.getSystemService(this.CAMERA_SERVICE);
        mCamera = Camera.open(0);
        try{
            mCamera.setPreviewTexture(texture);
            Camera.Parameters parameters = mCamera.getParameters();
            int format = parameters.getPreviewFormat();
            List<Camera.Size> list = parameters.getSupportedPreviewSizes();
            for(Camera.Size size: list){
                System.out.println("----size width = " + size.width + " size height = " + size.height);
            }
            int[] range = new int[2] ;
            parameters.getPreviewFpsRange(range);
            Log.d(TAG, "fps : " + range[0] + " " + range[1] + "format: " + format);
            mPreviewWidth = 1280;
            mPreviewHeight = 720;
            parameters.setPreviewSize(mPreviewWidth,mPreviewHeight);
            //parameters.setPreviewFormat(ImageFormat.YUV_420_888);
           // parameters.setPreviewFpsRange(7000,60000);
            mCamera.setParameters(parameters);
            mCamera.setPreviewCallback(mPreviewCallBack);
            mCamera.startPreview();
        }catch(IOException e){
            Log.e(TAG, Log.getStackTraceString(e));
            mCamera = null;
        }
    }



    private void closeCamera(){
        if(mCamera == null){
            Log.e(TAG, "Camera not open");
            return;
        }
        mCamera.stopPreview();
        mCamera.release();
    }
}
