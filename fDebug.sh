#!/bin/bash
eval "rm syw/big_*.mp2"
eval "./fshEncode.out syw/big"
eval "rm syw/finalOutput.mp2"
eval "time ffmpeg -f concat -i audioList_2.txt -c copy finalOutput_2.mp2"
