#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <jni.h>
#include <string>
#include <android/bitmap.h>
#include "ass/ass.h"
#include "ass/ass_types.h"

static void draw_ass_rgba(uint8_t *dst, ptrdiff_t dst_stride,
                          const uint8_t *src, ptrdiff_t src_stride,
                          int w, int h, uint32_t color)
{
    const unsigned int a = 0xff - (color & 0xff);
    const unsigned int b = (color >> 24) & 0xff;
    const unsigned int g = (color >> 16) & 0xff;
    const unsigned int r = (color >>  8) & 0xff;

    for (int y = 0; y < h; y++) {
        auto *dstrow = (uint32_t *) dst;
        for (int x = 0; x < w; x++) {
            const unsigned int v = src[x];
            unsigned int aa = a * v;
            uint32_t dstpix = dstrow[x];
            unsigned int dstb =  dstpix & 0xFF;
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

void libass_msg_callback(int level, const char *fmt, va_list args, void *data) {
    __android_log_vprint(ANDROID_LOG_DEBUG, "LIBASS_LOG", fmt, args);
}

std::unique_ptr<char[]> read_asset_file(AAssetManager* assetManager, const char* filename, int& length) {
    std::unique_ptr<AAsset, decltype(&AAsset_close)> asset(
            AAssetManager_open(assetManager, filename, AASSET_MODE_BUFFER),
            AAsset_close
    );
    if (!asset) {
        __android_log_print(ANDROID_LOG_ERROR, "TRACKERS", "Failed to open asset: %s", filename);
        return nullptr;
    }

    length = AAsset_getLength(asset.get());
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(length + 1);
    AAsset_read(asset.get(), buffer.get(), length);

    return buffer;
}

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

extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_prototypelibass_MainActivity_renderSubtitleFrame(
        JNIEnv *env,
        jobject thiz,
        jobject asset_manager,
        jint screenWidth,
        jint screenHeight,
        jint timestamp
) {
    AAssetManager* g_assetManager = AAssetManager_fromJava(env, asset_manager);

    // Lire le fichier de sous-titres et de police avec la fonction générique
    int subtitleLength, fontLength;
    auto subtitleBuffer = read_asset_file(g_assetManager, "subtitle.ass", subtitleLength);
    auto fontBuffer = read_asset_file(g_assetManager, "C059-Roman.otf", fontLength);
    if (!subtitleBuffer || !fontBuffer) return nullptr;

    std::unique_ptr<ASS_Library, decltype(&ass_library_done)> lib(
            ass_library_init(),
            ass_library_done
    );
    if (!lib) {
        __android_log_print(ANDROID_LOG_ERROR, "TRACKERS", "ass_library_init failed");
        return nullptr;
    }

    ass_set_message_cb(lib.get(), libass_msg_callback, nullptr);

    std::unique_ptr<ASS_Renderer, decltype(&ass_renderer_done)> renderer(
            ass_renderer_init(lib.get()),
            ass_renderer_done
    );
    if (!renderer) {
        __android_log_print(ANDROID_LOG_ERROR, "TRACKERS", "ass_renderer_init failed");
        return nullptr;
    }

    ass_set_storage_size(renderer.get(), screenWidth, screenHeight);
    ass_set_frame_size(renderer.get(), screenWidth, screenHeight);
    ass_set_fonts(renderer.get(), nullptr, nullptr, ASS_FONTPROVIDER_AUTODETECT, nullptr, true);
    ass_add_font(lib.get(), "C059-Roman.otf", fontBuffer.get(), fontLength);

    std::unique_ptr<ASS_Track, decltype(&ass_free_track)> track(
            ass_read_memory(lib.get(), subtitleBuffer.get(), subtitleLength, nullptr),
            ass_free_track
    );
    if (!track) {
        __android_log_print(ANDROID_LOG_ERROR, "TRACKERS", "ass_read_memory failed");
        return nullptr;
    }

    ASS_Image* ass_image = ass_render_frame(renderer.get(), track.get(), timestamp, nullptr);
    if (!ass_image) {
        __android_log_print(ANDROID_LOG_ERROR, "SUBTITLE_RENDER", "No subtitle image rendered");
        return nullptr;
    }

    jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
    jmethodID createBitmapMethod = env->GetStaticMethodID(
            bitmapClass,
            "createBitmap",
            "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;"
    );

    jobject bitmap = env->CallStaticObjectMethod(
            bitmapClass,
            createBitmapMethod,
            screenWidth,
            screenHeight,
            env->GetStaticObjectField(
                    env->FindClass("android/graphics/Bitmap$Config"),
                    env->GetStaticFieldID(env->FindClass("android/graphics/Bitmap$Config"), "ARGB_8888", "Landroid/graphics/Bitmap$Config;")
            )
    );

    AndroidBitmapInfo bitmapInfo;
    void* pixels = nullptr;

    if (AndroidBitmap_getInfo(env, bitmap, &bitmapInfo) < 0 || AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, "SUBTITLE_RENDER", "Unable to lock bitmap pixels");
        return nullptr;
    }

    memset(pixels, 0, bitmapInfo.stride * bitmapInfo.height);

    for (ASS_Image* img = ass_image; img; img = img->next) {
        uint8_t* dst = reinterpret_cast<uint8_t *>(pixels) + img->dst_y * bitmapInfo.stride + img->dst_x * 4;
        draw_ass_rgba(dst, (ptrdiff_t)bitmapInfo.stride, img->bitmap, img->stride, img->w, img->h, img->color);
    }

    AndroidBitmap_unlockPixels(env, bitmap);

    return bitmap;
}
