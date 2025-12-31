#include <stdio.h>
#include <libavutil/log.h>
#include <libavformat/avformat.h>

void testLog()
{
    av_log_set_level(AV_LOG_DEBUG);
    // av_log(NULL, AV_LOG_INFO, "hello ffmpeg %d", 120);
    printf("%d\n", 12);
}

int main()
{
    testLog();
    // int ans = testPriv();
    // if (ans != 0)
    // {
    //     return ans
    // }
    return 0;
}