// g++  video.cpp -I/root/git/mediasdk/include -I/root/bin/include -L/root/bin/lib -lavformat -lavcodec -lswscale  -lpthread  -lavfilter -lx264 -lx265 -lavutil -ldl -lpng -lz  -lmp3lame -lswresample -lyuv
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define __STDC_CONSTANT_MACROS

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
};
#endif

#include <3rd/libyuv/libyuv.h>

struct Stream
{
    bool                binput;
    AVFormatContext *   formatCtx;
    AVCodecContext  *   codecCtx;
    AVCodec         *   codec;
    int                 videoIndex;
    int                 audioIndex;
    const char *        name;
    AVFrame *           frame;
    AVPacket *          packet;
    AVStream *          vstream;
    AVStream *          astream;
    uint8 *             yuv;
    int                 width;
    int                 height;
    int                 frameIndex;
};

struct Core
{
    Stream  mask;
    Stream  src;
    Stream  dst;
    
    int     width, height;
    uint8 * dstRGBA;
    uint8 * bgRGBA;
};

void init(void)
{
    avcodec_register_all();
    av_register_all();
}

bool init_input_stream(Stream & stream, const char * name)
{
    stream.binput       = true;
    stream.name         = name;
    stream.formatCtx    = avformat_alloc_context();
    if (avformat_open_input(&stream.formatCtx, stream.name, NULL, NULL) != 0) {
        fprintf(stderr, "open %s error!\n", name);
        return false;
    }
    
    if (avformat_find_stream_info(stream.formatCtx, NULL) < 0) {
        fprintf(stderr, "get %s stream info error!\n", name);
        return false;
    }
    
    stream.videoIndex = -1;
    for(int i = 0; i < stream.formatCtx->nb_streams; i++) {
        if(stream.formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            stream.videoIndex = i;
            break;
        }
    }
    
    if (stream.videoIndex == -1) {
        fprintf(stderr, "%s not video stream\n", name);
        return false;
    }
    
    stream.codecCtx = stream.formatCtx->streams[stream.videoIndex]->codec;
    stream.codec    = avcodec_find_decoder(stream.codecCtx->codec_id);
    if (stream.codec == NULL) {
        fprintf(stderr, "can't find decoder for %s\n", name);
        return false;
    }
    
    if (avcodec_open2(stream.codecCtx, stream.codec, NULL) < 0) {
        fprintf(stderr, "open decoder for %s error\n", name);
        return false;
    }
    stream.frame    = av_frame_alloc();
    stream.packet   = (AVPacket *)av_malloc(sizeof(AVPacket));
    stream.width    = stream.codecCtx->width;
    stream.height   = stream.codecCtx->height;
    stream.yuv      = (uint8 *)malloc(stream.width * stream.height * 3 / 2);
    
    return true;
}

