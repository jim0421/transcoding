#MP4Box -sbr -add fsh/big.ac3#audio -add big.mp4#video -new fshNewBig.mp4
#MP4Box -sbr -add fsh/big.aac#audio -add big.mp4#video -new fshNewBig.mp4
#MP4Box -sbr -add fsh/big.mp2#audio -add big.mp4#video -new fshNewBig.mp4
MP4Box -sbr -add finalOutput.mp2#audio -add big.mp4#video -delay 2=4398 -new fshNewBig.mp4
#MP4Box -sbr -add fsh/big.mp2#audio -add ../Transcoder/101.mp4#video -delay 1=4398 -new fshNewBig.mp4
#MP4Box -sbr -add fsh/big.mp2#audio -add ../Transcoder/101.mp4#video -delay 2=4398 -new fshNewBig.mp4
#MP4Box -sbr -add big_header_missing.mp2#audio -add ../Transcoder/101.mp4#video -delay 1=4398 -new fshNewBig.mp4
