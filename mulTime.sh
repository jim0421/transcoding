#!/bin/bash
for i in `seq 1 6`;
do
    #eval "./fconcat.sh"
    #eval "rm syw/big_*.mp2"
    #eval "./fshEncode.out syw/big"
    #eval "ls -l syw/big_*.mp2>>syw/result.txt"
    eval "rm syw/finalOutput.mp2"
    eval "time ffmpeg -f concat -i audioListB.txt -c copy syw/finalOutput.mp2"
done
