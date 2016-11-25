#! /bin/sh
gcc fshEncode.cpp -g -w -o fshEncode.out \
-I /usr/local/include -L /usr/local/lib -lavformat -lavcodec -lavutil -lswscale -lpthread
