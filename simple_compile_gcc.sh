#! /bin/sh
gcc simpleTranscoder.cpp -g -w -o simpleTranscoder.out \
-I /usr/local/include -L /usr/local/lib -lavformat -lavcodec -lavutil -lswscale -lpthread -lavfilter
