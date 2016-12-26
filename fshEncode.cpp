/*
 *version 1.0 finished by fsh@2016.10.24
 */

extern "C"  
{  
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <unistd.h>
#include <pthread.h>
};  
#include "fshTranscode.h"
#define FILE_END -1
typedef struct fshEss
{
    int start;
    int end;
    char* inputName;
    char* outputName;
}fshEss;
int fshEncode(void *args);
static void fshAllocateThread(fshEss *ess, char* input);

int main(int argc, char* argv[])
{
    fshEss* ess = (fshEss*) (malloc(THREAD_CNT * sizeof(fshEss))); // not free
    
    /* read in cmd args	*/
    if (argc < 2) {
        printf("usage: give the pcm to be encoded\n"\
        	   "for example: big\n"\
        	   "*.pcm -> *.mp2\n");
        return 1;
    }
    /*char* output = (char *)malloc(sizeof(char)*100);*/
    char* input = newString;
    /*strcpy(output, argv[1]);*/
    strcpy(input, argv[1]);   
    /* allocate threads */
    fshAllocateThread(ess, input);
    //initialize ess 0



/*
 *    ess[0].start = 0; ess[0].end = FILE_END;
 *    ess[0].inputName = (char *) (malloc(40 * sizeof(char)));
 *    ess[0].inputName = "big.pcm";
 *    ess[0].outputName = (char *) (malloc(40 * sizeof(char)));
 *    ess[0].outputName = "fsh/big.mp2";
 *
 *    ess[1].start = 100000; ess[1].end = 200000;
 *    ess[1].inputName = (char *) (malloc(40 * sizeof(char)));
 *    ess[1].inputName = "mp2_90.pcm";
 *    ess[1].outputName = (char *) (malloc(40 * sizeof(char)));
 *    ess[1].outputName = "mp2_90_output_2.mp2";
 */
    
    // 初始化注册FFMPEG以供使用  
    av_register_all();  

    pthread_t* threads = (pthread_t*) malloc(THREAD_CNT * sizeof(pthread_t));
    for (int i = 0; i < THREAD_CNT; i++)
    {
        pthread_create(&threads[i], NULL, (void * (*)(void *))fshEncode, \
            (void *)&ess[i]);
    }
    for (int i = 0; i < THREAD_CNT; i++)
    {
        pthread_join(threads[i], NULL);
    }
    return 0;
}