bool init_output_stream(Stream & stream, const char * name, int w, int h)
{
    stream.binput               = false;
    
    if (avformat_alloc_output_context2(&stream.formatCtx, NULL, NULL, name) < 0) {
        fprintf(stderr, "create output context error:%s\n", name);
        return false;
    }
    
    if (avio_open(&stream.formatCtx->pb, name, AVIO_FLAG_WRITE) != 0) {
        fprintf(stderr, "open output stream file:%s error\n", name);
        return false;
    }
    
    stream.vstream = avformat_new_stream(stream.formatCtx, NULL);
    if (stream.vstream == NULL) {
        fprintf(stderr, "create output video stream error\n");
        return false;
    }
    
    stream.name                     = name;
    stream.vstream->time_base.num   = 1;
    stream.vstream->time_base.den   = 15;
    
    stream.codecCtx                 = stream.vstream->codec;
    stream.codecCtx->codec_id       = stream.formatCtx->oformat->video_codec;
    stream.codecCtx->codec_type     = AVMEDIA_TYPE_VIDEO;
    stream.codecCtx->pix_fmt        = AV_PIX_FMT_YUV420P;
    stream.codecCtx->width          = w;
    stream.codecCtx->height         = h;
    stream.codecCtx->time_base.num  = 1;
    stream.codecCtx->time_base.den  = 15;
    stream.codecCtx->bit_rate       = 400000;
    stream.codecCtx->gop_size       = 30;
    stream.codecCtx->qmin           = 10;
    stream.codecCtx->qmax           = 51;
    stream.codecCtx->qcompress      = 0.6;
    stream.codecCtx->profile        = FF_PROFILE_H264_MAIN;
    
    stream.codec = avcodec_find_encoder(stream.codecCtx->codec_id);
    if (stream.codec == NULL) {
        fprintf(stderr, "can't find video encoder for output stream\n");
        return false;
    }
    
    if(avcodec_open2(stream.codecCtx, stream.codec, NULL) < 0) {
        fprintf(stderr, "open video encoder error\n");
        return false;
    }
    
    //av_dump_format(stream.formatCtx, 0, name, 1);
    stream.frame         = av_frame_alloc();
    stream.frame->width  = stream.codecCtx->width;
    stream.frame->height = stream.codecCtx->height;
    stream.frame->format = stream.codecCtx->pix_fmt;
    
    
    int size  = avpicture_get_size(stream.codecCtx->pix_fmt,
                                   stream.codecCtx->width,
                                   stream.codecCtx->height);
    
    stream.width        = w;
    stream.height       = h;
    stream.yuv          = (uint8 *)av_malloc(size);
    stream.packet       = (AVPacket *)av_malloc(sizeof(AVPacket));
    
    avpicture_fill((AVPicture *)stream.frame,
                   stream.yuv,
                   stream.codecCtx->pix_fmt,
                   stream.codecCtx->width,
                   stream.codecCtx->height);
    
    avformat_write_header(stream.formatCtx, NULL);

    return true;
}

bool init_bg(Core & core, const char * bg)
{
    int fd = open(bg, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open bg:%s error\n", bg);
        return false;
    }
    
    int yuvsize = core.width * core.height * 3 / 2;
    core.bgRGBA = (uint8 *)malloc(core.width * core.height * 4);
    uint8 * yuv = (uint8 *)malloc(yuvsize);
    
    if (read(fd, yuv, yuvsize) != yuvsize) {
        fprintf(stderr, "read bg yuv size smaller than %d\n", yuvsize);
        free(yuv);
        close(fd);
        return false;
    }
    
    int w = core.width, h = core.height;
    
    libyuv::I420ToABGR(yuv, w,
                       yuv + w * h, w >> 1,
                       yuv + w * h * 5 / 4, w >> 1,
                       core.bgRGBA, w * 4, w, h);
    
    free(yuv);
    close(fd);
    return true;
}

bool init_resource(Core & core, const char * n1, const char * n2,
               const char * n3, const char * n4, int w, int h)
{
    memset(&core, 0x00, sizeof(Core));
    
    core.width  = w;
    core.height = h;
    
    if (!init_input_stream(core.mask, n1)) {
        fprintf(stderr, "init mask stream error\n");
        return false;
    }
    
    if (!init_input_stream(core.src, n2)) {
        fprintf(stderr, "init source stream error\n");
        return false;
    }
    
    if (!init_output_stream(core.dst, n3, w, h)) {
        fprintf(stderr, "init output stream error\n");
        return false;
    }
    
    if (!init_bg(core, n4)) {
        fprintf(stderr, "init background error\n");
        return false;
    }
    
    core.dstRGBA    = (uint8 *)malloc(w * h * 4);  //rgba
    
    return true;
}

bool pick_frame(Stream & stream)
{
    int got_picture = 0, ret = 0;;
    
    while (av_read_frame(stream.formatCtx, stream.packet) >= 0) {
        if(stream.packet->stream_index == stream.videoIndex) {
    
            got_picture = 0;
            ret = avcodec_decode_video2(stream.codecCtx,
                                        stream.frame,
                                        &got_picture,
                                        stream.packet);
            
            av_free_packet(stream.packet);
            
            if (ret < 0) {
                fprintf(stderr, "decode %s frame:%d error\n",
                        stream.name, stream.frameIndex + 1);
                return false;
            }
            
            if (got_picture > 0) {
                stream.frameIndex ++;
                
                int ysize = stream.width * stream.height;
                int usize = ysize >> 2;
                int uw = stream.width >> 1, uh = stream.height >> 1;
                
                for (int32_t h = 0; h < stream.height; h++) {
                    memcpy(stream.yuv + h * stream.width,
                           stream.frame->data[0] + h * stream.frame->linesize[0],
                           stream.width);
                }
                
                for (int32_t h = 0; h < uh; h++) {
                    memcpy(stream.yuv + ysize + h * uw,
                           stream.frame->data[1] + h * stream.frame->linesize[1],
                           uw);
                    
                    memcpy(stream.yuv + ysize + usize + h * uw,
                           stream.frame->data[2] + h * stream.frame->linesize[2],
                           uw);
                }
                
                //memcpy(stream.yuv, stream.frame->data[0], ysize);
                //memcpy(stream.yuv + ysize, stream.frame->data[1], usize);
                //memcpy(stream.yuv + ysize + usize, stream.frame->data[2], usize);
                
                return true;
            }
        }
    }
    
    return false;
}

