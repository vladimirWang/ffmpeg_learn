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
    
    av_log_set_level(AV_LOG_DEBUG);
    av_log(NULL, AV_LOG_INFO, "extra audio start\n");
    
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

    // 4. 目的文件上下文
    // 这一步相当于avformat_alloc_context + av_guess_format这两步
    avformat_alloc_output_context2(&oFmtCtx, NULL, NULL, dst);
    if (!oFmtCtx) {
        av_log(NULL, AV_LOG_ERROR, "NO memory\n");
        goto __ERROR;
    }
    int *stream_map = NULL;
    stream_map = av_calloc(pFmtCtx->nb_streams, sizeof(int));
    if (!stream_map) {
        av_log(NULL, AV_LOG_ERROR, "NO memory 2\n");
        goto __ERROR;
    }
    int i = 0;
    int stream_idx = 0;
    for (i =0;i<pFmtCtx->nb_streams;i++) {
        AVStream *outStream = NULL;
        AVStream *inStream = pFmtCtx->streams[i];
        AVCodecParameters *inCodecPar = inStream->codecpar;
        if (inCodecPar->codec_type != AVMEDIA_TYPE_AUDIO 
            && inCodecPar->codec_type != AVMEDIA_TYPE_VIDEO 
            && inCodecPar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
                stream_map[i] = -1;
                continue;
        }
        stream_map[i] = stream_idx++;
        // 5. 创建音频流
        outStream = avformat_new_stream(oFmtCtx, NULL);
        if (!outStream) {
            av_log(oFmtCtx, AV_LOG_ERROR, "NO memory 3\n");
            goto __ERROR;
        }
        avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
        // 设置0就会根据多媒体文件自动适配编解码器
        outStream->codecpar->codec_tag = 0;
    }


    // 6. 设置音频参数
    // inStream = pFmtCtx->streams[idx];


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
    // 8. 从源文件中读到音频，视频，字幕数据到目的文件中
    while(av_read_frame(pFmtCtx, &pkt) >=0) {
        AVStream *inStream = pFmtCtx->streams[pkt.stream_index];
        if (pkt.stream_index<0) {
            av_packet_unref(&pkt);
            continue;
        }
        int mapped_index = stream_map[pkt.stream_index];
        AVStream *outStream = oFmtCtx->streams[mapped_index];
        pkt.stream_index = mapped_index;
        // 改变时间戳
        // av_packet_rescale_ts 相当于老的api中的设置pts, dts, duration, stream_index
        av_packet_rescale_ts(&pkt, inStream->time_base, outStream->time_base);
        pkt.pos = -1;
        av_interleaved_write_frame(oFmtCtx, &pkt);
        av_packet_unref(&pkt);
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
    if (stream_map) {
        av_free(stream_map);
    }
    return 0;
}