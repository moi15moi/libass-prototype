#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/bitmap.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include <string>
#include "ass/ass_types.h"
#include "ass/ass.h"

static ANativeWindow* nativeWindow = nullptr;

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
                                                            jobject surface,jobject asset_manager) {
    
    // If a native window already exists, release it
    if (nativeWindow) {
        ANativeWindow_release(nativeWindow);
    }
    // Create a new native window from the provided surface
    nativeWindow = ANativeWindow_fromSurface(env, surface);

    // Check if the native window was successfully created
    if (nativeWindow == nullptr) {
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

    // Lock the ANativeWindow
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(nativeWindow, &buffer, nullptr) == 0) {
        __android_log_print(ANDROID_LOG_INFO, "NativeWindow", "Successfully locked the native window");

        // TODO, set avec la taille de l'écran
        // Il faut appeler ces méthodes, car la doc de ass_renderer_init le mentionne

        // Set storage size and frame size dynamically avec la taille du window
        // J'utilise le half size car on get des out of bounds avec buffer.width et buffer.height
        int half_width = buffer.width / 2;
        int half_height = buffer.height / 2;
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
        // TODO - Afficher le bitmap de ass_image
        if (!ass_image) {
            __android_log_print(ANDROID_LOG_DEBUG, "TRACKERS", "ass_render_frame is null");
            return;
        }

        // Get the buffer bits
        uint32_t* windowPixels = (uint32_t*)buffer.bits;
        if (windowPixels == nullptr) {
            __android_log_print(ANDROID_LOG_ERROR, "NativeWindow", "Failed to get buffer bits");
            ANativeWindow_unlockAndPost(nativeWindow);
            return;
        }
        __android_log_print(ANDROID_LOG_INFO, "NativeWindow", "Buffer dimensions: width=%d, height=%d, stride=%d", buffer.width, buffer.height, buffer.stride);

        // Ensure buffer dimensions are valid
        if (buffer.width <= 0 || buffer.height <= 0 || buffer.stride <= 0) {
            __android_log_print(ANDROID_LOG_ERROR, "NativeWindow", "Invalid buffer dimensions");
            ANativeWindow_unlockAndPost(nativeWindow);
            return;
        }

        // Render the ASS_Image linked list
        ASS_Image* img = ass_image;
        while (img) {
            __android_log_print(ANDROID_LOG_INFO, "ASS_Image", "Rendering image at dst_x=%d, dst_y=%d, width=%d, height=%d", img->dst_x, img->dst_y, img->w, img->h);
            for (int y = 0; y < img->h; ++y) {
                for (int x = 0; x < img->w; ++x) {
                    int dst_x = img->dst_x + x;
                    int dst_y = img->dst_y + y;
                    if (dst_x < half_width && dst_y < half_height) {
                        uint8_t alpha = img->bitmap[y * img->stride + x];
                        uint32_t color = img->color & 0xFFFFFF; // RGB
                        uint32_t pixel = (alpha << 24) | color; // ARGB
                        int index = dst_y * (buffer.stride/2) + dst_x;
                        if (index < half_height * (buffer.stride/2)) {
                            windowPixels[index] = pixel;
                        } else {
                            __android_log_print(ANDROID_LOG_ERROR, "NativeWindow", "Buffer access out of bounds at dst_x=%d, dst_y=%d, index=%d", dst_x, dst_y, index);
                            ANativeWindow_unlockAndPost(nativeWindow);
                            return;
                        }
                    } else {
                        __android_log_print(ANDROID_LOG_ERROR, "NativeWindow", "Coordinates out of bounds at dst_x=%d, dst_y=%d", dst_x, dst_y);
                    }
                }
            }
            img = img->next;
        }

        // Unlock and post the ANativeWindow
        ANativeWindow_unlockAndPost(nativeWindow);
        __android_log_print(ANDROID_LOG_INFO, "NativeWindow", "Successfully unlocked and posted the native window");
    } else {
        __android_log_print(ANDROID_LOG_ERROR, "NativeWindow", "Failed to lock the native window");
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_prototypelibass_MainActivity_initNativeWindow(
        JNIEnv* env,
        jobject /* this */,
        jobject surface,
        jobject bitmap) {
    if (nativeWindow) {
        ANativeWindow_release(nativeWindow);
    }
    nativeWindow = ANativeWindow_fromSurface(env, surface);

    if (nativeWindow == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, "NativeWindow", "Failed to get native window from surface");
        return;
    }

    // Lock the ANativeWindow
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(nativeWindow, &buffer, nullptr) == 0) {
        __android_log_print(ANDROID_LOG_INFO, "NativeWindow", "Successfully locked the native window");

        // Lock the bitmap
        AndroidBitmapInfo bitmapInfo;
        void* bitmapPixels;
        if (AndroidBitmap_getInfo(env, bitmap, &bitmapInfo) < 0) {
            __android_log_print(ANDROID_LOG_ERROR, "NativeWindow", "Failed to get bitmap info");
            ANativeWindow_unlockAndPost(nativeWindow);
            return;
        }
        if (AndroidBitmap_lockPixels(env, bitmap, &bitmapPixels) < 0) {
            __android_log_print(ANDROID_LOG_ERROR, "NativeWindow", "Failed to lock bitmap pixels");
            ANativeWindow_unlockAndPost(nativeWindow);
            return;
        }

        // Copy the bitmap pixels to the ANativeWindow buffer
        uint32_t* windowPixels = (uint32_t*)buffer.bits;
        uint32_t* bitmapPixels32 = (uint32_t*)bitmapPixels;
        for (int y = 0; y < buffer.height && y < bitmapInfo.height; ++y) {
            for (int x = 0; x < buffer.width && x < bitmapInfo.width; ++x) {
                windowPixels[y * buffer.stride + x] = bitmapPixels32[y * bitmapInfo.stride / 4 + x];
            }
        }

        // Unlock the bitmap
        AndroidBitmap_unlockPixels(env, bitmap);

        // Unlock and post the ANativeWindow
        ANativeWindow_unlockAndPost(nativeWindow);
        __android_log_print(ANDROID_LOG_INFO, "NativeWindow", "Successfully unlocked and posted the native window");
    } else {
        __android_log_print(ANDROID_LOG_ERROR, "NativeWindow", "Failed to lock the native window");
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_prototypelibass_MainActivity_releaseNativeWindow(
        JNIEnv* env,
        jobject /* this */) {
    if (nativeWindow) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = nullptr;
    }
}