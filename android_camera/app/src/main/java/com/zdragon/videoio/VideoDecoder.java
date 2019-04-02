package com.zdragon.videoio;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Handler;
import android.os.HandlerThread;
import android.support.annotation.NonNull;
import android.util.Log;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * This class use for Decode Video Frame Data and show to SurfaceTexture
 * Created by zj on 2018/7/29 0029.
 */
public class VideoDecoder {
    private final static String TAG = "VideoEncoder";
    private final static int CONFIGURE_FLAG_DECODE = 0;

    private MediaCodec  mMediaCodec;
    private MediaFormat mMediaFormat;
    private Surface     mSurface;
    private int         mViewWidth;
    private int         mViewHeight;

    private Map<Long,Long> mTimestamp = new HashMap<Long, Long>();
    private VideoEncoder mVideoEncoder;
    private Handler mVideoDecoderHandler;
    private HandlerThread mVideoDecoderHandlerThread = new HandlerThread("VideoDecoder");

    private long mEAvage = 0;
    private long mEFrames = 0;

    private final List<VideoFrame> mInputDatasQueue = new LinkedList();
    private final List<Integer> mInputIDQueue = new LinkedList<>();

    private DecodeThread mDencodeThread;

    public class DecodeThread implements Runnable {
        @Override
        public void run() {
            while (true) {
                VideoFrame frame = null;
                while (true) {
                    synchronized (mInputDatasQueue) {
                        if (mInputDatasQueue.isEmpty()) {
                            try {
                                mInputDatasQueue.wait(1000L);
                                continue;
                            } catch (InterruptedException var4) {
                                continue;
                            }
                        }
                        frame = mInputDatasQueue.get(0);
                        mInputDatasQueue.remove(0);
                    }
                    break;
                }
                int mBufferIndex = -1;
                while (true) {
                    synchronized (mInputIDQueue) {
                        if (mInputIDQueue.isEmpty()) {
                            try {
                                mInputIDQueue.wait();
                                continue;
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                                continue;
                            }
                        }
                        mBufferIndex = mInputIDQueue.get(0);
                        mInputIDQueue.remove(0);
                    }
                    break;
                }
                ByteBuffer inputBuffer = mMediaCodec.getInputBuffer(mBufferIndex);
                inputBuffer.clear();
                Log.d(TAG, "decode input");
                int length = 0;
                Log.d(TAG, "decode input data + period " + frame.timestamp);
                inputBuffer.put(frame.frame);
                length = frame.frame.length;
                long timestamp = System.currentTimeMillis();
                mTimestamp.put(frame.timestamp, timestamp);
                mMediaCodec.queueInputBuffer(mBufferIndex, 0, length, frame.timestamp, 0);
            }
        }
    };

    private MediaCodec.Callback mCallback = new MediaCodec.Callback() {
        @Override
        public void onInputBufferAvailable(@NonNull MediaCodec mediaCodec, int id) {

            synchronized (mInputIDQueue) {
                mInputIDQueue.add(id);
                mInputIDQueue.notifyAll();
            }
        }

        private long mTimeBegin = 0;
        @Override
        public void onOutputBufferAvailable(@NonNull MediaCodec mediaCodec, int id, @NonNull MediaCodec.BufferInfo bufferInfo) {
  //         Log.d(TAG, "decode output + period " + bufferInfo.presentationTimeUs);
            //ByteBuffer outputBuffer = mMediaCodec.getOutputBuffer(id);
            long pts = bufferInfo.presentationTimeUs;
            mMediaCodec.releaseOutputBuffer(id, true);
            long timestamp = mTimestamp.get(pts);
            mTimestamp.remove(pts);
            long timeCurrent = System.currentTimeMillis();

           //*/
            if (mEFrames <= 0)
                mTimeBegin = timeCurrent;
            long timediff = (timeCurrent - timestamp);
            mEAvage += timediff;
            mEFrames += 1;
            if (mEFrames >= 100) {
                double fEAvage = 0;
                fEAvage = 1.0 * mEAvage / mEFrames;
                double fps = 1000.0 * mEFrames / (timeCurrent - mTimeBegin);
                mEAvage = 0;
                mEFrames = 0;
                mTimeBegin = timeCurrent;
                Log.d(TAG, "decode output release + period " + bufferInfo.presentationTimeUs + "decode frame avgetime " + fEAvage + "fps " + fps);
            } else {
                Log.d(TAG, "decode output release + period " + bufferInfo.presentationTimeUs + " " + timediff);
            }
        }

        @Override
        public void onError(@NonNull MediaCodec mediaCodec, @NonNull MediaCodec.CodecException e) {
            Log.d(TAG, "------> onError");
        }

        @Override
        public void onOutputFormatChanged(@NonNull MediaCodec mediaCodec, @NonNull MediaFormat mediaFormat) {
            Log.d(TAG, "------> onOutputFormatChanged " + mediaFormat.toString());
        }
    };

    public VideoDecoder(String mimeType, Surface surface, int viewwidth, int viewheight){
        try {
            mMediaCodec = MediaCodec.createDecoderByType(mimeType);
        } catch (IOException e) {
            Log.e(TAG, Log.getStackTraceString(e));
            mMediaCodec = null;
            return;
        }

        if(surface == null){
            return;
        }

        this.mViewWidth  = viewwidth;
        this.mViewHeight = viewheight;
        this.mSurface = surface;

        mVideoDecoderHandlerThread.start();
        mVideoDecoderHandler = new Handler(mVideoDecoderHandlerThread.getLooper());

        mMediaFormat = MediaFormat.createVideoFormat(mimeType, mViewWidth, mViewHeight);
        mMediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible);//COLOR_FormatRGBAFlexible
        mMediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, 4000000);
        mMediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, 60);
        mMediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);
    }

    public void setEncoder(VideoEncoder videoEncoder){
        this.mVideoEncoder = videoEncoder;
    }

    public void startDecoder(){
        if(mMediaCodec != null && mSurface != null){
            mMediaCodec.setCallback(mCallback, mVideoDecoderHandler);
            mMediaCodec.configure(mMediaFormat, mSurface,null,CONFIGURE_FLAG_DECODE);
            mMediaCodec.start();

            mDencodeThread = new DecodeThread();
            new Thread(mDencodeThread).start();
        }else{
            throw new IllegalArgumentException("startDecoder failed, please check the MediaCodec is init correct");
        }
    }

    public void stopDecoder(){
        if(mMediaCodec != null){
            mMediaCodec.stop();
        }
    }

    /**
     * release all resource that used in Encoder
     */
    public void release(){
        if(mMediaCodec != null){
            mMediaCodec.release();
            mMediaCodec = null;
            mInputDatasQueue.clear();
        }
    }

    public int addFrame(VideoFrame frame) {
        synchronized(mInputDatasQueue) {
            mInputDatasQueue.notify();
            mInputDatasQueue.add(frame);
        }
        return  0;
    }
}