int fshEncode(void* args)
{  
    fshEss* es = (fshEss *)args;
    printf("pivot start time is %d\n", es->start);
    AVFormatContext* pFormatCtx;  
    AVOutputFormat* fmt;  
    AVStream* audio_st;  
    AVCodecContext* pCodecCtx;  
    AVCodec* pCodec;  
  
    uint8_t* frame_buf;  
    AVFrame* frame;  
    int size;  
  
    FILE *fileIn = fopen(es->inputName, "rb");  
    char* fileOut = es->outputName;
  
  
    // 解码文件格式  
    avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, fileOut);  
    fmt = pFormatCtx->oformat;  
  
    //注意输出路径  
    if (avio_open(&pFormatCtx->pb, fileOut, AVIO_FLAG_READ_WRITE) < 0)    
    {  
        printf("输出文件打开失败！\n");  
        return -1;  
    }  
  
    audio_st = avformat_new_stream(pFormatCtx, 0);  
    if (audio_st == NULL)  
    {  
        printf("新建流失败!!!\n");  
        return -1;  
    }  
  
    // 设定转码信息  
    pCodecCtx = audio_st->codec;  
    pCodecCtx->codec_id = fmt->audio_codec;  
    pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;  
    //pCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;  
    pCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;  
    pCodecCtx->sample_rate= 48000;               // 音频的采样率  
    pCodecCtx->channel_layout=AV_CH_LAYOUT_5POINT1;  
    //pCodecCtx->channel_layout=AV_CH_LAYOUT_STEREO;  
    pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);  
    pCodecCtx->bit_rate = 64000;             // 音频的比特率  
  
    //调试输出格式信息  
    av_dump_format(pFormatCtx, 0, fileOut, 1);  
  
    pCodec = avcodec_find_encoder(AV_CODEC_ID_DTS);
    if (!pCodec)  
    {  
        printf("找不到输入文件所需的编码器!\n");  
        return -1;
    }  
    if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0)  
    {  
        printf("打开编码器失败!\n");
        return -1;
    }  
  
    frame = av_frame_alloc();  
    frame->nb_samples= pCodecCtx->frame_size;  
    frame->format= pCodecCtx->sample_fmt;  

    size = av_samples_get_buffer_size(NULL, pCodecCtx->channels,\
        pCodecCtx->frame_size,pCodecCtx->sample_fmt, 1);  
    frame_buf = (uint8_t *)av_malloc(size);
    avcodec_fill_audio_frame(frame, pCodecCtx->channels, \
        pCodecCtx->sample_fmt,(const uint8_t*)frame_buf, size, 1);  
      
    // 填充输出的头文件信息  
    avformat_write_header(pFormatCtx,NULL);  
    /*
     *int f_size = 0;
     *int i = es->start;
     *goto omit_encode_body;
     */
    AVPacket pkt;  
    av_new_packet(&pkt,size);  
    if (fseek(fileIn, es->start * size, SEEK_SET) != 0)
    {
        printf("fseek error\n");
    }
    printf("pivot initial size is %d\n", size); 
    printf("first ftell is %ld\n",ftell(fileIn));
    int f_size = 0;
    int i = es->start;
    for (i; ; i++)
    {
        //printf("pivot >>> i is %d\n",i);
        // read in PCM by size
        /*
         *if(i >= es->end)
         *{
         *    break;
         *}
         */

        /*
         *if (i < 10){
         *    printf("pivot ftell is %ld\n",ftell(fileIn));
         *}
         */
        f_size = fread(frame_buf, 1, size, fileIn);
        //printf("pivot f_size is %d\n", f_size); 
        /*
         *if (f_size != 4608)
         *{
         *    printf("fshPivot size is not 4608\n");
         *}
         */
        if (f_size < 0)
        {
            printf("文件读取错误！\n");  
            return -1;  
        }
        else if(feof(fileIn))  
        {
            printf("End of file\n");
            break;  
        }
        frame->data[0] = frame_buf;  //采样信号  

        frame->pts=i*100;  
        int got_frame=0;  
        //编码  
        int ret = avcodec_encode_audio2(pCodecCtx, &pkt,frame, &got_frame);  
        if(ret < 0)  
        {
            printf("编码错误！\n");  
            return -1;  
        }
        //多个frame一个pkt会有影响么
        if (got_frame==1)  
        {  
            pkt.stream_index = audio_st->index;
            //$$audio_st->index remains zero
            ret = av_write_frame(pFormatCtx, &pkt);  
            av_free_packet(&pkt);  
        }  
    }  
omit_encode_body:      
    //写文件尾  
    av_write_trailer(pFormatCtx);  
  
    //清理  
    if (audio_st)  
    {  
        avcodec_close(audio_st->codec);  
        av_free(frame);  
        av_free(frame_buf);  
    }  
    avio_close(pFormatCtx->pb);  
    avformat_free_context(pFormatCtx);  
    fclose(fileIn);  
  
    printf("转码完毕！！！\n");  
    return 0;  
}

static void fshAllocateThread(fshEss *ess, char* input)
{
    //write to audio list
    FILE *outputList = fopen("audioList.txt", "wb");
    char* tmpChar;
    for (int i = 0; i < THREAD_CNT; i++){
        ess[i].start = 0;
        ess[i].end = FILE_END; 
        ess[i].inputName = newString;
        strcpy(ess[i].inputName, input);
        tmpChar = newString;
        sprintf(tmpChar, "_%d.pcm",i);
        strcat(ess[i].inputName, tmpChar);
        free(tmpChar);
        ess[i].outputName = newString;
        strcpy(ess[i].outputName, input);
        tmpChar = newString;
        //sprintf(tmpChar, "_%d.mp2",i);
        sprintf(tmpChar, "_%d.dts",i);
        strcat(ess[i].outputName, tmpChar);
        free(tmpChar);
        tmpChar = newString;
        fprintf(outputList, "file '%s'\n", ess[i].outputName);
        free(tmpChar); 
    }
}

