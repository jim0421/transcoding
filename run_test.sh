time -p ffmpeg -i big.mp2 -ac 1 -ab 64000 output1.mp2
time -p ./fshEncode.out
time -p ./decoding_encoding big
