#include <stdio.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>

static int encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *out) {
    int ret = avcodec_send_frame(ctx, frame);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "failed to send frame to encoder: %s\n", av_err2str(ret));
        goto __END;
    }

    while(ret >= 0) {
        avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret ==AVERROR_EOF) {
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

    // 2 查找编码器
    const AVCodec *codec = avcodec_find_encoder_by_name(codecname);
    if (!codec) {
        goto __ERROR;
    }
    // 3 创建编码器上下文
    AVCodecContext *ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        av_log(NULL, AV_LOG_ERROR, "NO MEMORY\n");
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
    FILE* f = fopen(dst, "wb");
    if (!f) {
        av_log(NULL, AV_LOG_ERROR, "dont open file: %s\n", dst);
        goto __ERROR;
    }
    // 7 创建AVFrame, 保存视频原始数据, frame是存储数据的外壳
    AVFrame *frame = av_frame_alloc();
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
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        av_log(NULL, AV_LOG_ERROR, "no memory to alloc pkt\n");
        goto __ERROR;
    }
    // 9 生成视频内容，通过摄像头或抓取屏幕api
    for (int i=0;i<25;i++) {
        // 确认frame中的data域是有效的， 如果被锁定了会重新分配一个buffer
        // 编码是会将frame传给编码器， 编码器会锁定frame中的data, 拿数据进行编码
        ret = av_frame_make_writable(frame);
        if (ret < 0) {
            break;
        }
        for (int y=0;y<ctx->height;y++) {
            for (int x=0;x<ctx->width;x++) {
                // 这里的i是第几帧
                frame->data[0][y*frame->linesize[0]+x] = x + y + i*3; // 控制的y数据，就是亮度
            }
        }
        for (int y=0;y<ctx->height;y++) {
            for (int x=0;x<ctx->width/2;x++) {
                // 这里的i是第几帧
                frame->data[1][y*frame->linesize[1]+x] = 128 + y + i*2; // u分量中128代表黑色
                frame->data[2][y*frame->linesize[2]+x] = 64 + y + i*5; // v分量中64代表黑色
            }
        } 
        frame->pts = i;
        
        // 10 编码
        ret = encode(ctx, frame, pkt, f);
        if (ret < 0) {
            goto __ERROR;
        }
    }

    // 11 把编码器缓冲区缓存的剩余数据强制输出
    encode(ctx, NULL, pkt, f);
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
