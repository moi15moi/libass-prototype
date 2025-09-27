#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <jni.h>
#include <string>
#include <android/bitmap.h>
#include "ass/ass.h"
#include "ass/ass_types.h"

#include <EGL/egl.h>   // On Android you already have EGL context active
#include <GLES3/gl3.h>
#include "libplacebo/log.h"
#include "libplacebo/renderer.h"
#include "libplacebo/opengl.h"
#include "libplacebo/utils/upload.h"

#define LOG_TAG "SubtitleRenderer"


typedef struct libplacebo_context {
    pl_log pllog;
    pl_opengl plgl;
    pl_renderer renderer;
    pl_fmt format_r8;
    pl_fmt format_rgba8;
} LibplaceboContext;

jobject nativeAssRenderFrame(JNIEnv* env, jclass clazz,
                             jlong context,
                             jlong render,
                             jlong track,
                             jlong time,
                             jboolean onlyAlpha,
                             jint width,
                             jint height) {
    if (!context) return NULL;

    LibplaceboContext* ctx = (LibplaceboContext*)context;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int changed;
    ASS_Image *image = ass_render_frame((ASS_Renderer *) render, (ASS_Track *) track, time, &changed);
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                        (end.tv_nsec - start.tv_nsec) / 1000000.0;

    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "ass_render_frame took %.3f ms", elapsed_ms);
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "ass_render_frame resolution %ix%i", width, height);

    if (image == NULL) {
        return NULL;
    }

    struct pl_tex_params out_params = {
            .w = image->w,
            .h = image->h,
            .format = ctx->format_r8,
            .renderable = true,
            .host_readable = true,
            .host_writable = true,
            .blit_dst = true,
            .sampleable = true,
    };

    EGLDisplay d = eglGetCurrentDisplay();
    EGLContext c = eglGetCurrentContext();
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "egl current display=%p context=%p", (void*)d, (void*)c);

    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Before pl_tex_create");
    pl_tex src_tex = pl_tex_create(ctx->plgl->gpu, &out_params);
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "After pl_tex_create");

    if (!src_tex) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Failed to create output texture");
        return NULL;
    }

    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Ass image is %ix%i", image->w, image->h);

    // TODO Utiliser posix_memalign
    bool is_ok = pl_tex_upload(ctx->plgl->gpu, &(struct pl_tex_transfer_params) {
            .tex        = src_tex,
            .rc         = { .x1 = image->w, .y1 = image->h, },
            .row_pitch  = static_cast<size_t>(image->stride),
            .ptr        = image->bitmap,
    });
    if (!is_ok) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Failed to pl_tex_upload");
        pl_tex_destroy(ctx->plgl->gpu, &src_tex);
        return NULL;
    }

    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "pl_tex_upload finished");

    struct pl_overlay_part part = {
            .src = { static_cast<float>(image->dst_x), static_cast<float>(image->dst_y), static_cast<float>(image->dst_x + image->w), static_cast<float>(image->dst_y + image->h) },
            .dst = { static_cast<float>(image->dst_x), static_cast<float>(image->dst_y), static_cast<float>(image->dst_x + image->w), static_cast<float>(image->dst_y + image->h) },
            .color = {
                    (image->color >> 24) / 255.0f,
                    ((image->color >> 16) & 0xFF) / 255.0f,
                    ((image->color >> 8) & 0xFF) / 255.0f,
                    (255 - (image->color & 0xFF)) / 255.0f,
            }
    };

    struct pl_overlay overlayl = {
            .tex = src_tex,
            .parts = &part,
            .num_parts = 1,
            .mode = PL_OVERLAY_MONOCHROME,
            .color = {
                    .primaries = PL_COLOR_PRIM_BT_709,
                    .transfer = PL_COLOR_TRC_SRGB,
            },
            .repr = {
                    .alpha = PL_ALPHA_INDEPENDENT
            }
    };


    struct pl_tex_params dst_params = {
            .w = width,
            .h = height,
            .format = ctx->format_rgba8,
            .renderable = true,
            .host_readable = true,
            .host_writable = true,
            .blit_dst = true,
            .sampleable = true,
    };
    pl_tex dst_tex = pl_tex_create(ctx->plgl->gpu, &dst_params);
    if (!dst_tex) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Failed to create output texture");
        return NULL;
    }
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "pl_tex_create finished");

    unsigned int target_type, iformat, fbo;
    GLuint tex_id = pl_opengl_unwrap(ctx->plgl->gpu, dst_tex, &target_type, (int *)&iformat, &fbo);

    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Libplacebo output GL texture ID: %u target_type=%u iformat=%u fbo=%u", tex_id, target_type, iformat, fbo);

    struct pl_frame target = {
            .repr = pl_color_repr_rgb,
            .num_planes = 1,
            .planes[0] = {
                    .texture = dst_tex,
                    .components = 4,
                    .component_mapping = {0, 1, 2, 3},
            },
            .overlays = &overlayl,
            .num_overlays = 1,
    };

    is_ok = pl_render_image(ctx->renderer, NULL, &target, &pl_render_default_params);
    if (!is_ok) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Failed to pl_render_image");
        pl_tex_destroy(ctx->plgl->gpu, &src_tex);
        return NULL;
    }
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "pl_render_image finished");

    GLenum erro = glGetError();
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Error libplacebo is %i", erro);

    jclass integerClass = env->FindClass("java/lang/Integer");
    jmethodID constructor = env->GetMethodID(integerClass, "<init>", "(I)V");
    return env->NewObject(integerClass, constructor, (jint) tex_id);
}



