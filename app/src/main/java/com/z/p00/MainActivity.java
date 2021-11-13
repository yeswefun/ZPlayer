package com.z.p00;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;

import androidx.appcompat.app.AppCompatActivity;

import com.z.p00.player.ZPlayer;

import java.io.File;

/*

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
        mPlayer.play();
    }
}