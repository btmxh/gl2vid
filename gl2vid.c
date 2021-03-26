#include "gl2vid.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "GLFW/glfw3.h"

void print_memory(char* ptr, size_t bytes) {
    printf("Pointer value: %lld\n", (long long)ptr);
    const int cols = 40;
    for(int i = 0; i < bytes; i++) {
        printf("%02hhX", ptr[i]);
        printf(" ");
        if((i + 1) % cols == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

struct g2v_context {
    GLFWwindow* window;
};

char g2v_error_log[256] = { 0 };
void err_printf(const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vsnprintf(g2v_error_log, 256, fmt, va);
    va_end(va);
}

const char* g2v_get_error_log() {
    return g2v_error_log;
}

const char* get_glfw_error() {
    char* err;
    glfwGetError(&err);
    return err;
}

g2v_context* g2v_create_context() {
    if(!glfwInit()) {
        err_printf("GLFW error: %s", get_glfw_error());
        return NULL;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(50, 50, "", NULL, NULL);
    if(!window) {
        err_printf("GLFW window creation error: %s", get_glfw_error());
        glfwTerminate();
        return NULL;
    }
    glfwMakeContextCurrent(window);
    if(!gladLoadGL()) {
        err_printf("GLAD initialization error.");
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    g2v_context* ctx = malloc(sizeof* ctx);
    ctx->window = window;
    return ctx;
}

void g2v_free_context(g2v_context* ctx) {
    if(!ctx)    return;
    glfwDestroyWindow(ctx->window);
    glfwTerminate();
}

int g2v_init_render_ctx(g2v_render_ctx* ctx, int width, int height) {
    ctx->width = width;
    ctx->height = height;
    ctx->pix_data = malloc(sizeof(int) * width * height);
    ctx->current_frame_index = 0;

    glGenFramebuffers(G2V_TARGETS, ctx->framebuffers);
    glGenBuffers(G2V_TARGETS, ctx->pbos);
#ifdef G2V_TARGET_RENDERBUFFER
    glGenRenderbuffers(G2V_TARGETS, ctx->renderbuffers);
#else
    glGenTextures(G2V_TARGETS, ctx->textures);
#endif

    for(int i = 0; i < G2V_TARGETS; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, ctx->framebuffers[i]);
#ifdef G2V_TARGET_RENDERBUFFER
        glBindRenderbuffer(GL_RENDERBUFFER, ctx->renderbuffers[i]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA4, width, height);
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, ctx->renderbuffers[i]);
#else
        glBindTexture(GL_TEXTURE_2D, ctx->textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx->textures[i], 0);
#endif
        glBindBuffer(GL_PIXEL_PACK_BUFFER, ctx->pbos[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, width * height * 4, NULL, GL_STREAM_READ);

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }
    return G2V_TRUE;
}

void g2v_free_render_ctx(g2v_render_ctx* ctx) {
    free(ctx->pix_data);
    glDeleteFramebuffers(G2V_TARGETS, ctx->framebuffers);
    glDeleteBuffers(G2V_TARGETS, ctx->pbos);
#ifdef G2V_TARGET_RENDERBUFFER
    glDeleteRenderbuffers(G2V_TARGETS, ctx->renderbuffers);
#else
    glDeleteTextures(G2V_TARGETS, ctx->textures);
#endif
}

void prepare_gl_state(g2v_render_ctx* ctx) {
    glBindFramebuffer(GL_FRAMEBUFFER, ctx->framebuffers[ctx->current_frame_index % G2V_TARGETS]);
    glViewport(0, 0, ctx->width, ctx->height);
}

void read_gl_data(g2v_render_ctx* ctx) {
    int read_pbo_idx = ctx->current_frame_index % G2V_TARGETS;
    int proc_pbo_idx = (read_pbo_idx + 1) % G2V_TARGETS;

    glBindBuffer(GL_PIXEL_PACK_BUFFER, ctx->pbos[read_pbo_idx]);
    // glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    // int* buffer_content = malloc(ctx->width * ctx->height * sizeof(int));
    glReadPixels(0, 0, ctx->width, ctx->height, GL_BGRA, GL_UNSIGNED_BYTE, 0);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, ctx->pbos[proc_pbo_idx]);
    int* buffer_content = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

    //Since glReadPixels returns pixel values upside down, we have to preprocess them
    // memcpy(ctx->pix_data, buffer_content, ctx->width * ctx->height * sizeof(int));
    int* pix_data = ctx->pix_data + ctx->width * (ctx->height - 1);
    do {
        memcpy(pix_data, buffer_content, ctx->width * sizeof(int));
        pix_data -= ctx->width;
        buffer_content += ctx->width;
    } while(pix_data != ctx->pix_data);

    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

    ctx->current_frame_index++;
}

int g2v_encode(g2v_encoder* encoder, g2v_render_ctx* ctx) {
    return encoder->encode_fn(ctx, encoder);
}

#ifdef G2V_USE_FFMPEG_ENCODER

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"

typedef struct {
    AVStream* stream;
    AVCodec* codec;
    AVCodecContext* codec_ctx;
    AVFrame* frame;
    int64_t next_pts;
    int encoding;
} ffmpeg_output_stream;

typedef struct {
    ffmpeg_output_stream video, audio;
    AVFormatContext* output_ctx;
    struct SwsContext *sws_ctx;
} ffmpeg_internals;

#define G2V_EOF 2

int ffmpeg_write_frame(ffmpeg_internals* fi, ffmpeg_output_stream* stream) {
    int ret = avcodec_send_frame(stream->codec_ctx, stream->frame);
    if(ret < 0) {
        err_printf("Error sending frame");
        return G2V_FALSE;
    }

    while(ret >= 0) {
        AVPacket pkt = { NULL };

        ret = avcodec_receive_packet(stream->codec_ctx, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            err_printf("Error encoding frame");
            return G2V_FALSE;
        }

        av_packet_rescale_ts(&pkt, stream->codec_ctx->time_base, stream->stream->time_base);
        pkt.stream_index = stream->stream->index;
        ret = av_interleaved_write_frame(fi->output_ctx, &pkt);
        av_packet_unref(&pkt);
        if(ret < 0) {
            err_printf("Error writing packet");
        }
    }

    return ret == AVERROR_EOF ? G2V_EOF : G2V_TRUE;
}

int ffmpeg_encode(g2v_render_ctx* ctx, g2v_encoder* encoder) {
    ffmpeg_internals* fi = encoder->internal_data;

    while(fi->audio.encoding || fi->video.encoding) {
        int encode_audio = !fi->video.encoding;
        if(!encode_audio) {
            if(fi->audio.encoding) {
                encode_audio = av_compare_ts(fi->video.next_pts, fi->video.codec_ctx->time_base, fi->audio.next_pts, fi->audio.codec_ctx->time_base) <= 0;
            }
        }

        if(!encode_audio) {
            //Encode video
            prepare_gl_state(ctx);
            int eof = encoder->render_video_frame(ctx, encoder->user_ptr);
            if(eof) {
                fi->video.frame = NULL;
            } else {
                read_gl_data(ctx);
                int in_linesize[1] = { 4 * ctx->width };
                sws_scale(fi->sws_ctx, &ctx->pix_data, in_linesize, 0, ctx->height, fi->video.frame->data, fi->video.frame->linesize);
            }
            int ret = ffmpeg_write_frame(fi, &fi->video);
            if(ret == G2V_FALSE) {
                return G2V_FALSE;
            } else if(ret == G2V_EOF) {
                fi->video.encoding = G2V_FALSE;
            }
        } else {
            
        }
    }

    return G2V_TRUE;
}

int ffmpeg_create_stream(ffmpeg_internals* fi, AVOutputFormat* fmt, ffmpeg_output_stream* os, enum AVCodecID codec_id) {
    os->codec = avcodec_find_encoder(codec_id);
    if(!os->codec) {
        err_printf("Encoder not found");
        return G2V_FALSE;
    }
    os->stream = avformat_new_stream(fi->output_ctx, NULL);
    if(!os->stream) {
        err_printf("Stream allocation failed");
        return G2V_FALSE;
    }
    os->stream->id = fi->output_ctx->nb_streams - 1;
    AVCodecContext* c = avcodec_alloc_context3(os->codec);
    if(!c) {
        err_printf("Codec allocation failed");
        return G2V_FALSE;
    }
    os->codec_ctx = c;
    os->encoding = G2V_TRUE;

    if (fi->output_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


    return G2V_TRUE;
}

void ffmpeg_free_stream(ffmpeg_output_stream* stream) {
    avcodec_free_context(&stream->codec_ctx);
}

int g2v_create_ffmpeg_encoder(g2v_encoder* enc, g2v_render_ctx* ctx, int fps, const char* output_file) {
    ffmpeg_internals* fi = malloc(sizeof* fi);
    avformat_alloc_output_context2(&fi->output_ctx, NULL, NULL, output_file);
    if(!fi->output_ctx) {
        err_printf("Could not allocate format context");
        goto fail1;
    }
    AVOutputFormat* fmt = fi->output_ctx->oformat;

    if(!ffmpeg_create_stream(fi, fmt, &fi->video, fmt->video_codec)) {
        err_printf("Video stream creation failed");
        goto fail2;
    }

    AVCodecContext* c = fi->video.codec_ctx;
    c->codec_id = fmt->video_codec;
    c->width = ctx->width;
    c->height = ctx->height;
    fi->video.stream->time_base = (AVRational) { 1, fps };
    c->time_base = fi->video.stream->time_base;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    if(avcodec_open2(c, fi->video.codec, NULL) < 0) {
        err_printf("Could not open codec");
        goto fail3;
    }

    fi->video.frame = av_frame_alloc();
    if(!fi->video.frame) {
        err_printf("Could not allocate frame");
        goto fail4;
    }

    fi->audio.encoding = G2V_FALSE;
    fi->video.next_pts = 0;
    fi->video.frame->width = c->width;
    fi->video.frame->height = c->height;
    fi->video.frame->format = c->pix_fmt;
    
    if(av_frame_get_buffer(fi->video.frame, 32) < 0) {
        err_printf("Could not allocate raw picture buffer");
        goto fail5;
    }

    fi->sws_ctx = sws_getContext(ctx->width, ctx->height, AV_PIX_FMT_RGB32, ctx->width, ctx->height, AV_PIX_FMT_YUV420P, 0, NULL, NULL, NULL);
    if(!fi->sws_ctx) {
        err_printf("Could not allocate SwsContext");
        goto fail6;
    }

    if(avcodec_parameters_from_context(fi->video.stream->codecpar, c) < 0) {
        err_printf("Could not copy encoding parameters");
        goto fail6;
    }

    av_dump_format(fi->output_ctx, 0, output_file, 1);

    if (!(fmt->flags & AVFMT_NOFILE)) {
        if(avio_open(&fi->output_ctx->pb, output_file, AVIO_FLAG_WRITE) < 0) {
            err_printf("Could not open file: %s", output_file);
            goto fail6;
        }
    }

    if(avformat_write_header(fi->output_ctx, NULL) < 0) {
        err_printf("Could not write header for file: %s", output_file);
        goto fail7;
    }

    enc->internal_data = fi;
    enc->encode_fn = ffmpeg_encode;

    return G2V_TRUE;

fail7:
    avio_closep(&fi->output_ctx->pb);
fail6:
    sws_freeContext(fi->sws_ctx);
fail5:
    av_freep(&fi->video.frame->data[0]);
fail4:
    av_frame_free(&fi->video.frame);
fail3:
    ffmpeg_free_stream(&fi->video);
fail2:
    avformat_free_context(fi->output_ctx);
fail1:
    free(fi);
    return G2V_FALSE;
}

//Not supported
int g2v_init_ffmpeg_audio_stream(g2v_encoder* enc) {
    return G2V_FALSE;
}

int g2v_finish_ffmpeg_encoder(g2v_encoder* enc) {
    ffmpeg_internals* fi = enc->internal_data;

    av_write_trailer(fi->output_ctx);
    avio_closep(&fi->output_ctx->pb);
    sws_freeContext(fi->sws_ctx);
    av_freep(&fi->video.frame->data[0]);
    av_frame_free(&fi->video.frame);
    ffmpeg_free_stream(&fi->video);
    if(fi->audio.stream) {
        ffmpeg_free_stream(&fi->audio);
    }
    avformat_free_context(fi->output_ctx);



    return G2V_TRUE;
}

#endif

#ifdef G2V_USE_NVIDIA_ENCODER
//Not supported yet
#include "nvEncodeAPI.h"

int g2v_create_nv_encoder(g2v_encoder* enc, g2v_render_ctx* ctx, int fps, const char* output_file) {
    return G2V_FALSE;
}
int g2v_init_nv_audio_stream(g2v_encoder* enc) {
    return G2V_FALSE;
}
int g2v_finish_nv_encoder(g2v_encoder* enc) {
    return G2V_FALSE;
}

#endif

double g2v_get_time() {
    return glfwGetTime();
}
