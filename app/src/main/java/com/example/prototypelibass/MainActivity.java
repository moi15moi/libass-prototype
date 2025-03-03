package com.example.prototypelibass;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;
import java.util.ArrayList;
import java.util.List;

// ImageView
import android.view.ViewTreeObserver;
import android.widget.ImageView;

import androidx.annotation.OptIn;
import androidx.appcompat.app.AppCompatActivity;

import com.example.prototypelibass.databinding.ActivityMainBinding;
import com.google.common.collect.ImmutableList;

// Media3/Exoplayer Classes
import androidx.media3.common.MediaItem;
import androidx.media3.common.Player;
import androidx.media3.common.util.UnstableApi;
import androidx.media3.exoplayer.ExoPlayer;
import androidx.media3.ui.PlayerView;
import androidx.media3.effect.TextureOverlay;
import androidx.media3.effect.OverlayEffect;
import androidx.media3.exoplayer.source.DefaultMediaSourceFactory;

/**
 * Activity principale de l'application qui charge les sous-titres et les affiche sur l'écran.
 */
@UnstableApi public class MainActivity extends AppCompatActivity implements Player.Listener {

    private ExoPlayer player;
    private PlayerView playerView;
    //private ImageView subtitleView;
    private static final String TAG = "MainActivity";
    private LibassSubtitleOverlay libassSubtitleOverlay;

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

        // Récupérer l'ImageView
        //subtitleView = findViewById(R.id.subtitle_view);

        // Initialize ExoPlayer
        playerView = findViewById(R.id.player_view);

        initializePlayer();

        /*
        // Utiliser un ViewTreeObserver pour récupérer les dimensions après la mise en page
        subtitleView.getViewTreeObserver().addOnPreDrawListener(new ViewTreeObserver.OnPreDrawListener() {
            @Override
            public boolean onPreDraw() {
                // Récupérer les dimensions de l'ImageView
                int imageViewWidth = subtitleView.getWidth();
                int imageViewHeight = subtitleView.getHeight();

                // Tentative de rendu du sous-titre
                Bitmap subtitleBitmap = renderSubtitleFrame(getAssets(), imageViewWidth, imageViewHeight, 0);
                displaySubtitle(subtitleView, subtitleBitmap);

                // Retirer le listener pour éviter de le déclencher à chaque dessin
                subtitleView.getViewTreeObserver().removeOnPreDrawListener(this);
                return true;
            }
        });
        */
    }

     /**
     * Initialize the ExoPlayer and set up the media item.
     * 1. Create Exo instance.
     * 2. Associate to playerView.
     * 3. Set MediaItem
     * 4. Add listener for player events.
     */
    private void initializePlayer() {
        if (player == null) {
            try{

                // Create an ExoPlayer instance
                player = new ExoPlayer.Builder(this).build();
                
                // Set the player to the PlayerView
                playerView.setPlayer(player);
                
                // Add listener for player events
                player.addListener(this);
                
                // Create a MediaItem from a sample video in your raw resources
                // You can change this to the path of your sample video
                MediaItem mediaItem = MediaItem.fromUri(Uri.parse("asset:///videoSamplemp4.mp4"));

                // Set the media item to be played
                player.setMediaItem(mediaItem);
                
                // Prepare the player
                player.prepare();

                player.addListener(new Player.Listener() {
                    @OptIn(markerClass = UnstableApi.class) @Override
                    public void onPlaybackStateChanged(int state) {
                        if (state == Player.STATE_READY) {
                            // Video is ready,
                            setupSubtitleOverlay();


                            // Remove temp Listener -- Memory Leak
                            player.removeListener(this);

                        }
                    }
                });
                
                // Start playback automatically
                player.play();

                } catch (Exception e) {
                    Log.e(TAG, "Error initializing player: " + e.getMessage());
                    Toast.makeText(this, "Error initializing player", Toast.LENGTH_SHORT).show();
                } 
        }
    }

    /**
     * Setup Subtitle Overlay when player is ready.
     * once player is ready we can grab dimensions.
     */
    @OptIn(markerClass = UnstableApi.class) private void setupSubtitleOverlay() {
        try{
            int videoWidth = player.getVideoFormat().width;
            int videoHeight = player.getVideoFormat().height;

            // Create a new instance of the LibassSubtitleOverlay
            LibassSubtitleOverlay subtitleOverlay = new LibassSubtitleOverlay(this, videoWidth, videoHeight);

            // Create a list of Overlays to apply
            List<TextureOverlay> overlays = new ArrayList<>();
            overlays.add(subtitleOverlay);

            // Create a new instance of OverlayEffect
            OverlayEffect overlayEffect = new OverlayEffect(ImmutableList.copyOf(overlays));

            // Apply effect to the player
            player.setVideoEffects(ImmutableList.of(overlayEffect));

            Log.d(TAG, "Subitle overlay setup successfully with videosize: " + videoWidth + "x" + videoHeight);
        }
        catch (Exception e){
            Log.e(TAG, "Error setting up subtitle overlay: " + e.getMessage());
            Toast.makeText(this, "Error setting up subtitle overlay", Toast.LENGTH_SHORT).show();
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
    // ---------------------------------------------------------------------------------------------

    /**
     * Affiche l'image du sous-titre dans un ImageView.
     *
     * @param subtitleView l'ImageView dans lequel afficher le bitmap.
     * @param subtitleBitmap le bitmap à afficher dans l'ImageView.
     */
    /* 
    private void displaySubtitle(ImageView subtitleView, Bitmap subtitleBitmap) {
        if (subtitleBitmap != null) {
            subtitleView.setImageBitmap(subtitleBitmap);
            Log.d("SUBTITLE_RENDER", "Bitmap width: " + subtitleBitmap.getWidth() +
                    ", height: " + subtitleBitmap.getHeight());
        } else {
            Log.e("SUBTITLE_RENDER", "Le bitmap du sous-titre est null");
        }
    }
    */

    /**
     * A native method that is implemented by the 'prototypelibass' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

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