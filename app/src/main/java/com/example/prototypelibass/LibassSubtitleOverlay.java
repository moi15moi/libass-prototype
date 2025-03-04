package com.example.prototypelibass;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.util.Log;


import androidx.media3.common.util.UnstableApi;
import androidx.media3.effect.BitmapOverlay;
import androidx.media3.effect.OverlaySettings;

/**
    * Custom BitmapOverlay to render libass subtitles.
*/

@UnstableApi public class LibassSubtitleOverlay extends BitmapOverlay {
    private static final String TAG = "LibassSubtitleOverlay";

    // Reference to main activity
    private MainActivity mainActivity;

    // Reference to AssetManager
    private AssetManager assetManager;
    
    // Dimension for video
    private int videoWidth;
    private int videoHeight;

    // Cache the last timestamps(ms) we rendered the subtitles for
    private long lastTimestamp = -1;

    // Cache current Bitmap being displayed.
    private Bitmap currentBitmap;

    /**
     * Create a new instance of LibassSubtitleOverlay.
     * @param mainActivity Reference to main activity
     * @param videoWidth Width of the video
     * @param videoHeight Height of the video
     */
     public LibassSubtitleOverlay(MainActivity mainActivity, int videoWidth, int videoHeight){
         this.mainActivity = mainActivity;
         this.assetManager = mainActivity.getAssets();
         this.videoWidth = videoWidth;
         this.videoHeight = videoHeight;
     }

    /**
     * Get subtitle Bitmap for the current playback position
    * for everyframe during playback, we will call this method to get the subtitle bitmap
    * Maybe to do optimization to check if we really need to re-render.
    * 1. Call our native method to get our Bitmap
    * 2. Return the bitmap to the overlay
    */
    @Override
    public Bitmap getBitmap(long presentationTimeUs) {
        // Convert timestamp to ms
        long timestamp = presentationTimeUs / 1000;

        try {
            // Now get subtitle from native code based on timestamp
            Bitmap newBitmap = mainActivity.renderSubtitleFrame(assetManager, videoWidth, videoHeight, (int) timestamp);

            if (newBitmap != null) {
                // If native code returns a bitmap, use it
                if (currentBitmap != null && currentBitmap != newBitmap) {
                    // Recycle old bitmap to avoid memory leaks
                    currentBitmap.recycle();
                }
                currentBitmap = newBitmap;
                Log.d(TAG, "Updated bitmap from native code at: " + timestamp + "ms");
            } else {
                // If native code returns null (no subtitle at this timestamp),
                // we can either return null or a transparent bitmap
                Log.d(TAG, "Native code returned null bitmap at: " + timestamp + "ms");
            }
        } catch (Exception e) {
            Log.e(TAG, "Error rendering subtitle frame: " + e.getMessage());
        }

        return currentBitmap;  // Return whatever bitmap we have
    }


    /**
     * Customize the overlay settings to position the subtitles on the video frame.
     * 
     * This method allows us to control:
     * - Where the subtitles appear on the screen
     * - How large they are relative to the video
     * - Their transparency/opacity
     * - Any other visual settings
     * 
     * @param presentationTimeUs The presentation timestamp in microseconds
     * @return The OverlaySettings for positioning and displaying the subtitle
     */
    @Override
    public OverlaySettings getOverlaySettings(long presentationTimeUs) {
        // Create settings to position the subtitles at the bottom center of the frame
        return new OverlaySettings.Builder()
                .setAlphaScale(1.0f) // Fully opaque
                .setBackgroundFrameAnchor(0.0f, 0.7f) // Position at the bottom center of the screen
                .setOverlayFrameAnchor(0.5f, 0.5f) // Align the center of the subtitle with the anchor point
                .setScale(0.8f, 0.8f) // Scale to 80% of video width and 20% of height
                .build();
    }

    /**
     * Release any resources used by the overlay.
     * Maybe memory leak
     * 
     */
    @Override
    public void release() {
        if (currentBitmap != null && !currentBitmap.isRecycled()) {
            currentBitmap.recycle();
            currentBitmap = null;
        }
    }


}
