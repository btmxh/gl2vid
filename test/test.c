#include "gl2vid.h"

#include "stdlib.h"
#include "stdio.h"

#define EXIT() do { fprintf(stderr, "Error occurred: %s\n", g2v_get_error_log()); exit(1); } while(0);
#define CHECK(x) if(!(x)) { fprintf(stderr, "Error occurred: %s\n", g2v_get_error_log()); exit(1); }
#define FPS 25
#define SECONDS 20.0
const int frames = (int) (FPS * SECONDS);

int render_video_frame(g2v_render_ctx* ctx, void* userptr) {
    int i = ctx->current_frame_index;
    glClearColor(1.0f / frames * i, 1.0f / frames * i, 1.0f / frames * i, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    return i >= frames;
}

int main() {
    g2v_context* ctx = g2v_create_context();
    CHECK(ctx != NULL);

    g2v_render_ctx rctx;
    g2v_encoder encoder;

    CHECK(g2v_init_render_ctx(&rctx, 2048, 2048))
    CHECK(g2v_create_ffmpeg_encoder(&encoder, &rctx, FPS, "output.mkv"))
    encoder.render_video_frame = render_video_frame;
    CHECK(g2v_encode(&encoder, &rctx))
    CHECK(g2v_finish_ffmpeg_encoder(&encoder))

    g2v_free_render_ctx(&rctx);
    g2v_free_context(ctx);

    return 0;
}