uint8 colorblend(uint32 front, uint32 bg, uint8 mask)
{
    if (mask <= 25) {
        return bg;
    } else if (mask >= 230) {
        return front;
    }
    
    return ((bg * (255 - mask) + mask * front) >> 8) & 0xFF;
}

// algorithm refer: http://blog.csdn.net/xhhjin/article/details/6445460
void alphablend(uint8 * front, uint8 * bg, uint8 * mask, int width, int height)
{
    for (int h = 0; h < height; h ++) {
        for (int w = 0; w < width; w ++) {
            int pos = h * width + w;
            int rgb = pos << 2;
            
            front[rgb + 0] = colorblend(front[rgb + 0], bg[rgb + 0], mask[pos]);
            front[rgb + 1] = colorblend(front[rgb + 1], bg[rgb + 1], mask[pos]);
            front[rgb + 2] = colorblend(front[rgb + 2], bg[rgb + 2], mask[pos]);
            front[rgb + 3] = 0xFF;  //opaque
        }
    }
}

bool push_frame(Stream & stream)
{
    int ysize = stream.width * stream.height;
    int usize = ysize >> 2;
    
    stream.frame->data[0] = stream.yuv;
    stream.frame->data[1] = stream.yuv + ysize;
    stream.frame->data[2] = stream.yuv + ysize + usize;
    
//    av_image_fill_linesizes(stream.frame->linesize,
//                            stream.codecCtx->pix_fmt,
//                            stream.width);
//
//
//    ret = av_image_fill_pointers(stream.frame->data,
//                                 stream.codecCtx->pix_fmt,
//                                 stream.codecCtx->height,
//                                 stream.yuv,
//                                 stream.frame->linesize);
    stream.frame->pts     = stream.frameIndex ++;//(stream.frameIndex ++) * 1000 / 15;
    
    av_init_packet(stream.packet);
    
    int got = 0;
    int ret = avcodec_encode_video2(stream.codecCtx,
                                    stream.packet,
                                    stream.frame,
                                    &got);
    if (ret < 0) {
        fprintf(stderr, "encode %s frame:%d error\n",
                stream.name, stream.frameIndex);
        return false;
    }
    
    if (got == 1) {
        bool iframe = (stream.packet->flags & AV_PKT_FLAG_KEY);
        
//        fprintf(stdout, "encode stream:%s frame:%d done, size:(%s)%d\n",
//                stream.name, stream.frameIndex, iframe ? "I" : "P",
//                stream.packet->size);
        
        stream.packet->stream_index = stream.vstream->index;
        av_packet_rescale_ts(stream.packet,
                             stream.codecCtx->time_base,
                             stream.vstream->time_base);
        stream.packet->pos = -1;
        
        if (av_interleaved_write_frame(stream.formatCtx, stream.packet) < 0) {
            fprintf(stderr, "write frame into %s error\n", stream.name);
            return false;
        }
        
        return true;
    }
    
    return false;
}

