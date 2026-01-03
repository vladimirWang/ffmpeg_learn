#!/bin/bash

clang -g -o a remux.c `pkg-config -cflags --libs libavutil libavformat libavcodec`