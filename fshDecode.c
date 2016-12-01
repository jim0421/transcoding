/*
 * modified by fsh @2016.10.16
 */

#include <math.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>

typedef struct fshEss
{
    int start;
    int end;
    char* inputName;
    char* outputName;
}fshEss;
static void fshDecode(void* args);
static int fshGetPeriod(char* filename);
static void fshAllocateThread(fshEss *ess, char* input);
#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096
#include "fshTranscode.h"
int main(int argc, char **argv)
{
	/* register all the codecs */
    avcodec_register_all();
    
    /* read in cmd args	*/
    if (argc < 2) {
        printf("usage: give the media to be decoded\n"\
        	   "for example: big\n"\
        	   "*.mp2 -> *.pcm\n");
        return 1;
    }
    /*char* output = (char *)malloc(sizeof(char)*100);*/
    char* input = (char *)malloc(sizeof(char)*100);
    /*strcpy(output, argv[1]);*/
    strcpy(input, argv[1]);
    /*strcat(input,".mp2");*/
	/*printf("input is %s and output is %s\n", input, output);*/
    
    /* initialize ess array */
    fshEss* ess = (fshEss*) (malloc(THREAD_CNT * sizeof(fshEss))); // not free
    
    /* allocate threads */
    fshAllocateThread(ess, input);
    int fshCount = THREAD_CNT;
#ifdef DEBUG
    /*printf("come into debug\n");*/
#undef THREAD_CNT
#define THREAD_CNT 1
#endif
    fshCount = THREAD_CNT;
 
    /* create threads */
    pthread_t* threads = (pthread_t*) (malloc(THREAD_CNT * sizeof(pthread_t)));
    
    for (int i = 0; i < THREAD_CNT; i++){
        pthread_create(&threads[i], NULL, (void * (*)(void *))fshDecode, \
            (void *)&ess[i]);
    }
    for (int i = 0; i < THREAD_CNT; i++){
        pthread_join(threads[i], NULL);
    }
}

static int fshGetPeriod(char* filename)
{
    FILE* fp = fopen(filename, "rb");
    fseek(fp, 0L, SEEK_END);
    int fz = ftell(fp);
    /*printf("pivot fp is @ position %d\n",fz);*/
    return fz;
}

/*
 * Audio decoding.
 */
static void fshDecode(void* args)
{
    /*fetch sample struct*/
    fshEss* es = (fshEss *)args;
    const char* filename = es->inputName;
    const char* outfilename = es->outputName;
    int fshBufferSize = es->end - es->start;
    printf(">>> check buffer size is %d\n", fshBufferSize);
    printf(">>> Decode audio file %s to %s\n", filename, outfilename);
   
    /*open codec context and avpkg*/
    AVCodec *codec;
    AVCodecContext *c= NULL;
    int len;
    FILE *f, *outfile;
    uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    AVPacket avpkt;
    AVFrame *decoded_frame = NULL;
    av_init_packet(&avpkt);

    /* find the MPEG audio decoder */
    codec = avcodec_find_decoder(AV_CODEC_ID_MP2);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }
    
    outfile = fopen(outfilename, "wb");
    if (!outfile) {
        av_free(c);
        exit(1);
    }
    
    fseek(f, es->start, SEEK_SET);
    printf(">>> the start point is %ld\n", ftell(f));
    avpkt.data = inbuf;
    avpkt.size = fread(inbuf, 1, AUDIO_INBUF_SIZE, f);
    fshBufferSize -= AUDIO_INBUF_SIZE;
    int fillBufferSize;

    //$$ suppose first inbuf is always correct, need checking
    while (avpkt.size > 0) {
        /*printf("pivot avpkt size is %d\n", avpkt.size);*/
        // every avpkt has same size
        int i, ch;
        int got_frame = 0;
        
        /*step one allocate decode frame*/
        if (!decoded_frame) {
            if (!(decoded_frame = av_frame_alloc())) {
                fprintf(stderr, "Could not allocate audio frame\n");
                exit(1);
            }
        }
        /*step two decode a frame from an avpkt to a frame */ 
        len = avcodec_decode_audio4(c, decoded_frame, &got_frame, &avpkt);
        /*printf("pivot len is %d\n", len);*/
        if (len < 0) {
            fprintf(stderr, "Error while decoding\n");
            exit(1);
        }
        if (got_frame) {
            /*printf("pivot inside got_frame\n");*/
            /* if a frame has been decoded, output it */
            int data_size = av_get_bytes_per_sample(c->sample_fmt);
            if (data_size < 0) {
                /* This should not occur, checking just for paranoia */
                fprintf(stderr, "Failed to calculate data size\n");
                exit(1);
            }
            for (i=0; i<decoded_frame->nb_samples; i++){
                for (ch=0; ch<c->channels; ch++){
                    fwrite(decoded_frame->data[ch] + data_size*i, 1, \
                        data_size, outfile);
                }
            }
        }
        avpkt.size -= len;
        avpkt.data += len;
        /*printf("fshPivot len is %d\n", len);*/
        //20480
        avpkt.dts = avpkt.pts = AV_NOPTS_VALUE;
        if (avpkt.size < AUDIO_REFILL_THRESH) {//4096
            /* Refill the input buffer, to avoid trying to decode
             * incomplete frames. Instead of this, one could also use
             * a parser, or use a proper container format through
             * libavformat. */
            memmove(inbuf, avpkt.data, avpkt.size);
            avpkt.data = inbuf;
            /* decode until f goes to ess.end */
            if (fshBufferSize < (AUDIO_INBUF_SIZE-avpkt.size)){// no enough words 
                fillBufferSize = fshBufferSize;
                fshBufferSize = 0;
            }else{
                fillBufferSize = AUDIO_INBUF_SIZE-avpkt.size;
                fshBufferSize -= fillBufferSize;
            }
            len = fread(avpkt.data + avpkt.size, 1, fillBufferSize, f);
            /*len = fread(avpkt.data + avpkt.size, 1,\ */
                /*AUDIO_INBUF_SIZE-avpkt.size, f);*/
            if (len > 0)
                avpkt.size += len;
            else
                printf("come to end of file\n");
                /*$$may be error here*/
        }
    }

    fclose(outfile);
    fclose(f);

    avcodec_close(c);
    av_free(c);
    av_frame_free(&decoded_frame);
}