void flushout(Stream & stream)
{
    int framecount = 0;
    
    while (true) {
        av_init_packet(stream.packet);
        
        int got = 0;
        int ret = avcodec_encode_video2(stream.codecCtx,
                                        stream.packet,
                                        NULL,
                                        &got);
        av_frame_free(NULL);
        
        if (ret < 0) {
            break;
        }
        
        if (got == 0) {
            break;
        }
        
        //fprintf(stdout, "flush encode, got frame:%d\n", ++ framecount);
        stream.packet->stream_index = stream.vstream->index;
        av_packet_rescale_ts(stream.packet,
                             stream.codecCtx->time_base,
                             stream.vstream->time_base);
        stream.packet->pos = -1;
        
        if (av_interleaved_write_frame(stream.formatCtx, stream.packet) < 0) {
            fprintf(stderr, "write frame into %s error\n", stream.name);
        }
    }
    
    av_write_trailer(stream.formatCtx);
}

bool do_template(Core & core)
{
    int w = core.width;
    int h = core.height;
    int ysize = w * h;
    int usize = ysize >> 2;
    
    while (pick_frame(core.mask) && pick_frame(core.src)) {
        
        // 1. convert src.yuv to RGBA
        libyuv::I420ToABGR(core.src.yuv, w,
                           core.src.yuv + ysize, w >> 1,
                           core.src.yuv + ysize + usize, w >> 1,
                           core.dstRGBA, w * 4, w, h);
        
        // 2. alpha mixed bgRGBA & dstRGBA
        alphablend(core.dstRGBA, core.bgRGBA, core.mask.yuv, w, h);
        
        // 3. convert dstRGBA to yuv
        libyuv::ABGRToI420(core.dstRGBA, w * 4,
                           core.dst.yuv, w,
                           core.dst.yuv + ysize, w >> 1,
                           core.dst.yuv + ysize + usize, w >> 1,
                           w, h);
        
        //memcpy(core.dst.yuv, core.src.yuv, ysize + usize + usize);
        
        // 4. encoder and write to mp4 file.
        push_frame(core.dst);
    }
    
    flushout(core.dst);
    
    return true;
}

void noop(void)
{
}

void uninit_stream(Stream & stream)
{
    stream.yuv != NULL ? av_free(stream.yuv) : noop();
    stream.frame != NULL ? av_frame_free(&stream.frame) : noop();
    stream.packet != NULL ? av_free(stream.packet) : noop();
    stream.codecCtx != NULL ? (void)avcodec_close(stream.codecCtx) : noop();
    
    if (stream.binput) {
        stream.formatCtx != NULL ? avformat_close_input(&stream.formatCtx) : noop();
    } else {
        stream.formatCtx != NULL ? (void)avio_close(stream.formatCtx->pb) : noop();
        stream.formatCtx != NULL ? avformat_free_context(stream.formatCtx) : noop();
    }
}

void uninit_resource(Core & core)
{
    uninit_stream(core.mask);
    uninit_stream(core.src);
    uninit_stream(core.dst);
    
    core.bgRGBA != NULL ? free(core.bgRGBA) : noop();
    core.dstRGBA != NULL ? free(core.dstRGBA) : noop();
}

void showsystime(void)
{
    time_t now;
    struct tm *timenow;
    time(&now);
    timenow = localtime(&now);
    printf("%s/n", asctime(timenow));
}

// input: mask.mp4, src.mp4, output.mp4, bg.yuv, width, height
// mask.mp4, black-white video, video mask.
// src.mp4, source video. according mask.mp4, integrate bg.yuv into output.mp4
// bg.yuv, background image. layout below src.mp4 frame.
//
// 1. decode one frame in mask.mp4 as mask.yuv
// 2. decode one frame in src.mp4 as src.yuv
// 3. convert src.yuv to src.rgba(RGBA format). Alpha channel absolutely opaque.
// 4. regard mask.yuv Y as src.rgba aplha channel. (0->255, black->white)
// 5. convert bg.yuv to bg.rgba
// 6. alpha mixed bg.rgba & src.rgba into output.rgba
// 7. convert output.rgba to output.yuv
// 8. encode output.yuv into output.mp4
int main(int argc, char * argv[])
{
    Core core;
    
    showsystime();
    
    // 1.init
    init();
    
    // initialize test data.
    init_resource(core, "asset1.mp4", "asset2.mp4", "out.mp4", "bg.yuv", 544, 960);
    
    if (do_template(core)) {
        fprintf(stdout, "video template succeed!\n");
    } else {
        fprintf(stderr, "video template failed!\n");
    }
    
    uninit_resource(core);
    
    showsystime();
    
    return 0;
}
