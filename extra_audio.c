#include <stdio.h>
#include <libavutil/log.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>

int main(int argc, char *argv[]) {
    
    // 1. 处理参数
    char * src;
    char * dst;
    // 音频流在多媒体文件中的索引值
    int idx = -1;
    AVPacket pkt;
    AVFormatContext *pFmtCtx=  NULL;
    AVFormatContext *oFmtCtx = NULL;
    const AVOutputFormat *outFmt = NULL;

    AVStream *outStream = NULL;
    AVStream *inStream = NULL;
    
    av_log_set_level(AV_LOG_DEBUG);
    
    if (argc< 3) {
        av_log(NULL, AV_LOG_INFO, "argv count is %s\n", "less than necessary");
        exit(-1);
    }
    src = argv[1];
    dst = argv[2];
    // 2. 打开多媒体文件
    int ret = avformat_open_input(&pFmtCtx, src, NULL, NULL);
    if (ret< 0) {
        av_log(NULL, AV_LOG_ERROR, "open input file failed: %s\n", av_err2str(ret));
        exit(-1);

    }
    av_log(NULL, AV_LOG_INFO, "open input file success\n");
    // 3. 读取音频流信息
    idx = av_find_best_stream(pFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (idx<0) {
        av_log(pFmtCtx, AV_LOG_INFO, "does not include audio stream\n");
        goto __ERROR;
    }
    // 4. 目的文件上下文
    oFmtCtx = avformat_alloc_context();
    if (!oFmtCtx) {
        av_log(NULL, AV_LOG_ERROR, "NO memory\n");
        goto __ERROR;
    }
    // 通过输出的文件名找到avOutputFmt, 
    // 把outputfmt设置到 oFmtCtx
    outFmt = av_guess_format(NULL, dst, NULL);
    oFmtCtx->oformat = outFmt;

    // 5. 创建音频流
    outStream = avformat_new_stream(oFmtCtx, NULL);
    // 6. 设置音频参数
    inStream = pFmtCtx->streams[idx];
    avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
    // 设置0就会根据多媒体文件自动适配编解码器
    outStream->codecpar->codec_tag = 0;

    // 将上下文与目标文件进行绑定
    ret = avio_open2(&oFmtCtx->pb, dst, AVIO_FLAG_WRITE, NULL, NULL);
    if (ret < 0) {
        av_log(oFmtCtx, AV_LOG_ERROR, "%s", av_err2str(ret));
        goto __ERROR;
    }
    // 7. 写入多媒体文件头到目的文件
    ret = avformat_write_header(oFmtCtx, NULL);
    if (ret<0) {
        av_log(oFmtCtx, AV_LOG_ERROR, "%s", av_err2str(ret));
        goto __ERROR;
    }
    // 8. 从源文件中读到音频数据到目的文件中
    while(av_read_frame(pFmtCtx, &pkt) >=0) {
        if (pkt.stream_index == idx) {
            // 改变时间戳
            pkt.pts = av_rescale_q_rnd(pkt.pts, inStream->time_base, outStream->time_base, (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            pkt.dts = pkt.pts;
            pkt.duration = av_rescale_q(pkt.duration, inStream->time_base, outStream->time_base);
            pkt.stream_index = 0;
            pkt.pos = -1;
            av_interleaved_write_frame(oFmtCtx, &pkt);
            av_packet_unref(&pkt);
        }
    }
    // 9. 写多媒体文件尾到目的文件
    av_write_trailer(oFmtCtx);
    // 10. 释放资源并关闭文件
    printf("extra audio\n");
    __ERROR:
    if (pFmtCtx) {
        avformat_close_input(&pFmtCtx);
        pFmtCtx = NULL;
    }
    if (oFmtCtx->pb) {
        avio_close(oFmtCtx->pb);
    }
    if (oFmtCtx) {
        avformat_free_context(oFmtCtx);
        oFmtCtx = NULL;
    }
    return 0;
}