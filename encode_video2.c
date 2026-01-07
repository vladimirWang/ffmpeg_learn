#include <stdio.h>
#include <stdlib.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include "./util.c"

// 入参实例
// ./a test.h264 libx264

int encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *out) {
    int ret = avcodec_send_frame(ctx, frame);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "failed to send frame to encoder: %s\n", av_err2str(ret));
        goto __END;
    }

    // 如果是刷新编码器（frame == NULL），需要持续接收直到 AVERROR_EOF
    // 如果是正常编码，遇到 AVERROR(EAGAIN) 可以返回（编码器需要更多输入）
    int is_flush = (frame == NULL);
    
    while(ret >= 0) {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN)) {
            // 如果是刷新操作，继续循环；如果是正常编码，返回等待更多输入
            if (is_flush) {
                continue;
            } else {
                return 0;
            }
        } else if (ret == AVERROR_EOF) {
            // 编码器已刷新完毕
            return 0;
        } else if (ret < 0) {
            return -1;
        }
        fwrite(pkt->data, 1, pkt->size, out);
        av_packet_unref(pkt);
    }
    
    __END:
    
    return 0;
}

int main(int argc, char* argv[]) {
    // // 音频流在多媒体文件中的索引值
    // int idx = -1;
    // AVPacket pkt;
    // AVFormatContext *pFmtCtx=  NULL;
    // AVFormatContext *oFmtCtx = NULL;
    // const AVOutputFormat *outFmt = NULL;

    // AVStream *outStream = NULL;
    // AVStream *inStream = NULL;
    
    av_log_set_level(AV_LOG_DEBUG);
    // 1. 输入参数
    av_log(NULL, AV_LOG_INFO, "extra audio start\n");
    
    if (argc< 3) {
        av_log(NULL, AV_LOG_INFO, "argv count is %s\n", "less than necessary");
        exit(-1);
    }
    char * codecname = argv[2];
    char * dst = argv[1];

    // 初始化变量
    AVCodecContext *ctx = NULL;
    AVFrame *frame = NULL;
    AVPacket *pkt = NULL;
    FILE *f = NULL;

    // 2 查找编码器
    const AVCodec *codec = avcodec_find_encoder_by_name(codecname);
    if (!codec) {
        av_log(NULL, AV_LOG_ERROR, "codec not found: %s\n", codecname);
        goto __ERROR;
    }
    // 3 创建编码器上下文
    ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        av_log(NULL, AV_LOG_ERROR, "NO MEMORY\n");
        goto __ERROR;
    }
    // 4 设置编码器参数
    ctx->width = 640;
    ctx->height = 480;
    ctx->bit_rate = 500000;
    ctx->time_base = (AVRational){1, 25};
    ctx->framerate = (AVRational){25, 1}; // 设置帧率（25帧每秒）
    ctx->gop_size = 10; // 每10帧一组
    ctx->max_b_frames = 1; // 一组帧中最多几个b帧
    ctx->pix_fmt = AV_PIX_FMT_YUV420P; // 视频源格式
    
    if (codec->id == AV_CODEC_ID_H264) {
        av_opt_set(ctx->priv_data, "preset", "slow", 0);
    }
    // 5 编码器与编码器上下文绑定
    int ret = avcodec_open2(ctx, codec, NULL);
    if (ret < 0) {
        av_log(ctx, AV_LOG_ERROR, "codec open failed: %s\n", av_err2str(ret));
        goto __ERROR;
    }
    // 6 创建输出文件
    f = fopen(dst, "wb");
    if (!f) {
        av_log(NULL, AV_LOG_ERROR, "dont open file: %s\n", dst);
        goto __ERROR;
    }
    // 7 创建AVFrame, 保存视频原始数据, frame是存储数据的外壳
    frame = av_frame_alloc();
    if (!frame) {
        av_log(NULL, AV_LOG_ERROR, "no memory to alloc frame\n");
        goto __ERROR;
    }
    frame->width = ctx->width;
    frame->height = ctx->height;
    frame->format = ctx->pix_fmt;
    // 为frame分配空间时，必须先设置好宽高，否则报错如下
    // could not allocate the video frame Invalid argument

    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "could not allocate the video frame %s\n", av_err2str(ret));
        goto __ERROR;
    }
    // 8 创建AVPacket, 保存编码后的视频帧
    pkt = av_packet_alloc();
    if (!pkt) {
        av_log(NULL, AV_LOG_ERROR, "no memory to alloc pkt\n");
        goto __ERROR;
    }
    // 9 生成视频内容，通过摄像头或抓取屏幕api
    for (int i=0;i<25;i++) {
        // 调用独立函数生成视频帧
        ret = generate_video_frame(ctx, frame, i);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "generate frame %d failed\n", i);
            goto __ERROR;
        }
        
        // 10 编码
        ret = encode(ctx, frame, pkt, f);
        if (ret < 0) {
            goto __ERROR;
        }
    }

    // 11 把编码器缓冲区缓存的剩余数据强制输出
    // 发送 NULL frame 来刷新编码器
    ret = avcodec_send_frame(ctx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "failed to flush encoder: %s\n", av_err2str(ret));
        goto __ERROR;
    }
    
    // 持续接收所有剩余的编码数据，直到返回 AVERROR_EOF
    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR_EOF) {
            // 编码器已完全刷新
            break;
        } else if (ret == AVERROR(EAGAIN)) {
            // 继续等待
            continue;
        } else if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "error receiving packet during flush: %s\n", av_err2str(ret));
            goto __ERROR;
        }
        fwrite(pkt->data, 1, pkt->size, f);
        av_packet_unref(pkt);
    }
    
    printf("succeed\n");

__ERROR:
    if (ctx) {
        avcodec_free_context(&ctx);
    }
    if (frame) {
        av_frame_free(&frame);
    }
    if (pkt) {
        av_packet_free(&pkt);
    }
    if (f) {
        fclose(f);
    }
    return 0;
}
