#!/bin/bash

clang -g -o a extra_video.c `pkg-config -cflags --libs libavutil libavformat libavcodec`