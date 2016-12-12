#!/bin/bash
eval "rm syw/big_*"
eval "time ./fshDecode syw/big"
eval "time ./fshEncode.out syw/big"
eval "rm syw/finalOutput.mp2"
eval " time ffmpeg -f concat -i audioList.txt -c copy finalOutput.mp2"
