package com.example.prototypelibass;

import androidx.appcompat.app.AppCompatActivity;

import android.content.res.AssetManager;
import android.os.Bundle;
import android.widget.TextView;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import com.example.prototypelibass.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'prototypelibass' library on application startup.
    static {
        System.loadLibrary("prototypelibass");
    }

    private ActivityMainBinding binding;
    private SurfaceView surfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        surfaceView = findViewById(R.id.surfaceView);
        surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                // Load the bitmap from resources
                Bitmap bitmap = BitmapFactory.decodeResource(getResources(), R.drawable.test_bitmap);
                // Call native method to initialize ANativeWindow and render the bitmap
                initNativeWindow(holder.getSurface(), bitmap);
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                // To-Do handle le changement au SurfaceView tels que les rotation du window et les resizes
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                // Call native method to release ANativeWindow
                releaseNativeWindow();
            }
        });


        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());

        AssetManager assetManager = getAssets();
        renderASSFile(assetManager);
    }

    /**
     * A native method that is implemented by the 'prototypelibass' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native void renderASSFile(AssetManager assetManager);
    public native void initNativeWindow(Object surface, Bitmap bitmap);
    public native void releaseNativeWindow();
}