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
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * This class use for Encode Video Frame Data.
 * Created by zj on 2018/7/29 0029.
 */
public class VideoEncoder {

    private final static String TAG = "VideoEncoder";
    private final static int CONFIGURE_FLAG_ENCODE = MediaCodec.CONFIGURE_FLAG_ENCODE;

    private MediaCodec  mMediaCodec;
    private MediaFormat mMediaFormat;
    private int         mViewWidth;
    private int         mViewHeight;
    private long        mPeriod = 0;

    private byte[]      spspps = null;

    private Map<Long,Long> mTimestamp = new HashMap<Long, Long>();
    private Handler mVideoEncoderHandler;
    private HandlerThread mVideoEncoderHandlerThread = new HandlerThread("VideoEncoder");

    private long mEAvage = 0;
    private long mEFrames = 0;
    //This video stream format must be I420
    private final List<byte[]>  mInputDatasQueue = new LinkedList();

    private final List<Integer> mInputIDQueue = new LinkedList<>();


    private final List<BufferInfo> mOutputQueue = new LinkedList<>();

    //Cachhe video stream which has been encoded.
    private VideoDecoder mVideoDecoder;
    private EncodeThread  mEncodeThread;

    private OutProcess    mOutProcess;

    private long mTimeBegin = 0;

    public class OutProcess implements Runnable {
        @Override
        public void run() {
            while (true) {
                BufferInfo bufferInfo = null;
                while (true) {
                    synchronized (mOutputQueue) {
                        if (mOutputQueue.isEmpty()) {
                            try {
                                mOutputQueue.wait(1000L);
                                continue;
                            } catch (InterruptedException var4) {
                                continue;
                            }
                        }
                        bufferInfo = mOutputQueue.get(0);
                        mOutputQueue.remove(0);
                    }
                    break;
                }

                ByteBuffer outputBuffer = mMediaCodec.getOutputBuffer(bufferInfo.index);
                Log.d(TAG, "encode output + period " + bufferInfo.presentationTimeUs);

                byte[] buffer = new  byte[bufferInfo.size];
                outputBuffer.get(buffer);
                mMediaCodec.releaseOutputBuffer(bufferInfo.index, false);
                int offset = 4;
                if (buffer[2] == 0x01)
                    offset = 3;
                int type = (buffer[offset] & 0x1f);
                Log.d(TAG, "NAL type " + type);
                if (type == 0x07 || type == 0x08) { // sps pps
                    //spspps = buffer;
                    VideoFrame frame = new VideoFrame();
                    frame.frame = buffer;
                    frame.timestamp = 0;//
                    mVideoDecoder.addFrame(frame);
                    Log.d(TAG, "SPS PPS" + Arrays.toString(buffer));
                    continue;
                }

                long timestamp = mTimestamp.get(bufferInfo.presentationTimeUs);
                mTimestamp.remove(bufferInfo.presentationTimeUs);
                long timeCurrent = System.currentTimeMillis();
                if (mEFrames <= 0)
                    mTimeBegin = timeCurrent;
                long timediff = (timeCurrent - timestamp);
                mEAvage += timediff;
                mEFrames += 1;
                if (type == 0x01) {
                    VideoFrame frame = new VideoFrame();
                    frame.frame = buffer;
                    frame.timestamp = bufferInfo.presentationTimeUs;//
                    mVideoDecoder.addFrame(frame);
                } else if (type == 0x05) {
                    VideoFrame frame = new VideoFrame();
                    frame.frame = buffer;
                    frame.timestamp = bufferInfo.presentationTimeUs;//
                    mVideoDecoder.addFrame(frame);
                }
                if (mEFrames >= 100) {
                    double fEAvage = 0;
                    fEAvage = 1.0 * mEAvage / mEFrames;
                    double fps = 1000.0 * mEFrames / (timeCurrent - mTimeBegin);
                    mEAvage = 0;
                    mEFrames = 0;
                    mTimeBegin = timeCurrent;
                    Log.d(TAG, "encode output release + period " + bufferInfo.presentationTimeUs + "encode frame avgetime " + fEAvage + "fps " + fps);
                } else {
                    Log.d(TAG, "encode output release + period " + bufferInfo.presentationTimeUs + " " + timediff);
                }
            }

        }

    };

    public class EncodeThread implements Runnable {
        @Override
        public void run() {

            while (true) {
                byte[] dataSources = null;
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
                        dataSources = mInputDatasQueue.get(0);
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
                Log.d(TAG, "encode input");
                int length = 0;
                inputBuffer.put(dataSources);
                length = dataSources.length;
                Log.d(TAG, "encode source data + period " + mPeriod);
                long timestamp = System.currentTimeMillis();
                mTimestamp.put(mPeriod, timestamp);
                mMediaCodec.queueInputBuffer(mBufferIndex, 0, length, mPeriod, 0);
                mPeriod += 16;
            }
        }
    };

