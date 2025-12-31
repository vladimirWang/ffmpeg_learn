#include <stdio.h>
#include <libavutil/log.h>
#include <libavformat/avformat.h>

int main()
{
    av_log_set_level(AV_LOG_INFO);

    AVIODirContext *ctx = NULL;
    AVIODirEntry *entry = NULL;
    int ret = avio_open_dir(&ctx, "./", NULL);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open dir failed: %s\n", av_err2str(ret));
        return -1;
    }
    while (1)
    {
        ret = avio_read_dir(ctx, &entry);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "read dir failed: %s\n", av_err2str(ret));
            goto __fail;
        }
        if (!entry)
        {
            // av_log(NULL, AV_LOG_INFO, "%12" PRId64 " %s \n", entry->size, entry->name);
            break;
        }

        av_log(NULL, AV_LOG_INFO, "%12" PRId64 " %s\n", entry->size, entry->name);
        avio_free_directory_entry(&entry);
    }
    // av_log(NULL, AV_LOG_INFO, "s");

__fail:
    avio_close_dir(&ctx);
    av_log(NULL, AV_LOG_INFO, "success3\n");
    return 0;
}