// Fonction de rendu RGBA adaptée de mpv-player
// Mélange les pixels des sous-titres (src) avec le buffer de destination (dst)
// en prenant en compte le canal alpha et la couleur spécifique des sous-titres
// Provient de mpv: https://github.com/mpv-player/mpv/blob/bc96b23ef686d29efe95d54a4fd1836c177d7a61/sub/draw_bmp.c#L295-L338
static void draw_ass_rgba(uint8_t *dst, ptrdiff_t dst_stride,
                          const uint8_t *src, ptrdiff_t src_stride,
                          int w, int h, uint32_t color) {
    // 1. Extraction CORRECTE du format RGBA de libass
    const unsigned int ass_r = (color >> 24) & 0xff; // Rouge (bits 24-31)
    const unsigned int ass_g = (color >> 16) & 0xff; // Vert (bits 16-23)
    const unsigned int ass_b = (color >> 8) & 0xff; // Bleu (bits 8-15)
    const unsigned int ass_a = 0xff - (color & 0xff); // Alpha inversé (ASS utilise 0 = opaque)

    // Parcours de tous les pixels de l'image
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const unsigned int v = src[x]; // Alpha du sous-titre

            // 2. Extraction des composantes DESTINATION (ANDROID_BITMAP_FORMAT_RGBA_8888.)
            unsigned int dst_r = dst[x * 4 + 0];
            unsigned int dst_g = dst[x * 4 + 1];
            unsigned int dst_b = dst[x * 4 + 2];
            unsigned int dst_a = dst[x * 4 + 3];

            // 3. Calcul du blending alpha
            unsigned int aa = ass_a * v;
            unsigned int blend_factor = 255 * 255 - aa;

            // Mélange des canaux (ASS + Destination)
            unsigned int out_r = (v * ass_r * ass_a + dst_r * blend_factor) / (255 * 255);
            unsigned int out_g = (v * ass_g * ass_a + dst_g * blend_factor) / (255 * 255);
            unsigned int out_b = (v * ass_b * ass_a + dst_b * blend_factor) / (255 * 255);
            unsigned int out_a = (aa * 255 + dst_a * blend_factor) / (255 * 255);

            // 4. Réassemblage en RBGA pour Android
            dst[x * 4 + 0] = out_r;
            dst[x * 4 + 1] = out_g;
            dst[x * 4 + 2] = out_b;
            dst[x * 4 + 3] = out_a;
        }
        dst += dst_stride;
        src += src_stride;
    }
}

