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
    AAsset* subtitleAsset = AAssetManager_open(g_assetManager, subtitleFilename, AASSET_MODE_BUFFER);
    if (!subtitleAsset) {
        __android_log_print(ANDROID_LOG_DEBUG, "TRACKERS", "Failed to open subtitleAsset: %s", subtitleFilename);
        return;
    }

    int subtitleLength = AAsset_getLength(subtitleAsset);
    char* subtitleBuffer = (char*) malloc(subtitleLength + 1);
    AAsset_read(subtitleAsset, subtitleBuffer, subtitleLength);
    AAsset_close(subtitleAsset);

    // Read the font file
    const char* fontFilename = "C059-Roman.otf";
    AAsset* fontAsset = AAssetManager_open(g_assetManager, fontFilename, AASSET_MODE_BUFFER);
    if (!fontAsset) {
        __android_log_print(ANDROID_LOG_DEBUG, "TRACKERS", "Failed to open fontAsset: %s", fontFilename);
        return;
    }

    size_t fontLength = AAsset_getLength(fontAsset);
    char* fontBuffer = (char*) malloc(fontLength + 1);
    AAsset_read(fontAsset, fontBuffer, fontLength);
    AAsset_close(fontAsset);



    ASS_Library* lib = ass_library_init();
    if (!lib) {
        __android_log_print(ANDROID_LOG_DEBUG, "TRACKERS", "ass_library_init is null");
        return;
    }

    ASS_Renderer* renderer = ass_renderer_init(lib);
    if (!renderer) {
        __android_log_print(ANDROID_LOG_DEBUG, "TRACKERS", "ass_renderer_init is null");
        return;
    }
    // TODO, set avec la taille de l'écran
    // Il faut appeler ces méthodes, car la doc de ass_renderer_init le mentionne
    ass_set_storage_size(renderer, 100, 100);
    ass_set_frame_size(renderer, 100, 100);
    ass_set_fonts(renderer, nullptr, nullptr, ASS_FONTPROVIDER_AUTODETECT, nullptr, true);

    // Logger les messages de libass
    ass_set_message_cb(lib, libass_msg_callback, nullptr);

    // Ajouter la font "C059-Roman.otf" à libass
    ass_add_font(lib, fontFilename, fontBuffer, fontLength);

    // Lire le fichier .ass avec libass
    ASS_Track* track = ass_read_memory(lib, subtitleBuffer, subtitleLength, nullptr);
    if (!track) {
        __android_log_print(ANDROID_LOG_DEBUG, "TRACKERS", "ass_read_memory is null");
        return;
    }

    ASS_Image* ass_image = ass_render_frame(renderer, track, 0, nullptr);
    // TODO - Afficher le bitmap de ass_image

    ass_free_track(track);
    ass_renderer_done(renderer);
    ass_library_done(lib);
    free(fontBuffer);
    free(subtitleBuffer);
}