    private MediaCodec.Callback mCallback = new MediaCodec.Callback() {
        @Override
        public void onInputBufferAvailable(@NonNull MediaCodec mediaCodec, int id) {
            Log.d(TAG, "onInputBufferAvailable " + id);
            synchronized (mInputIDQueue) {
                mInputIDQueue.add(id);
                mInputIDQueue.notifyAll();
            }
        }


        @Override
        public void onOutputBufferAvailable(@NonNull MediaCodec mediaCodec, int id, @NonNull MediaCodec.BufferInfo bufferInfo) {
            Log.d(TAG, "onOutputBufferAvailable " + id);
            BufferInfo bInfo = new BufferInfo();
            bInfo.index = id;
            bInfo.offset = bufferInfo.offset;
            bInfo.size = bufferInfo.size;
            bInfo.presentationTimeUs = bufferInfo.presentationTimeUs;
            bInfo.flags = bufferInfo.flags;
            synchronized (mOutputQueue) {
                mOutputQueue.add(bInfo);
                mOutputQueue.notifyAll();
            }

            /*
            ByteBuffer outputBuffer = mMediaCodec.getOutputBuffer(id);
            Log.d(TAG, "encode output + period " + bufferInfo.presentationTimeUs);

            byte[] buffer = new  byte[bufferInfo.size];
            outputBuffer.get(buffer);
            mMediaCodec.releaseOutputBuffer(id, false);
            int offset = 4;
            if (buffer[2] == 0x01)
                offset = 3;
            int type = (buffer[offset] & 0x1f);
            Log.d(TAG, "NAL type " + type);
            if (type == 0x07 || type == 0x08) { // sps pps
                //spspps = buffer;
                VideoFrame frame = new VideoFrame();
                frame.frame = buffer;
                frame.timestamp = 0;//
                mVideoDecoder.addFrame(frame);
                Log.d(TAG, "SPS PPS" + Arrays.toString(buffer));
                return;
            }

            long timestamp = mTimestamp.get(bufferInfo.presentationTimeUs);
            mTimestamp.remove(bufferInfo.presentationTimeUs);
            long timeCurrent = System.currentTimeMillis();
            if (mEFrames <= 0)
                mTimeBegin = timeCurrent;
            long timediff = (timeCurrent - timestamp);
            mEAvage += timediff;
            mEFrames += 1;
            if (type == 0x01) {
                VideoFrame frame = new VideoFrame();
                frame.frame = buffer;
                frame.timestamp = bufferInfo.presentationTimeUs;//
                mVideoDecoder.addFrame(frame);
            } else if (type == 0x05) {
                VideoFrame frame = new VideoFrame();
                frame.frame = buffer;
                frame.timestamp = bufferInfo.presentationTimeUs;//
                mVideoDecoder.addFrame(frame);
            }
            if (mEFrames >= 100) {
                double fEAvage = 0;
                fEAvage = 1.0 * mEAvage / mEFrames;
                double fps = 1000.0 * mEFrames / (timeCurrent - mTimeBegin);
                mEAvage = 0;
                mEFrames = 0;
                mTimeBegin = timeCurrent;
                Log.d(TAG, "encode output release + period " + bufferInfo.presentationTimeUs + "encode frame avgetime " + fEAvage + "fps " + fps);
            } else {
                Log.d(TAG, "encode output release + period " + bufferInfo.presentationTimeUs + " " + timediff);
            }//*/
        }

        @Override
        public void onError(@NonNull MediaCodec mediaCodec, @NonNull MediaCodec.CodecException e) {
            Log.d(TAG, "------> onError");
        }

        @Override
        public void onOutputFormatChanged(@NonNull MediaCodec mediaCodec, @NonNull MediaFormat mediaFormat) {
            Log.d(TAG, "encode ------> onOutputFormatChanged " + mediaFormat.toString());
        }
    };

    public VideoEncoder(String mimeType, int viewwidth, int viewheight){
        try {
            mMediaCodec = MediaCodec.createEncoderByType(mimeType);
        } catch (IOException e) {
            Log.e(TAG, Log.getStackTraceString(e));
            mMediaCodec = null;
            return;
        }

        this.mViewWidth  = viewwidth;
        this.mViewHeight = viewheight;

        mVideoEncoderHandlerThread.start();
        mVideoEncoderHandler = new Handler(mVideoEncoderHandlerThread.getLooper());

        mMediaFormat = MediaFormat.createVideoFormat(mimeType, mViewWidth, mViewHeight);
        //mMediaFormat.setInteger(MediaFormat.KEY_PROFILE,MediaCodecInfo.CodecProfileLevel.AVCProfileBaseline);
        mMediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible);
        mMediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, 4000000);
        mMediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, 60);
        mMediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);
        mMediaFormat.setInteger(MediaFormat.KEY_INTRA_REFRESH_PERIOD, 60);
    }

    /**
     * Input Video stream which need encode to Queue
     * @param needEncodeData I420 format stream
     */
    public void inputFrameToEncoder(byte [] needEncodeData){
        synchronized(mInputDatasQueue) {
            mInputDatasQueue.add(needEncodeData);
            mInputDatasQueue.notify();
        }
        Log.d(TAG, "-----> inputEncoder queue current size = " + mInputDatasQueue.size());
    }


    /**
     * start the MediaCodec to encode video data
     */
    public void startEncoder(){
        if(mMediaCodec != null){
            mMediaCodec.setCallback(mCallback, mVideoEncoderHandler);
            mMediaCodec.configure(mMediaFormat, null, null, CONFIGURE_FLAG_ENCODE);
            mMediaCodec.start();
            mEncodeThread = new EncodeThread();
            new Thread(mEncodeThread).start();
            mOutProcess = new OutProcess();
            new Thread(mOutProcess).start();
        }else{
            throw new IllegalArgumentException("startEncoder failed,is the MediaCodec has been init correct?");
        }
    }

    public void setVideoDecoder(VideoDecoder decoder) {
        mVideoDecoder = decoder;
    }

    /**
     * stop encode the video data
     */
    public void stopEncoder(){
        if(mMediaCodec != null){
            mMediaCodec.stop();
            mMediaCodec.setCallback(null);
        }
    }

    /**
     * release all resource that used in Encoder
     */
    public void release(){
        if(mMediaCodec != null){
            mInputDatasQueue.clear();
            mMediaCodec.release();
        }
    }
}
