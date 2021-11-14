package com.z.p00;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;

import androidx.appcompat.app.AppCompatActivity;

import com.z.p00.player.ZPlayer;
import com.z.p00.player.listener.OnMediaErrorListener;
import com.z.p00.player.listener.OnMediaPreparedListener;

import java.io.File;

/*
    一个简单的音视频播放器
 */
public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";
    private final File mFile = new File(Environment.getExternalStorageDirectory(), "/Music/5.mp3");
    private ZPlayer mPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Log.e(TAG, "(java)file path: " + mFile.getAbsolutePath());
        mPlayer = new ZPlayer();
        mPlayer.setDataSource(mFile.getAbsolutePath());
        mPlayer.setOnMediaErrorListener(new OnMediaErrorListener() {
            @Override
            public void onError(int code, String text) {
                Log.e(TAG, "code: " + code + ", text: " + text);
            }
        });
        mPlayer.setOnMediaPreparedListener(new OnMediaPreparedListener() {
            @Override
            public void onPrepared() {
                Log.e(TAG, "准备完毕");
                mPlayer.play();
            }
        });
        mPlayer.prepareAsync();
    }
}