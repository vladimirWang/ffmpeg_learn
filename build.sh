#!/bin/bash

# clang -g -o b -I/usr/local/ffmpeg/include -L/usr/local/ffmpeg/lib -lavutil main.c
# clang -g -o a testDir2.c `pkg-config -cflags --libs libavutil libavformat`
clang -g -o a extra_audio.c `pkg-config -cflags --libs libavutil libavformat libavcodec`