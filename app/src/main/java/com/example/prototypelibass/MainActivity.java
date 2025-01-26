package com.example.prototypelibass;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.util.Log;
import android.view.ViewTreeObserver;
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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        ActivityMainBinding binding;
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Récupérer l'ImageView
        ImageView subtitleView = findViewById(R.id.subtitle_view);

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
    }

    /**
     * Affiche l'image du sous-titre dans un ImageView.
     *
     * @param subtitleView l'ImageView dans lequel afficher le bitmap.
     * @param subtitleBitmap le bitmap à afficher dans l'ImageView.
     */
    private void displaySubtitle(ImageView subtitleView, Bitmap subtitleBitmap) {
        if (subtitleBitmap != null) {
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