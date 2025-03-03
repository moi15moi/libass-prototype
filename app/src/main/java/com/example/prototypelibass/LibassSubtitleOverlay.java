package com.example.prototypelibass;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.media3.common.util.UnstableApi;
import androidx.media3.effect.BitmapOverlay;
import androidx.media3.effect.OverlaySettings;

/**
    * Custom BitmapOverlay to render libass subtitles.
*/

@UnstableApi public class LibassSubtitleOverlay extends BitmapOverlay{
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
    private Bitmap lastBitmap;

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
      public Bitmap getBitmap(long presentationTimeUs){
        // Convert timestamp to ms
        long timestamp = presentationTimeUs / 1000;

        try{
            // Call our RenderSubtitleFrame
            Bitmap newBitmap = mainActivity.renderSubtitleFrame(assetManager, videoWidth, videoHeight,(int) timestamp);

            // Update current Bitmap.
            if (newBitmap != null){
                currentBitmap = newBitmap;
            }
        }catch (Exception e)
        {
            Log.e(TAG, "Failed to render subtitle frame");
        }
        return currentBitmap;
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
    @NonNull
    @Override
    public OverlaySettings getOverlaySettings(long presentationTimeUs) {
        // Create settings to position the subtitles at the bottom of the frame
        return new OverlaySettings.Builder()
                // Alpha scale = 1.0 means fully opaque (no transparency) change later we want transparent
                .setAlphaScale(1.0f)
                
                // Position bottom
                // (0, -0.7) means center horizontally, and 70% down from the center (toward bottom)
                .setBackgroundFrameAnchor(0, -0.7f)

                .setOverlayFrameAnchor(0.5f, 0.5f) // Center the overlay on the screen

                .setScale(1.0f, 1.0f) // Scale the overlay to the full width of the screen, but no height

                // .setRotationDegrees(0) // If you need to rotate the overlay
                
                .build();
    }

    /**
     * Release any resources used by the overlay.
     * Maybe memory leak
     * 
     */
    @Override
    public void release() {
        // Release any resources used by the overlay
        if (currentBitmap != null) {
            currentBitmap.recycle();
            currentBitmap = null;
        }
    }


}
