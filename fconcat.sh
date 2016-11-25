#!/bin/bash
eval "./fshEncode.out"
eval "ffmpeg -f concat -i audioList.txt -c copy finalOutput.mp2"
