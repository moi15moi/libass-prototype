package com.example.prototypelibass;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import java.util.ArrayList;
import java.util.List;


import androidx.appcompat.app.AppCompatActivity;
import com.example.prototypelibass.databinding.ActivityMainBinding;
import com.google.common.collect.ImmutableList;

// Media3/Exoplayer Classes
import androidx.media3.common.Effect;
import androidx.media3.common.MediaItem;
import androidx.media3.common.Player;
import androidx.media3.common.VideoSize;
import androidx.media3.common.util.UnstableApi;
import androidx.media3.exoplayer.ExoPlayer;
import androidx.media3.ui.PlayerView;
import androidx.media3.effect.OverlayEffect;

/**
 * Activity principale de l'application qui charge les sous-titres et les affiche sur l'écran.
 */
@UnstableApi public class MainActivity extends AppCompatActivity implements Player.Listener {

    private ExoPlayer player;
    private PlayerView playerView;

    private static final String TAG = "MainActivity";

    // Reference to the subtitle overlay for later updating
    private LibassSubtitleOverlay currentSubtitleOverlay;

    // Used to load the 'prototypelibass' library on application startup.
    static {
        System.loadLibrary("prototypelibass");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        ActivityMainBinding binding;
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        playerView = findViewById(R.id.player_view);

        initializePlayer();
    }

     /**
     * Initialize the ExoPlayer and set up the media item.
     * 1. Create Exo instance.
     * 2. Associate to playerView.
     * 3. Set MediaItem
     * 4. Add listener for player events.
     */
     // In initializePlayer() method
     private void initializePlayer() {
         if (player == null) {
             try {
                 // Create player
                 player = new ExoPlayer.Builder(this).build();
                 playerView.setPlayer(player);

                 // Add this player as a listener to receive video size changes
                 player.addListener(this);

                 // Initial default dimensions - these will be updated when video size is known
                 int defaultWidth = 1280;  // Standard default
                 int defaultHeight = 720;  // Standard default

                 // Create subtitle overlay with initial dimensions
                 currentSubtitleOverlay = new LibassSubtitleOverlay(this, defaultWidth, defaultHeight);

                 List<Effect> effects = new ArrayList<>();
                 effects.add(new OverlayEffect(ImmutableList.of(currentSubtitleOverlay)));

                 // Apply effects before setting media item
                 Log.d(TAG, "Applying video effects to player with initial dimensions: " +
                         defaultWidth + "x" + defaultHeight);
                 player.setVideoEffects(effects);

                 // Set media item and play
                 MediaItem mediaItem = MediaItem.fromUri(Uri.parse("asset:///sample2.mp4"));
                 player.setMediaItem(mediaItem);
                 player.prepare();
                 player.play();

                 Log.d(TAG, "Player setup complete");
             } catch (Exception e) {
                 Log.e(TAG, "Error initializing player: " + e.getMessage());
                 e.printStackTrace();
             }
         }
     }

    /**
     * Handle video size changes to update the subtitle overlay dimensions.
     * This is called when the player learns the actual dimensions of the video.
     */
    @Override
    public void onVideoSizeChanged(VideoSize videoSize) {
        if (videoSize.width > 0 && videoSize.height > 0) {
            Log.d(TAG, "Video size changed: " + videoSize.width + "x" + videoSize.height);

            // Update the subtitle overlay with actual video dimensions
            updateSubtitleOverlay(videoSize.width, videoSize.height);
        }
    }

    /**
     * Update the subtitle overlay with new dimensions.
     * This creates a new overlay with the proper dimensions and applies it to the player.
     */
    private void updateSubtitleOverlay(int width, int height) {
        try {
            // Release the old overlay
            if (currentSubtitleOverlay != null) {
                currentSubtitleOverlay.release();
            }

            // Create a new overlay with updated dimensions
            currentSubtitleOverlay = new LibassSubtitleOverlay(this, width, height);

            // Apply the new overlay to the player
            List<Effect> effects = new ArrayList<>();
            effects.add(new OverlayEffect(ImmutableList.of(currentSubtitleOverlay)));
            player.setVideoEffects(effects);

            Log.d(TAG, "Updated subtitle overlay dimensions to: " + width + "x" + height);
        } catch (Exception e) {
            Log.e(TAG, "Error updating subtitle overlay: " + e.getMessage());
            e.printStackTrace();
        }
    }

    /**
     * Release the player when the activity is destroyed.
     */
    @Override
    protected void onDestroy() {
        super.onDestroy();
        releasePlayer();
    }

    /**
     * Release the player when the activity is paused.
     */
    @Override
    protected void onPause() {
        super.onPause();
        if (player != null) {
            player.pause();
        }
    }

    /**
     * Release the ExoPlayer instance.
     */
    private void releasePlayer() {
        if (player != null) {
            //player.clearVideoEffect();

            player.release();
            player = null;
        }
    }

    /**
     * Resume playback when the activity is resumed.
     */
    @Override
    protected void onResume() {
        super.onResume();
        if (player != null) {
            player.play();
        }
    }

    /**
     * Handle player events.
     */
    @Override
    public void onPlaybackStateChanged(int state) {
        switch (state) {
            case Player.STATE_READY:
                Log.d(TAG, "Player is ready");
                break;
            case Player.STATE_BUFFERING:
                Log.d(TAG, "Player is buffering");
                break;
            case Player.STATE_ENDED:
                Log.d(TAG, "Playback has ended");
                break;
            case Player.STATE_IDLE:
                Log.d(TAG, "Player is idle");
                break;
        }
    }

    /**
     * Rendu d'un cadre de sous-titre à partir des ressources du gestionnaire d'actifs.
     *
     * @param assetManager le gestionnaire d'actifs permettant d'accéder aux fichiers dans les assets.
     * @param screenWidth la largeur de l'écran.
     * @param screenHeight la hauteur de l'écran.
     * @param timestamp le timestamp pour déterminer quel sous-titre doit être affiché.
     * @return un bitmap représentant le cadre du sous-titre.
     */
    public native Bitmap renderSubtitleFrame(AssetManager assetManager, int screenWidth, int screenHeight, int timestamp);
}