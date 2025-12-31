#include <stdio.h>
#include <libavutil/log.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>

int main(int argc, char *argv[]) {

    // 1. 处理参数
    char * src;
    char * dst;
    AVFormatContext *pFmtCtx=  NULL;
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
    // 4. 目的文件上下文
    // 5. 创建音频流
    // 6. 设置音频参数
    // 7. 写入多媒体文件头到目的文件
    // 8. 从源文件中读到音频数据到目的文件中
    // 9. 写多媒体文件尾到目的文件
    // 10. 释放资源并关闭文件
    printf("extra audio\n");
    return 0;
}