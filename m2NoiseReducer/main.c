//
// Created by xiaofeng on 7/5/17.
//
#include <m2_vad/m2_ns.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "PCMHeader.h"

#define MAX_FRAME_SIZE 1600

unsigned char g_out_frame_buffer[MAX_FRAME_SIZE * 2];

struct {
    unsigned char* p;
    int size;
    int len;
}   g_out_buffer = {NULL, 0, 0};



void showUsage()
{
    printf("usage:\n");
    printf("\tm2NoiseReducer -i input_file -o output_file\n\n");
    printf("\tinput_file: wav file(only support:16bit, mono channel\n\t\t16000 or 8000 sample rate)\n");
}

void parseArgs(int argc, const char* argv[], char** input_file, char** out_file_name)
{
    int find_in = 0;
    int find_out = 0;
    if(argc != 5)
    {
        showUsage();
        exit(0);
    }
    for(int i = 1; i < argc - 1; i++)
    {
        if(strcmp(argv[i], "-o") == 0)
        {
            if(strcmp(argv[i + 1], "-i") != 0)
            {
                find_out = 1;
                int len = strlen(argv[i + 1]) + 1;
                *out_file_name = calloc(len, 1);
                memcpy(*out_file_name, argv[i + 1], len - 1);
            }
        }
        else if(strcmp(argv[i], "-i") == 0)
        {
            if(strcmp(argv[i + 1], "-o") != 0)
            {
                find_in = 1;
                int len = strlen(argv[i + 1]) + 1;
                *input_file = calloc(len, 1);
                memcpy(*input_file, argv[i + 1], len - 1);
            }
        }
    }
    if(!find_in || !find_out)
    {
        showUsage();
        exit(0);
    }
}
int main(int argc, const char* argv[])
{
    char* inputFile = NULL;
    char* outputFile = NULL;

    parseArgs(argc, argv, &inputFile, &outputFile);
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
        printf("ERROR： unsupport wav format. bit:%d, sample:%d, channel:%d\n"
                ,inputWavHeader.nBitPerSample, (int)inputWavHeader.nSamplesPerSec, inputWavHeader.nChannels);
        exit(1);
    }
    else
    {
        printf("input file format bit:%d, sample:%d, channel:%d\n"
                ,inputWavHeader.nBitPerSample, (int)inputWavHeader.nSamplesPerSec, inputWavHeader.nChannels);
    }
    fseek(fp, 0L, SEEK_END);
    long audioSize = ftell(fp);
    audioSize -= 44;
    fseek(fp, 44, SEEK_SET);
    unsigned char* input_buffer = malloc(audioSize);
    if(audioSize != fread(input_buffer, 1, audioSize, fp))
    {
        printf("failed to read input data\n");
        exit(1);
    }
    fclose(fp);
    //进行降噪
    M2NS *ns_handle = M2NS_create();
    if(!M2NS_create)
    {
        printf("ERROR: M2NS_create failed\n");
        exit(1);
    }
    if(0 != M2NS_init(ns_handle, (int)inputWavHeader.nSamplesPerSec))
    {
        printf("ERROR: M2NS_init failed\n");
        exit(1);
    }
    int frames = audioSize / MAX_FRAME_SIZE;
    for(int i = 0; i < frames; i++)
    {
        int ret = M2NS_process(ns_handle, input_buffer + i * MAX_FRAME_SIZE,
                               MAX_FRAME_SIZE, g_out_frame_buffer, MAX_FRAME_SIZE *2);
        if(ret < 0)
        {
            printf("ERROR: M2NS_process failed\n");
            exit(1);
        }
        if(!g_out_buffer.p)
        {
            g_out_buffer.size = ret * 50;
            g_out_buffer.p = malloc(g_out_buffer.size);
            memcpy(g_out_buffer.p, g_out_frame_buffer, ret);
            g_out_buffer.len = ret;
        }
        else
        {
            if(ret + g_out_buffer.len > g_out_buffer.size)
            {
                g_out_buffer.size += ret * 50;
                g_out_buffer.p = realloc(g_out_buffer.p, g_out_buffer.size);
                memcpy(g_out_buffer.p + g_out_buffer.len, g_out_frame_buffer, ret);
            }
            else
            {
                memcpy(g_out_buffer.p + g_out_buffer.len, g_out_frame_buffer, ret);
            }
            g_out_buffer.len += ret;
        }
    }
    M2NS_destroy(ns_handle);
    if(g_out_buffer.len > 100)
    {
        FILE* fp1 = fopen(outputFile, "wb+");
        if(!fp1)
        {
            printf("ERROR: can't create out file.\n");
            exit(1);
        }
        WAVEHEADFMT outHeader = {0};
        PrepareWaveHead(&outHeader, 1, inputWavHeader.nSamplesPerSec, inputWavHeader.nBitPerSample, g_out_buffer.len);
        WriteWaveHead(&outHeader, fp1);
        if(g_out_buffer.len != fwrite(g_out_buffer.p, 1, g_out_buffer.len, fp1))
        {
            printf("ERROR: write file failed.\n");
            exit(1);
        }
        fclose(fp1);
        printf("Finished, Noise-reduced audio files save to \"%s\"\n", outputFile);
        free(input_buffer);
        free(g_out_buffer.p);
    }
    return 0;
}
