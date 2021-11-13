package com.z.p00.player;

import android.text.TextUtils;

public class ZPlayer {

    static {
        System.loadLibrary("z-player");
    }

    private String url;

    public void setDataSource(String url) {
        this.url = url;
    }

    public void play() {
        if (TextUtils.isEmpty(url)) {
            throw new RuntimeException("请先设置播放地址呀!");
        }
        nPlay(url);
    }

    private native void nPlay(String url);

}
