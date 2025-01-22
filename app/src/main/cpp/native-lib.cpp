#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/bitmap.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include <string>
#include "ass/ass.h"
#include "ass/ass_types.h"

// TODO - Voir si utile
//static ANativeWindow* nativeWindow = nullptr;

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


// Provient de mpv: https://github.com/mpv-player/mpv/blob/bc96b23ef686d29efe95d54a4fd1836c177d7a61/sub/draw_bmp.c#L295-L338
static void draw_ass_rgba(uint8_t *dst, ptrdiff_t dst_stride,
                          uint8_t *src, ptrdiff_t src_stride,
                          int w, int h, uint32_t color)
{
    const unsigned int r = (color >> 24) & 0xff;
    const unsigned int g = (color >> 16) & 0xff;
    const unsigned int b = (color >>  8) & 0xff;
    const unsigned int a = 0xff - (color & 0xff);

    for (int y = 0; y < h; y++) {
        uint32_t *dstrow = (uint32_t *) dst;
        for (int x = 0; x < w; x++) {
            const unsigned int v = src[x];
            unsigned int aa = a * v;
            uint32_t dstpix = dstrow[x];
            unsigned int dstb =  dstpix        & 0xFF;
            unsigned int dstg = (dstpix >>  8) & 0xFF;
            unsigned int dstr = (dstpix >> 16) & 0xFF;
            unsigned int dsta = (dstpix >> 24) & 0xFF;
            dstb = (v * b * a   + dstb * (255 * 255 - aa)) / (255 * 255);
            dstg = (v * g * a   + dstg * (255 * 255 - aa)) / (255 * 255);
            dstr = (v * r * a   + dstr * (255 * 255 - aa)) / (255 * 255);
            dsta = (aa * 255    + dsta * (255 * 255 - aa)) / (255 * 255);
            dstrow[x] = dstb | (dstg << 8) | (dstr << 16) | (dsta << 24);
        }
        dst += dst_stride;
        src += src_stride;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_prototypelibass_MainActivity_renderASSFile(JNIEnv *env, jobject thiz, 
                                                            jobject surface,jobject asset_manager) {
    __android_log_print(ANDROID_LOG_ERROR, "NativeWindow", "renderASSFile is called");

    // If a native window already exists, release it
    /*if (nativeWindow) {
        ANativeWindow_release(nativeWindow);
    }*/
    /*if (!nativeWindow) {
        nativeWindow = ANativeWindow_fromSurface(env, surface);
    }*/
    std::unique_ptr<ANativeWindow, decltype(&ANativeWindow_release)> nativeWindow(
            ANativeWindow_fromSurface(env, surface),
            ANativeWindow_release
    );
    if (!nativeWindow) {
        __android_log_print(ANDROID_LOG_ERROR, "NativeWindow", "Failed to get native window from surface");
        return;
    }
    
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

    ANativeWindow_Buffer buffer;
    int32_t result = ANativeWindow_lock(nativeWindow.get(), &buffer, NULL);
    if (result != 0) {
        __android_log_print(ANDROID_LOG_ERROR, "NativeWindow", "Failed to lock the native window. Result: %d", result);
        return;
    }

   if (buffer.format != AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM) {
        __android_log_print(ANDROID_LOG_INFO, "NativeWindow", "The format isn't AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM. It is %d", buffer.format);
        return;
    }

    // TODO, set avec la taille de l'écran
    // Il faut appeler ces méthodes, car la doc de ass_renderer_init le mentionne
    // Set storage size and frame size dynamically avec la taille du window
    // J'utilise le half size car on get des out of bounds avec buffer.width et buffer.height - ToDo faudrait voir la meilleur facon de faire ceci
    int half_width = buffer.width;
    int half_height = buffer.height;
    ass_set_storage_size(renderer.get(), half_width, half_height);
    ass_set_frame_size(renderer.get(), half_width, half_height);
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
    if (!ass_image) {
        __android_log_print(ANDROID_LOG_DEBUG, "TRACKERS", "ass_render_frame is null");
        return;
    }

    // Get the buffer bits
    uint32_t* windowPixels = (uint32_t*)buffer.bits;
    if (!windowPixels) {
        __android_log_print(ANDROID_LOG_ERROR, "NativeWindow", "Failed to get buffer bits");
        ANativeWindow_unlockAndPost(nativeWindow.get());
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, "NativeWindow", "Buffer dimensions: width=%d, height=%d, stride=%d", buffer.width, buffer.height, buffer.stride);

    // Ensure buffer dimensions are valid
    /*if (buffer.width <= 0 || buffer.height <= 0 || buffer.stride <= 0) {
        __android_log_print(ANDROID_LOG_ERROR, "NativeWindow", "Invalid buffer dimensions");
        ANativeWindow_unlockAndPost(nativeWindow.get());
        return;
    }*/

    // Render the ASS_Image linked list
    ASS_Image* img = ass_image;
    while (img) {
        __android_log_print(ANDROID_LOG_INFO, "ASS_Image", "Rendering image at dst_x=%d, dst_y=%d, width=%d, height=%d", img->dst_x, img->dst_y, img->w, img->h);

        uint8_t* dst = reinterpret_cast<uint8_t *>(windowPixels) + img->dst_y * buffer.stride + img->dst_x * 4;
        draw_ass_rgba(dst, buffer.stride, img->bitmap, img->stride, img->w, img->h, img->color);

        img = img->next;
    }

    // Setter l'activité totalement en rouge (test)
    /*for (int y = 0; y < buffer.height; y++) {
        for (int x = 0; x < buffer.width; x++) {
            int A = 255;
            int R = 255;
            int G = 0;
            int B = 0;
            windowPixels[x] = (R & 0xff) << 24 | (G & 0xff) << 16 | (B & 0xff) << 8 | (A & 0xff);

        }
        windowPixels += buffer.stride;
    }*/


    // Unlock and post the ANativeWindow
    ANativeWindow_unlockAndPost(nativeWindow.get());
    __android_log_print(ANDROID_LOG_INFO, "NativeWindow", "Successfully unlocked and posted the native window");
}



extern "C" JNIEXPORT void JNICALL
Java_com_example_prototypelibass_MainActivity_releaseNativeWindow(
        JNIEnv* env,
        jobject /* this */) {
    // TODO - Voir si utile
    /*if (nativeWindow) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = nullptr;
    }*/
}