// Callback de libass pour les messages d'erreur
void libass_msg_callback(int level, const char *fmt, va_list args, void *data) {
    __android_log_vprint(ANDROID_LOG_DEBUG, "LIBASS_LOG", fmt, args);
}

// Lit un fichier depuis les assets Android
std::unique_ptr<char[]>
read_asset_file(AAssetManager *assetManager, const char *filename, int &length) {
    // Ouverture de l'asset avec gestion automatique de fermeture
    std::unique_ptr<AAsset, decltype(&AAsset_close)> asset(
            AAssetManager_open(assetManager, filename, AASSET_MODE_BUFFER),
            AAsset_close
    );

    if (!asset) {
        __android_log_print(ANDROID_LOG_ERROR, "TRACKERS", "Failed to open asset: %s", filename);
        return nullptr;
    }

    // Lecture de contenu dans un buffer
    length = AAsset_getLength(asset.get());
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(length + 1);
    AAsset_read(asset.get(), buffer.get(), length);

    return buffer;
}

// Méthode JNI de démonstration
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_prototypelibass_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    // Initialisation minimale de libass pour vérifier la version
    ASS_Library *lib = ass_library_init();
    int version = ass_library_version();
    __android_log_print(ANDROID_LOG_DEBUG, "TRACKERS", "Print la version de libass 0x%x", version);
    ass_library_done(lib);

    return env->NewStringUTF(hello.c_str());
}

// Méthode principale de rendu des sous-titres
extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_prototypelibass_MainActivity_renderSubtitleFrame(
        JNIEnv *env,
        jobject thiz,
        jobject asset_manager,
        jint screenWidth,
        jint screenHeight,
        jint timestamp,
        jlong context
) {
    // Récupération de l'AssetManager natif
    AAssetManager *g_assetManager = AAssetManager_fromJava(env, asset_manager);

    // Lire le fichier de sous-titres et de police avec la fonction générique
    int subtitleLength, fontLength;
    auto subtitleBuffer = read_asset_file(g_assetManager, "subtitle.ass", subtitleLength);
    auto fontBuffer = read_asset_file(g_assetManager, "C059-Roman.otf", fontLength);
    if (!subtitleBuffer || !fontBuffer) return nullptr;

    // Initialisation de libass avec gestion automatique de mémoire
    std::unique_ptr<ASS_Library, decltype(&ass_library_done)> lib(
            ass_library_init(),
            ass_library_done
    );
    if (!lib) {
        __android_log_print(ANDROID_LOG_ERROR, "TRACKERS", "ass_library_init failed");
        return nullptr;
    }

    ass_set_message_cb(lib.get(), libass_msg_callback, nullptr);

    // Création du renderer
    std::unique_ptr<ASS_Renderer, decltype(&ass_renderer_done)> renderer(
            ass_renderer_init(lib.get()),
            ass_renderer_done
    );
    if (!renderer) {
        __android_log_print(ANDROID_LOG_ERROR, "TRACKERS", "ass_renderer_init failed");
        return nullptr;
    }

    // Configuration du renderer
    ass_set_storage_size(renderer.get(), screenWidth, screenHeight);
    ass_set_frame_size(renderer.get(), screenWidth, screenHeight);
    ass_set_fonts(renderer.get(), nullptr, nullptr, ASS_FONTPROVIDER_AUTODETECT, nullptr, true);
    ass_add_font(lib.get(), "C059-Roman.otf", fontBuffer.get(), fontLength);

    // Chargement de la piste de sous-titres
    std::unique_ptr<ASS_Track, decltype(&ass_free_track)> track(
            ass_read_memory(lib.get(), subtitleBuffer.get(), subtitleLength, nullptr),
            ass_free_track
    );
    if (!track) {
        __android_log_print(ANDROID_LOG_ERROR, "TRACKERS", "ass_read_memory failed");
        return nullptr;
    }

    // Rendu de l'image à un timestamp donné
    ASS_Image *ass_image = ass_render_frame(renderer.get(), track.get(), timestamp, nullptr);
    if (!ass_image) {
        __android_log_print(ANDROID_LOG_ERROR, "SUBTITLE_RENDER", "No subtitle image rendered");
        return nullptr;
    }


    return nativeAssRenderFrame(env, (jclass)thiz, context, (jlong)renderer.get(), (jlong)track.get(), timestamp, true ? JNI_TRUE : JNI_FALSE, (jint)screenWidth, (jint)screenHeight);
}



