#ifndef GL2VID_H
#define GL2VID_H

#ifdef __cplusplus
extern "C" {
#endif

#define G2V_TRUE 1
#define G2V_FALSE 0

#ifndef __gl_h_
#include "glad/glad.h"
#endif

#include <stdio.h>

/**
 * @brief Get the latest error log from gl2vid
 * 
 * @return the latest error log
 */
const char* g2v_get_error_log();

/**
 * @brief An opaque structure to store OpenGL context data.
 * 
 * You can have only one gl2vid context at any time, and if you, for whatever reasons,
 * want to recreate another context, you must free the current context first.
 * 
 * Since the gl2vid API doesn't need OpenGL context data to work, this struct exists 
 * for the sole purpose of being freed in the end.
 * 
 * If you are using another OpenGL context, you don't even need a g2v_context to use the gl2vid API
 * 
 * @see g2v_create_context()
 * @see g2v_free_context(g2v_context*)
 * 
 */
typedef struct g2v_context g2v_context;

/**
 * @brief Create a gl2vid context, which is required for OpenGL rendering.
 * 
 * Since this internally is an OpenGL context, you may (or even must) not call this if you already have an OpenGL context
 * 
 * @return pointer to gl2vid context, which may be NULL if failed
 */
g2v_context* g2v_create_context();

/**
 * @brief Deinit a initialized gl2vid context.
 * 
 * @param ctx pointer to already created gl2vid context
 */
void g2v_free_context(g2v_context* ctx);

#define G2V_TARGETS 2
#define G2V_TARGET_RENDERBUFFER

/**
 * @brief gl2vid render context, which contains all OpenGL objects needed to render offscreen
 * 
 */
typedef struct {
    /**
     * @brief Video frame dimensions
     * 
     */
    int width, height;

    /**
     * @brief Current frame index, starts from 0
     * 
     */
    int current_frame_index;

    /**
     * @brief Framebuffers for all rendering works to be rendered on
     * 
     */
    GLuint framebuffers[G2V_TARGETS];
#ifdef G2V_TARGET_RENDERBUFFER
    /**
     * @brief Renderbuffer attachment for framebuffers
     * 
     */
    GLuint renderbuffers[G2V_TARGETS];
#else
    /**
     * @brief Texture attachment for framebuffers
     * 
     */
    GLuint textures[G2V_TARGETS];
#endif

    /**
     * @brief Pixel buffer objects for speeding up CPU-GPU pixels transfer
     * 
     */
    GLuint pbos[G2V_TARGETS];

    /**
     * @brief Current pixel data, in RGBA32 (for better alignment, and maybe transparency support in the future)
     * 
     * 
     */
    int* pix_data;
} g2v_render_ctx;

/**
 * @brief Initialize a allocated gl2vid render context
 * 
 * @param ctx pointer to the allocated render context
 * @param width width of new video frame
 * @param height height of new video frame
 * @return G2V_TRUE if success, G2V_FALSE otherwise
 */
int g2v_init_render_ctx(g2v_render_ctx* ctx, int width, int height);

/**
 * @brief Deinit the allocated gl2vid render context
 * 
 * @param ctx pointer to the render context to be deinited
 */
void g2v_free_render_ctx(g2v_render_ctx* ctx);

/**
 * @brief Abstract interface of a gl2vid video encoder
 * 
 */
struct g2v_encoder {
    /**
     * @brief Internal data, depends on the encoder type
     * 
     */
    void* internal_data;

    /**
     * @brief Encode function, depends on the encoder type
     * 
     */
    int(*encode_fn)(g2v_render_ctx*, void*);

    /**
     * @brief User callback to render video frames.
     * 
     */
    int(*render_video_frame)(g2v_render_ctx*, struct g2v_encoder*);

    /**
     * @brief User callback to render audio frames.
     * 
     */
    int(*render_audio_frame)(g2v_render_ctx*, struct g2v_encoder*);

    /**
     * @brief User pointer.
     * 
     */
    void* user_ptr;
};

/**
 * @brief Abstract interface of a gl2vid video encoder
 * 
 */
typedef struct g2v_encoder g2v_encoder;

/**
 * @brief Encode the whole media, using the encoder and render context specified
 * 
 * @param encoder pointer to initialized gl2vid video encoder
 * @param render_ctx pointer to initialized gl2vid render context
 * @return G2V_TRUE if success, G2V_FALSE otherwise
 */
int g2v_encode(g2v_encoder* encoder, g2v_render_ctx* render_ctx);

#ifdef G2V_USE_FFMPEG_ENCODER

/**
 * @brief Create a video encoder which internally uses ffmpeg, with frame dimensions fetched from the render context.
 * 
 * @param enc pointer to allocated gl2vid video encoder
 * @param ctx pointer to initialized gl2vid render context
 * @param fps number of frames per second of output video
 * @param output_file output filename
 * @return G2V_TRUE if success, G2V_FALSE otherwise
 */
int g2v_create_ffmpeg_encoder(g2v_encoder* enc, g2v_render_ctx* ctx, int fps, const char* output_file);

/**
 * @brief Create an audio stream for an ffmpeg video encoder
 * 
 * NOTE: audio is not supported (yet) in gl2vid
 * 
 * @param enc pointer to initialized ffmpeg video encoder
 * @return G2V_TRUE if success, G2V_FALSE otherwise
 */
int g2v_init_ffmpeg_audio_stream(g2v_encoder* enc);

/**
 * @brief Finish encoding of an ffmpeg encoder (write trailer + free allocated memory)
 * 
 * @param enc pointer to initialized ffmpeg video encoder
 * @return G2V_TRUE if success, G2V_FALSE otherwise 
 */
int g2v_finish_ffmpeg_encoder(g2v_encoder* enc);

#endif

#ifdef G2V_USE_NVIDIA_ENCODER

/*
    NVIDIA's encoder (NVenc) is currently not supported because I don't understand jack shit from their examples KEKW
*/

/**
 * @brief Create a video encoder which internally uses NVenc, with frame dimensions fetched from the render context.
 * 
 * @param enc pointer to allocated gl2vid video encoder
 * @param ctx pointer to initialized gl2vid render context
 * @param fps number of frames per second of output video
 * @param output_file output filename
 * @return G2V_TRUE if success, G2V_FALSE otherwise
 */
int g2v_create_nv_encoder(g2v_encoder* enc, g2v_render_ctx* ctx, int fps, const char* output_file);

/**
 * @brief Create an audio stream for an NVenc video encoder
 * 
 * NOTE: audio is not supported (yet) in gl2vid
 * 
 * @param enc pointer to initialized NVenc video encoder
 * @return G2V_TRUE if success, G2V_FALSE otherwise
 */
int g2v_init_nv_audio_stream(g2v_encoder* enc);

/**
 * @brief Finish encoding of an NVenc encoder (write trailer + free allocated memory)
 * 
 * @param enc pointer to initialized NVenc video encoder
 * @return G2V_TRUE if success, G2V_FALSE otherwise 
 */
int g2v_finish_nv_encoder(g2v_encoder* enc);

#endif

/**
 * @brief Basically glfwGetTime(), included to measure encoding time. If you are using C++, std::chrono::high_resolution_clock should be preferred.
 * 
 * @return the current time in seconds
 */
double g2v_get_time();

#ifdef __cplusplus
}
#endif

#endif
