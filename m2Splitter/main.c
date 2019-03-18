//
// Created by xiaofeng on 7/5/17.
//

#include <m2_vad/m2_vad.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <dirent.h>
#include "PCMHeader.h"

#define MAX_FRAME_SIZE 1024

typedef struct
{
    unsigned char * frameData;
    unsigned int frameSize;
} AudioFrame;

char g_empty_wav_header[44] = {0};
void showUsage()
{
    printf("usage:\n");
    printf("\tm2splitter -l vad_level -i input_file -o output_directory\n\n");
    printf("\tvad_level: (0~5 default is 2),In a noisy environment, set the higher the value\n");
    printf("\tinput_file: wav file(only support:16bit, mono channel\n\t\t16000 or 8000 sample rate)\n");
    printf("\toutput_directory: the output audio files will write in this directory\n\t\tif it's not exists, m2splitter will auto create it.\n");
}
int isDirectoryExist(char* path)
{
    if(!path)
    {
        return -1;
    }
    DIR* test = opendir(path);
    if(test)
    {
        closedir(test);
        return 0;
    }
    return -1;
}
int createDirectory(char* path)
{
    if(!path)
    {
        return -1;
    }
    char temp[256];
    sprintf(temp, "mkdir -p %s", path);
    return system(temp);
}
void parseArgs(int argc, char* argv[], char** input_file, char** out_folder, int* vad_level)
{
    int find_in = 0;
    int find_out = 0;
    *vad_level = 2;
    if(argc < 5)
    {
        showUsage();
        exit(0);
    }
    for(int i = 1; i < argc - 1; i++)
    {
        if(strcmp(argv[i], "-o") == 0)
        {
            if(strcmp(argv[i + 1], "-i") != 0 && strcmp(argv[i + 1], "-l") != 0 )
            {
                find_out = 1;
                *out_folder = argv[i + 1];
            }
        }
        else if(strcmp(argv[i], "-i") == 0)
        {
            if(strcmp(argv[i + 1], "-o") != 0 && strcmp(argv[i + 1], "-l") != 0 )
            {
                find_in = 1;
                *input_file = argv[i + 1];
            }
        }
        else if(strcmp(argv[i], "-l") == 0)
        {
            if(strcmp(argv[i + 1], "-o") != 0 && strcmp(argv[i + 1], "-i") != 0 )
            {
                int level = atoi(argv[i + 1]);
                if(level >= 0 && level <= 5)
                {
                    *vad_level = level;
                }
            }
        }
    }
    if(!find_in || !find_out)
    {
        showUsage();
        exit(0);
    }
}
int main(int argc, char* argv[])
{

    char* inputFile = NULL;
    char* outputFolder = NULL;
    int vad_level = 0;
    parseArgs(argc, argv, &inputFile, &outputFolder, &vad_level);

    if(0 != isDirectoryExist(outputFolder))
    {
        if(0 != createDirectory(outputFolder))
        {
            printf("ERROR: can't create output directory :%s\n", outputFolder);
            exit(1);
        }
    }
    //load pcm data(仅支持16bit 8000或者16000采样率的wav文件)
    FILE* fp = fopen(inputFile, "rb");
    if(!fp)
    {
        printf("ERROR: can't open input file:%s\n", inputFile);
        exit(1);
    }

    WAVEHEADFMT inputWavHeader = {0};
    if(ReadWaveHead(&inputWavHeader, fp) != 0)
    {
        printf("ERROR: read wav header error\n");
        exit(1);
    }

    if(inputWavHeader.nBitPerSample != 16 || inputWavHeader.nChannels != 1 ||
            (inputWavHeader.nSamplesPerSec != 8000 && inputWavHeader.nSamplesPerSec != 16000))
    {
        printf("ERROR： unsupport wav format bit:%d, sample:%d, channel:%d\n"
                ,inputWavHeader.nBitPerSample, (int)inputWavHeader.nSamplesPerSec, inputWavHeader.nChannels);
        exit(1);
    }
    else
    {
        printf("input file format bit:%d, sample:%d, channel:%d, vad level:%d\n"
                ,inputWavHeader.nBitPerSample, (int)inputWavHeader.nSamplesPerSec, inputWavHeader.nChannels, vad_level);
    }
    fseek(fp, 0L, SEEK_END);
    long audioSize = ftell(fp);
    audioSize -= 44;
    fseek(fp, 44, SEEK_SET);
    //模拟音频流，在实时环境使用时可根据最大录音长度将音频流设置为环形buffer
    //这里为了简单直接读取所有数据存入数组
    int audio_stream_size = audioSize / MAX_FRAME_SIZE + 1;
    AudioFrame* audio_stream = (AudioFrame*) calloc(audio_stream_size, sizeof(AudioFrame));

    unsigned char temp[MAX_FRAME_SIZE];
    int index = 0;
    int new_file_index = 1;
    while(!feof(fp))
    {
        size_t readSize = fread(temp, 1, MAX_FRAME_SIZE, fp);
        if(readSize)
        {
            audio_stream[index].frameData = (unsigned char *)malloc(readSize);
            memcpy(audio_stream[index].frameData, temp, readSize);
            audio_stream[index].frameSize = readSize;
            index += 1;
        }
    }
    fclose(fp);
    //进行VAD检测
    M2VAD * vad_handle = M2VAD_create();
    if(!vad_handle)
    {
        printf("ERROR: M2VAD_create failed\n");
        exit(1);
    }
    //起点检测时间300ms 尾点的检测时间为900ms
    if(0 != M2VAD_init(vad_handle, (int)inputWavHeader.nSamplesPerSec, 300, 900, TRUE))
    {
        printf("ERROR: M2VAD_init failed\n");
        exit(1);
    }
    //取值范围(0-5)环境越嘈杂需要设置这个值越高
    M2VAD_set_sensitivity(vad_handle, vad_level);
    //由于检测到声音的起点的时候，已经过了一小段检测时间，所以需要
    //回溯一定的数据才能保证截取到的时完整的音频文件
    int sb_frames = (((300 + 200) * ((int)inputWavHeader.nSamplesPerSec / 1000) * 2) / MAX_FRAME_SIZE); //起点需要回溯的音频帧数,留有余地
    int eb_frames = ((900 * ((int)inputWavHeader.nSamplesPerSec / 1000) * 2) / MAX_FRAME_SIZE); //尾点需要回溯的音频帧数

    int real_start_index = 0;
    int real_end_index = 0;

    for(int i = 0; i < audio_stream_size; i++)
    {
        if(audio_stream[i].frameData)
        {
            M2VADDetectResult res = M2VAD_process(vad_handle, audio_stream[i].frameData,
                                                    audio_stream[i].frameSize);
            if(res == M2_VAD_ERROR_IN_PROCESS)
            {
                printf("ERROR: M2VAD_process failed\n");
                exit(1);
            }
            else if(res == M2_VAD_START_POINT_DETECTED)
            {
                if(i - sb_frames > 0)
                {
                    real_start_index = i - sb_frames;
                }
                else
                {
                    real_start_index = 0;
                }
            }
            else if(res == M2_VAD_END_POINT_DETECTED)
            {
                if(i - eb_frames > real_start_index)
                {
                    real_end_index = i - eb_frames;
                }
                else
                {
                    printf("err in cult end point\n");
                    exit(1);
                }


                if(real_end_index > real_start_index)
                {
                    sprintf((char*)temp, "%s/%d.wav", outputFolder, new_file_index);
                    fp = fopen(temp, "wb+");
                    if(!fp)
                    {
                        printf("ERROR: can't create out file.\n");
                        continue;
                    }
                    fwrite(g_empty_wav_header, 1, 44, fp);
                    int writeSize = 0;
                    for(int j = real_start_index; j <= real_end_index; j ++)
                    {
                        if(audio_stream[j].frameSize != fwrite(audio_stream[j].frameData, 1, audio_stream[j].frameSize, fp))
                        {
                            printf("ERROR: write file error.\n");
                            break;
                        }
                        writeSize += audio_stream[j].frameSize;
                    }
                    WAVEHEADFMT newWavHeader;
                    PrepareWaveHead(&newWavHeader, 1, inputWavHeader.nSamplesPerSec, 16, writeSize);
                    if(0 != WriteWaveHead(&newWavHeader, fp))
                    {
                        fclose(fp);
                        printf("ERROR: write wav header failed.\n");
                        continue;
                    }
                    fclose(fp);
                    new_file_index += 1;
                }
            }
        }
        else
        {
            break;
        }
    }
    for(int i = 0; i < audio_stream_size; i++)
    {
        if(audio_stream[i].frameData)
        {
            free(audio_stream[i].frameData);
        }
    }
    free(audio_stream);
    printf("Finished, split out %d files to \"%s\"\n", new_file_index - 1, outputFolder);
    return 0;
}
