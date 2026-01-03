#!/bin/bash

clang -g -o a extra_audio.c `pkg-config -cflags --libs libavutil libavformat libavcodec`