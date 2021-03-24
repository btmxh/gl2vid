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

const char* g2v_get_error_log();

typedef struct g2v_context g2v_context;

//Create an OpenGL context
g2v_context* g2v_create_context();
void g2v_free_context(g2v_context* ctx);

#define G2V_TARGETS 2
#define G2V_TARGET_RENDERBUFFER

typedef struct {
    int width, height;
    int current_frame_index;
    GLuint framebuffers[G2V_TARGETS];
#ifdef G2V_TARGET_RENDERBUFFER
    GLuint renderbuffers[G2V_TARGETS];
#else
    GLuint textures[G2V_TARGETS];
#endif
    GLuint pbos[G2V_TARGETS];
    int* pix_data;
} g2v_render_ctx;

int g2v_init_render_ctx(g2v_render_ctx* ctx, int width, int height);
void g2v_free_render_ctx(g2v_render_ctx* ctx);

struct g2v_encoder {
    void* internal_data;
    int(*encode_fn)(g2v_render_ctx*, void*);
    //USER DEFINED CALLBACKS
    int(*render_video_frame)(g2v_render_ctx*, struct g2v_encoder*);
    int(*render_audio_frame)(g2v_render_ctx*, struct g2v_encoder*);
    void* user_ptr;
};

typedef struct g2v_encoder g2v_encoder;

int g2v_encode(g2v_encoder* encoder, g2v_render_ctx* render_ctx);

#ifdef G2V_USE_FFMPEG_ENCODER

int g2v_create_ffmpeg_encoder(g2v_encoder* enc, g2v_render_ctx* ctx, int fps, const char* output_file);
int g2v_init_ffmpeg_audio_stream(g2v_encoder* enc);
int g2v_finish_ffmpeg_encoder(g2v_encoder* enc);

#endif

#ifdef G2V_USE_NVIDIA_ENCODER

int g2v_create_nv_encoder(g2v_encoder* enc, g2v_render_ctx* ctx, int fps, const char* output_file);
int g2v_init_nv_audio_stream(g2v_encoder* enc);
int g2v_finish_nv_encoder(g2v_encoder* enc);

#endif

//very shitty timers
double g2v_get_time();

#ifdef __cplusplus
}
#endif

#endif
