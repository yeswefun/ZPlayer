package com.z.p00.player;

import android.text.TextUtils;
import android.view.Surface;

import com.z.p00.player.listener.OnMediaErrorListener;
import com.z.p00.player.listener.OnMediaPreparedListener;

public class ZPlayer {

    static {
        System.loadLibrary("z-player");
    }

    private String url;

    public void setDataSource(String url) {
        this.url = url;
    }

    private OnMediaErrorListener mOnMediaErrorListener;
    public void setOnMediaErrorListener(OnMediaErrorListener listener) {
        mOnMediaErrorListener = listener;
    }
    /*
        native -> java
     */
    private void onError(int code, String text) {
        if (mOnMediaErrorListener != null) {
            mOnMediaErrorListener.onError(code, text);
        }
    }

    private OnMediaPreparedListener mOnMediaPreparedListener;
    public void setOnMediaPreparedListener(OnMediaPreparedListener listener) {
        mOnMediaPreparedListener = listener;
    }
    /*
        native -> java
     */
    private void onPrepared() {
        if (mOnMediaPreparedListener != null) {
            mOnMediaPreparedListener.onPrepared();
        }
    }

    public void play() {
        if (TextUtils.isEmpty(url)) {
            throw new RuntimeException("请先设置播放地址呀!");
        }
        nPlay();
    }

    public void prepareAsync() {
        if (TextUtils.isEmpty(url)) {
            throw new RuntimeException("请先设置播放地址呀!");
        }
        nPrepareAsync(url);
    }

    private native void nPrepareAsync(String url);

    private native void nPlay();

    public native void setSurface(Surface surface);
}
