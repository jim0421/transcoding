#!/bin/bash
eval "rm syw/big_*"
eval "./fshDecode syw/big"
eval "./fshEncode.out syw/big"
eval "rm syw/finalOutput.mp2"
eval "ffmpeg -f concat -i audioList.txt -c copy syw/finalOutput.mp2"