static void log_cb(void *priv, enum pl_log_level level, const char *msg)
{
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "placebo - %s", msg);
}

void checkGLError(const char* context) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "OpenGL error at %s: 0x%04x", context, err);
    }
}



extern "C"
JNIEXPORT jlong JNICALL Java_com_example_prototypelibass_MainActivity_nativeInitializeLibplacebo(JNIEnv* env, jclass clazz) {
    EGLDisplay display = eglGetCurrentDisplay();
    EGLContext context = eglGetCurrentContext();
    if (display == EGL_NO_DISPLAY || context == EGL_NO_CONTEXT) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Failed to eglGetCurrentDisplay or eglGetCurrentContext");
        return 0;
    }

    /*pl_log pllog = pl_log_create(PL_API_VER, &(struct pl_log_params) {
            .log_cb     = log_cb,
            .log_level   = PL_LOG_DEBUG,
    });*/

    struct pl_log_params params = {
            .log_cb    = log_cb,
            .log_level = PL_LOG_DEBUG,
    };

    pl_log pllog = pl_log_create(PL_API_VER, &params);


    struct pl_opengl_params gl_params = {
            .get_proc_addr = eglGetProcAddress,
            .allow_software     = true,         // allow software rasterers
            .debug              = true,         // enable error reporting
    };
    pl_opengl plgl = pl_opengl_create(pllog, &gl_params);
    if (!plgl) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Failed to create pl_opengl");
        return 0;
    }

    pl_renderer renderer = pl_renderer_create(pllog, plgl->gpu);
    if (!renderer) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Failed to create renderer");
        pl_opengl_destroy(&plgl);
        return 0;
    }

    pl_fmt format_r8 = pl_find_named_fmt(plgl->gpu, "r8");
    if (!format_r8) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Format r8 not found");
        return 0;
    }

    pl_fmt format_rgba8 = pl_find_named_fmt(plgl->gpu, "rgba8");
    if (!format_rgba8) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Format rgba8 not found");
        return 0;
    }


    LibplaceboContext* libplaceboContext = (LibplaceboContext*)malloc(sizeof(LibplaceboContext));
    if (!libplaceboContext) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Failed to allocate LibplaceboContext");
        return 0;
    }

    libplaceboContext->pllog = pllog;
    libplaceboContext->plgl = plgl;
    libplaceboContext->renderer = renderer;
    libplaceboContext->format_r8 = format_r8;
    libplaceboContext->format_rgba8 = format_rgba8;

    // Return pointer casted to jlong
    return (jlong)libplaceboContext;
}

extern "C"
JNIEXPORT void JNICALL Java_com_example_prototypelibass_MainActivity_nativeUninitializeLibplacebo(JNIEnv* env, jclass clazz, jlong ctxPtr) {
    if (!ctxPtr) return;

    LibplaceboContext* ctx = (LibplaceboContext*)ctxPtr;

    if (ctx->renderer) {
        pl_renderer_destroy(&ctx->renderer);
    }

    if (ctx->plgl) {
        pl_opengl_destroy(&ctx->plgl);
    }

    if (ctx->pllog) {
        pl_log_destroy(&ctx->pllog);
    }


    free(ctx);
}



