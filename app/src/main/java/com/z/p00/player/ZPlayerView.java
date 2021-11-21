package com.z.p00.player;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceView;

import com.z.p00.player.listener.OnMediaErrorListener;
import com.z.p00.player.listener.OnMediaPreparedListener;

/*
    native层使用SurfaceView播放视频数据

    TODO: 了解一下SurfaceView挖洞机制
 */
public class ZPlayerView extends SurfaceView implements OnMediaPreparedListener {

    private static final String TAG = "ZPlayerView";
    private final ZPlayer mPlayer;

    public ZPlayerView(Context context) {
        this(context, null);
    }

    public ZPlayerView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ZPlayerView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        getHolder().setFormat(PixelFormat.RGBA_8888);

        mPlayer = new ZPlayer();
        mPlayer.setOnMediaErrorListener(new OnMediaErrorListener() {
            @Override
            public void onError(int code, String text) {
                Log.e(TAG, "code: " + code + ", text: " + text);
            }
        });
        mPlayer.setOnMediaPreparedListener(this);
    }

    public void play(String url) {
        stop();
        mPlayer.setDataSource(url);
        mPlayer.prepareAsync();
    }

    /*
        准备之后，播放之前
     */
    @Override
    public void onPrepared() {
        mPlayer.setSurface(getHolder().getSurface());
        Log.e(TAG, "准备完毕");
        mPlayer.play();
    }

    /**
     * 停止方法，释放上一个视频的内存
     */
    private void stop() {
        Log.e(TAG, "暂停上一次播放");
    }
}
