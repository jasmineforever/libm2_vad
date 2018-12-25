#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include "m2_ns.h"
#include "ns/noise_suppression.h"
#define DEFAULT_NS_BUFFER_SIZE 4096
typedef struct
{
    int             sample_rate;
    int             frame_size_in_byte; //10ms
    int             init_state;
    NsHandle*       ns_handle;
    unsigned char*  data_buffer;
    int             data_buffer_size;
    int             data_buffer_len;
    short*          frame_buffer;

} M2NSInst;

M2NS* M2NS_create()
{
    int size = sizeof(M2NSInst);
    return calloc(size, 1);
}
int M2NS_init(M2NS* handle, int sample_rate)
{
    M2NSInst* rh = (M2NSInst*)handle;
    if(NULL == rh || (sample_rate != 8000 && sample_rate != 16000 ))
    {
        printf("ERROR: invalid params in M2NS_init\n");
        return -1;
    }

    WebRtcNs_Create(&rh->ns_handle);
    WebRtcNs_Init(rh->ns_handle, sample_rate);
    WebRtcNs_set_policy(rh->ns_handle, 2);
    rh->init_state = 1;
    rh->frame_size_in_byte = (sample_rate / 1000) * 10 * 2;
    rh->data_buffer = malloc(DEFAULT_NS_BUFFER_SIZE);
    rh->data_buffer_size = DEFAULT_NS_BUFFER_SIZE;
    rh->frame_buffer = (short*)malloc(rh->frame_size_in_byte);
    return 0;
}
int M2NS_process(M2NS* handle, void* data, unsigned int dataSize, void* pOutBuffer, unsigned int out_buf_size)
{
    int ret_len = 0;
    M2NSInst* rh = (M2NSInst*)handle;
    if(NULL == rh || rh-> init_state == 0 || dataSize <= 0 || NULL == data)
    {
        printf("ERROR: invalid params in M2NS_process\n");
        return -1;
    }
    int new_size = rh->data_buffer_len + dataSize;
    if(new_size > out_buf_size)
    {
        return -2;
    }
    else if(new_size < rh->frame_size_in_byte)
    {
        memcpy(rh->data_buffer + rh->data_buffer_len, data, dataSize);
        rh->data_buffer_len = new_size;
        return 0;
    }
    else if (new_size > rh->data_buffer_size)
    {
        rh->data_buffer = realloc(rh->data_buffer, new_size + DEFAULT_NS_BUFFER_SIZE);
        memcpy(rh->data_buffer + rh->data_buffer_len, data, dataSize);
        rh->data_buffer_len += dataSize;
        rh->data_buffer_size = new_size + DEFAULT_NS_BUFFER_SIZE;
    }
    else
    {
        memcpy(rh->data_buffer + rh->data_buffer_len, data, dataSize);
        rh->data_buffer_len = new_size;
    }
    // frame size  = 10ms audio

    int slices = rh->data_buffer_len  / rh->frame_size_in_byte;
    int remains = rh->data_buffer_len % rh->frame_size_in_byte;

    for(int i = 0; i < slices; i++)
    {
        short* audioFrame = (short*)(rh->data_buffer + i * rh->frame_size_in_byte);
        WebRtcNs_Process(rh->ns_handle, audioFrame, audioFrame, rh->frame_buffer, rh->frame_buffer);
        memcpy(pOutBuffer + ret_len, rh->frame_buffer, rh->frame_size_in_byte);
        ret_len += rh->frame_size_in_byte;
    }
    memcpy(rh->data_buffer, rh->data_buffer + rh->frame_size_in_byte * slices, remains);
    rh->data_buffer_len = remains;
    return ret_len;
}
int M2NS_reset(M2NS* handle)
{
    M2NSInst* rh = (M2NSInst*)handle;
    if(!rh)
    {
        return -1;
    }
    rh->data_buffer_len = 0;
}
int M2NS_destroy(M2NS* handle)
{
    M2NSInst* rh = (M2NSInst*)handle;
    if(!rh)
    {
        return -1;
    }
    if(rh->ns_handle)
    {
        WebRtcNs_Free(rh->ns_handle);
        rh->ns_handle = NULL;
    }
    if(rh->data_buffer)
    {
        free(rh->data_buffer);
        rh->data_buffer = NULL;
    }
    if(rh->frame_buffer)
    {
        free(rh->frame_buffer);
        rh->frame_buffer = NULL;
    }
    free(rh);
    return 0;
}