static void fshAllocateThread(fshEss *ess, char* input)
{
    char* fileName = (char *)malloc(sizeof(char) * STRING_SIZE);
    strcpy(fileName, input);
    /*can change type*/
    strcat(fileName, ".mp2");
    int fileEnd = fshGetPeriod(fileName);
    printf("fileEnd is %d\n", fileEnd);

    /*average to thread num*/
    int avgLen = fileEnd / THREAD_CNT;
    char* tmpChar;
    for (int i = 0; i < THREAD_CNT-1; i++){
        ess[i].start = i * avgLen;
        ess[i].end = (i+1) * avgLen;
        ess[i].inputName = newString;
        strcpy(ess[i].inputName, fileName);
        ess[i].outputName = newString;
        strcpy(ess[i].outputName, input);
        tmpChar = newString;
        sprintf(tmpChar, "_%d.pcm",i);
        strcat(ess[i].outputName, tmpChar);
        free(tmpChar);
    }
    ess[THREAD_CNT-1].start = (THREAD_CNT-1) * avgLen;//last one
    ess[THREAD_CNT-1].end = fileEnd;
    ess[THREAD_CNT-1].inputName = newString;
    strcpy(ess[THREAD_CNT-1].inputName, fileName);
    ess[THREAD_CNT-1].outputName = newString;
    strcpy(ess[THREAD_CNT-1].outputName, input);
    tmpChar = newString;
    sprintf(tmpChar, "_%d.pcm",THREAD_CNT-1);
    strcat(ess[THREAD_CNT-1].outputName, tmpChar);
    free(tmpChar);

/*
 *    ess[0].start = 1000000; ess[0].end = 2000000;
 *    //$$fucking bug here, broken header when decode audio
 *    ess[0].inputName = (char *)malloc(sizeof(char) * STRING_SIZE);
 *    [>ess[0].inputName = "big_1.mp2";<]
 *    strcpy(ess[0].inputName, input);
 *    strcat(ess[0].inputName, "_1.mp2\0");
 *    ess[0].inputName = "big.mp2";
 *    [>strcat(ess[0].inputName, input); <]
 *    //$$funking bug here
 *    ess[0].outputName = newString;    
 *    strcpy(ess[0].outputName, output); 
 *    strcat(ess[0].outputName, "_1.pcm");
 *    ess[0].outputName = "big.pcm";
 *
 *
 *    ess[1].start = 1000000; ess[1].end = 2000000;
 *    //%%funking answer is I set ess[0] again here, but why
 *    ess[1].inputName = (char *) (malloc(40 * sizeof(char)));
 *    strcpy(ess[1].inputName, input); 
 *    strcat(ess[1].inputName, "_2.mp2");
 *    ess[1].outputName = (char *) (malloc(40 * sizeof(char)));
 *    strcpy(ess[1].outputName, input); 
 *    strcat(ess[1].outputName, "_2.pcm");
 */
    
}
