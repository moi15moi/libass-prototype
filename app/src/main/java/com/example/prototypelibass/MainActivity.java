package com.example.prototypelibass;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.widget.ImageView;

import androidx.appcompat.app.AppCompatActivity;

import com.example.prototypelibass.databinding.ActivityMainBinding;

/**
 * Activity principale de l'application qui charge les sous-titres et les affiche sur l'écran.
 */
public class MainActivity extends AppCompatActivity {

    // Used to load the 'prototypelibass' library on application startup.
    static {
        System.loadLibrary("prototypelibass");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Récupération des dimensions de l'écran
        DisplayMetrics displayMetrics = getDisplayMetrics();

        // Tentative de rendu du sous-titre
        Bitmap subtitleBitmap = renderSubtitleFrame(getAssets(), displayMetrics.widthPixels, displayMetrics.heightPixels, 0);
        displaySubtitle(subtitleBitmap);
    }

    /**
     * Récupère les dimensions de l'écran actuel.
     *
     * @return un objet DisplayMetrics contenant la largeur et la hauteur de l'écran.
     */
    private DisplayMetrics getDisplayMetrics() {
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        Log.d("SCREEN_DIMENSIONS", "Width: " + displayMetrics.widthPixels + ", Height: " + displayMetrics.heightPixels);
        return displayMetrics;
    }

    /**
     * Affiche l'image du sous-titre dans un ImageView.
     *
     * @param subtitleBitmap le bitmap à afficher dans l'ImageView.
     */
    private void displaySubtitle(Bitmap subtitleBitmap) {
        if (subtitleBitmap != null) {
            ImageView subtitleView = findViewById(R.id.subtitle_view);
            subtitleView.setImageBitmap(subtitleBitmap);
            Log.d("SUBTITLE_RENDER", "Bitmap width: " + subtitleBitmap.getWidth() +
                    ", height: " + subtitleBitmap.getHeight());
        } else {
            Log.e("SUBTITLE_RENDER", "Le bitmap du sous-titre est null");
        }
    }

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