package com.z.p00;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;

import com.z.p00.player.ZPlayerView;

import java.io.File;

/*
    一个简单的音视频播放器

    TODO: 引入模板设计模式

    TODO: 存在的问题: 音视频不同步
 */
public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";
    /*
        Environment.getExternalStorageDirectory()
            /storage/emulated/0

        mFile.getAbsolutePath()
            /storage/emulated/0/Music/5.mp3
     */
    private final File mFile = new File(Environment.getExternalStorageDirectory(), "/Download/r.mp4");

    private ZPlayerView mPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Log.e(TAG, "(java)file path: " + mFile.getAbsolutePath());
        mPlayer = findViewById(R.id.zplayer_view);
    }

    public void handlePlay(View view) {
        mPlayer.play(mFile.getAbsolutePath());
        decodeVideo(mPlayer.getHolder().getSurface(), mFile.getAbsolutePath());
    }

    private native void decodeVideo(Surface surface, String absolutePath);
}

/*
    9. 解码音频数据

    10. 播放音频数据
        AudioTrack
            创建AudioTrack对象
            启动循环, 设置为播放状态 - play
            将数据放到指定数组中 - write, pcm -> memory

    TODO: 内存上涨, 一直往上涨, 直到崩掉 - 代码调整

    TODO: 可以播放声音, 但声音为噪音 - 重采样

    查看apk的打包情况
        Build
            Analyze APK

    apk的性能分析
        Run
            Profile 'app'

    TODO: 内存上涨, 一直往上涨, 直到崩掉 - 代码调整
        内存平滑, 没有上涨

        要做一个什么东西, 是一个demo还是高效可用的项目
        很多细节的东西要处理完善, 回收内存, 检测内存, 包括可用性(java, native)

    TODO: 可以播放声音, 但声音为噪音 - 重采样
        指定AudioTrack的播放参数
            2 channels
            44100 sample_rate
            2 bytes
        但解码出来不一定与这些参数一样
        通过重采样, 使用其与AudioTrack中指定的参数一样

    11. 音频重采样

    W/Zygote: mz_is_rooted true

    12. 添加错误回调到java层
        子线程回调java层的错误

    13. 多线程解码
        JNIEnv是线程私有的, 所以才要传javaVM

    14. JNIEnv子线程回调java层

        JNIEnv env 是一个线程对应一个env，线程间不可以共享同一个env变量。
        那么如何在新创建的线程中使用env变量呢？

            1、JavaVM *g_vm;
                env->GetJavaVM(&g_vm);
                //来获取JavaVM指针.获取了这个指针后,将该JavaVM保存起来。（env是当前线程中对应的变量）

            2、在新线程中调用
                JNIEnv *env_new;
                // attach to JavaVM
                vm->AttachCurrentThread(&env_new, NULL)

            3、之后在新的线程中就可以使用env_new了，可以调用java接口或者是调用JNIEnv 中的方法了。

            4、新的线程结束前记得调用g_vm->DetachCurrentThread();

        JNI中JNIEnv的意义
            JNIEnv是一个指针，指向jvm中保存的JNI函数表。
            通过JNIEnv可以调用JNI函数。
            JNIEnv是线程私有的，每个线程有独自的JNIEnv。
            通过JavaVM->GetEnv()可以获得本线程的JNIEnv。


    出现异常之后, 在terminal中输入
    $ adb logcat | ndk-stack -sym app/build/intermediates/cmake/debug/obj/arm64-v8a

        /home/lin/Android/Sdk/ndk/21.1.6352462 - ndk-stack


https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/invocation.html#library_version
Attaching to the VM
The JNI interface pointer (JNIEnv) is valid only in the current thread.
Should another thread need to access the Java VM,
it must first call AttachCurrentThread() to attach itself to the VM and obtain a JNI interface pointer.
Once attached to the VM, a native thread works just like an ordinary Java thread running inside a native method.
The native thread remains attached to the VM until it calls DetachCurrentThread() to detach itself.

The attached thread should have enough stack space to perform a reasonable amount of work.
The allocation of stack space per thread is operating system-specific.
For example, using pthreads, the stack size can be specified in the pthread_attr_t argument to pthread_create.


Detaching from the VM
A native thread attached to the VM must call DetachCurrentThread() to detach itself before exiting.
A thread cannot detach itself if there are Java methods on the call stack.


JNI中的变量声明周期
    JNI中变量有三种声明周期

    1.局部引用（Local Reference）
    在所创建的native函数中有效，在函数结束时结束声明周期（可以被GC）。

    2.全局引用（Global Reference）
    可以跨方法，跨线程使用，在主动释放之前不能被GC。

    3.弱全局引用（Weak Global Reference）
    可以跨方法，跨线程使用，不释放也可能被GC。


    出现异常之后, 在terminal中输入
    $ adb logcat | ndk-stack -sym app/build/intermediates/cmake/debug/obj/arm64-v8a

        /home/lin/Android/Sdk/ndk/21.1.6352462 - ndk-stack


    15. OpenSLES播放音频原理
        sl00

    16. OpenSLES播放音频数据
        initCreateOpenSLES();

    17. 代码结构的调整, 引入生产者与消费者模型(queue + cond + mutex)
        ZAudio
        ZPacketQueue
        ZPlayerState

为员工缴纳六险一金（养老、医疗、失业、生育、工伤、住房公积金、商业险）

$ man dlopen
$ man dlclose

adb logcat | ndk-stack -sym app/build/intermediates/cmake/debug/obj/arm64-v8a

    18. 使用SurfaceView播放视频数据

    19. 整合SurfaceView
        ZVideo

    20. 模板设计模式
        ZMedia

    21. 音视频同步的原理

播放视频的流程跟播放音频的流程类似
*/