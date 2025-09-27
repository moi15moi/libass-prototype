package com.example.prototypelibass;

import android.content.res.AssetManager;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;

import javax.microedition.khronos.opengles.GL10;

public class Renderer implements GLSurfaceView.Renderer {
    private int textureId;
    private Long context;
    private AssetManager assetManager;

    public Renderer(AssetManager assetManager) {
        this.assetManager = assetManager;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, javax.microedition.khronos.egl.EGLConfig config) {
        // Setup OpenGL state if needed (disable depth test, etc.)

    }

    @Override
    public void onDrawFrame(GL10 gl) {
        this.context = MainActivity.nativeInitializeLibplacebo();
        this.textureId = MainActivity.renderSubtitleFrame(this.assetManager, 500, 400, 0, context);

        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);

        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId);

        // Draw a fullscreen quad with this texture bound.
        // You’ll need a simple shader + vertex buffer for a quad covering NDC [-1,1].
    }


    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        GLES20.glViewport(0, 0, width, height);
    }
}
