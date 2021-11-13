package com.z.p00.player;

import android.text.TextUtils;

import com.z.p00.player.listener.OnMediaErrorListener;

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

    public void play() {
        if (TextUtils.isEmpty(url)) {
            throw new RuntimeException("请先设置播放地址呀!");
        }
        nPlay(url);
    }

    private native void nPlay(String url);

}
