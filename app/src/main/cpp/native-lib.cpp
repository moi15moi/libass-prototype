#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <jni.h>
#include <string>
#include "ass/ass.h"
#include "ass/ass_types.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_prototypelibass_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    ASS_Library* lib = ass_library_init();
    int version = ass_library_version();
    __android_log_print(ANDROID_LOG_DEBUG, "TRACKERS", "Print la version de libass 0x%x", version);
    ass_library_done(lib);

    return env->NewStringUTF(hello.c_str());
}

void libass_msg_callback(int level, const char *fmt, va_list args, void *data) {
    __android_log_vprint(ANDROID_LOG_DEBUG, "LIBASS_LOG", fmt, args);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_prototypelibass_MainActivity_renderASSFile(JNIEnv *env, jobject thiz,
                                                            jobject asset_manager) {
    AAssetManager* g_assetManager = AAssetManager_fromJava(env, asset_manager);

    // Read the subtitle file
    const char* subtitleFilename = "subtitle.ass";
    std::unique_ptr<AAsset, decltype(&AAsset_close)> subtitleAsset(
            AAssetManager_open(g_assetManager, subtitleFilename, AASSET_MODE_BUFFER),
            AAsset_close
    );
    if (!subtitleAsset) {
        __android_log_print(ANDROID_LOG_DEBUG, "TRACKERS", "Failed to open subtitleAsset: %s", subtitleFilename);
        return;
    }

    int subtitleLength = AAsset_getLength(subtitleAsset.get());
    std::unique_ptr<char[]> subtitleBuffer = std::make_unique<char[]>(subtitleLength + 1);
    AAsset_read(subtitleAsset.get(), subtitleBuffer.get(), subtitleLength);

    // Read the font file
    const char* fontFilename = "C059-Roman.otf";
    std::unique_ptr<AAsset, decltype(&AAsset_close)> fontAsset(
            AAssetManager_open(g_assetManager, fontFilename, AASSET_MODE_BUFFER),
            AAsset_close
    );
    if (!fontAsset) {
        __android_log_print(ANDROID_LOG_DEBUG, "TRACKERS", "Failed to open fontAsset: %s", fontFilename);
        return;
    }

    size_t fontLength = AAsset_getLength(fontAsset.get());
    std::unique_ptr<char[]> fontBuffer = std::make_unique<char[]>(fontLength + 1);
    AAsset_read(fontAsset.get(), fontBuffer.get(), fontLength);



    std::unique_ptr<ASS_Library, decltype(&ass_library_done)> lib(
        ass_library_init(),
        ass_library_done
    );
    if (!lib) {
        __android_log_print(ANDROID_LOG_DEBUG, "TRACKERS", "ass_library_init is null");
        return;
    }

    std::unique_ptr<ASS_Renderer, decltype(&ass_renderer_done)> renderer(
            ass_renderer_init(lib.get()),
            ass_renderer_done
    );
    if (!renderer) {
        __android_log_print(ANDROID_LOG_DEBUG, "TRACKERS", "ass_renderer_init is null");
        return;
    }
    // TODO, set avec la taille de l'écran
    // Il faut appeler ces méthodes, car la doc de ass_renderer_init le mentionne
    ass_set_storage_size(renderer.get(), 100, 100);
    ass_set_frame_size(renderer.get(), 100, 100);
    ass_set_fonts(renderer.get(), nullptr, nullptr, ASS_FONTPROVIDER_AUTODETECT, nullptr, true);

    // Logger les messages de libass
    ass_set_message_cb(lib.get(), libass_msg_callback, nullptr);

    // Ajouter la font "C059-Roman.otf" à libass
    ass_add_font(lib.get(), fontFilename, fontBuffer.get(), fontLength);

    // Lire le fichier .ass avec libass
    std::unique_ptr<ASS_Track, decltype(&ass_free_track)> track(
            ass_read_memory(lib.get(), subtitleBuffer.get(), subtitleLength, nullptr),
            ass_free_track
    );
    if (!track) {
        __android_log_print(ANDROID_LOG_DEBUG, "TRACKERS", "ass_read_memory is null");
        return;
    }

    ASS_Image* ass_image = ass_render_frame(renderer.get(), track.get(), 0, nullptr);
    // TODO - Afficher le bitmap de ass_image
}