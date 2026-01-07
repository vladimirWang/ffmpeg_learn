#include <stdio.h>
#include <stdlib.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>

// ========== 提取的独立函数：生成模拟视频帧数据 ==========
/**
 * 生成模拟的YUV420P格式视频帧数据
 * @param ctx 编码器上下文（获取宽/高/像素格式等参数）
 * @param frame 待填充数据的AVFrame（需提前分配好内存）
 * @param frame_idx 帧索引（用于生成渐变的YUV数据）
 * @return 0成功，<0失败
 */
int generate_video_frame(AVCodecContext *ctx, AVFrame *frame, int frame_idx) {
    if (!ctx || !frame) {
        av_log(NULL, AV_LOG_ERROR, "invalid input parameters for generate_video_frame\n");
        return -1;
    }

    // 确保frame可写（编码器可能锁定内存，需解锁/重新分配）
    int ret = av_frame_make_writable(frame);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "frame make writable failed: %s\n", av_err2str(ret));
        return ret;
    }

    // 填充Y分量（亮度）：完整分辨率
    for (int y = 0; y < ctx->height; y++) {
        for (int x = 0; x < ctx->width; x++) {
            frame->data[0][y * frame->linesize[0] + x] = x + y + frame_idx * 3;
        }
    }

    // 填充U分量（色度）：宽度减半
    for (int y = 0; y < ctx->height; y++) {
        for (int x = 0; x < ctx->width / 2; x++) {
            frame->data[1][y * frame->linesize[1] + x] = 128 + y + frame_idx * 2;
        }
    }

    // 填充V分量（色度）：宽度减半
    for (int y = 0; y < ctx->height; y++) {
        for (int x = 0; x < ctx->width / 2; x++) {
            frame->data[2][y * frame->linesize[2] + x] = 64 + y + frame_idx * 5;
        }
    }

    // 设置帧的时间戳（PTS）
    frame->pts = frame_idx;

    return 0;
}