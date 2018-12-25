#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include "m2_vad.h"
#include "vad/webrtc_vad.h"
#include "m2_ns.h"
typedef struct
{
    int             s_level;
    int             sample_rate;
    int             s_check_num;
    int             e_check_num;
    int             frame_size;
    int             in_speech;
    int             check_count;
    int             now_check_size;
    int             init_state;
    VadInst*        vad_handle;
    M2NS*           ns_handle;
    unsigned char*  ns_buffer;
    int             ns_buffer_size;
    int*		    e_array;
    int*			s_array;
    unsigned char*  check_data;

} M2VADInst;

const int16_t g_vad_params[6][4][3] = {
        {
                { 6, 3, 2 },
                { 9, 5, 3 },
                { 94, 10000, 94 },
                { 1100, 1100, 1100 } //modify the second value to change the sensitivity
        },
        {
                { 6, 3, 2 },
                { 9, 5, 3 },
                { 94, 10000, 94 },
                { 1100, 1050, 1100 }
        },
        {
                { 6, 3, 2 },
                { 9, 5, 3 },
                { 94, 10000, 94 },
                { 1200, 1000, 1200 }
        },
        {
                { 6, 3, 2 },
                { 9, 5, 3 },
                { 94, 10000, 94 },
                { 1200, 950, 1200 }
        },
        {
                { 6, 3, 2 },
                { 9, 5, 3 },
                { 94, 10000, 94 },
                { 1200, 900, 1200 }
        },
        {
                { 6, 3, 2 },
                { 9, 5, 3 },
                { 94, 10000, 94 },
                { 1200, 850, 1200 }
        }
};

M2VAD* M2VAD_create()
{
    return calloc(sizeof(M2VADInst), 1);
}

int M2VAD_init(M2VAD* handle, int sample_rate, int sp_time, int ep_time, BOOL enableNS)
{
    if(NULL == handle || (sample_rate != 8000 && sample_rate != 16000 )
       || (sp_time < 50 || sp_time > 3000) || (ep_time < 50 || ep_time > 4000))
    {
        printf("ERROR: invalid params in M2VAD_init\n");
        return -1;
    }
    M2VADInst* rh = (M2VADInst*)handle;
    rh->sample_rate = sample_rate;
    rh->s_check_num = sp_time / 20;
    rh->e_check_num = ep_time / 20;
    rh->frame_size = 20 * (sample_rate / 1000) * 2;

    int *p =  (int*)calloc(rh->s_check_num, sizeof(int));
    if(!p)
    {
        printf("ERROR: malloc error in M2VAD_init\n");
        return -1;
    }
    rh->s_array = p;
    p = (int*)malloc(rh->e_check_num * sizeof(int));
    if(!p)
    {
        printf("ERROR: malloc error in M2VAD_init\n");
        return -1;
    }
    rh->e_array = p;
    memset(rh->e_array, 1, rh->e_check_num * sizeof(int));
    unsigned char* pc = (unsigned char*)malloc(M2_VAD_CHECK_DATA_MAX_SIZE);
    if(!pc)
    {
        printf("ERROR: malloc error in M2VAD_init\n");
        return -1;
    }
    rh->check_data = pc;

    if(0 != WebRtcVad_ValidRateAndFrameLength(sample_rate, rh->frame_size / 2))
    {
        printf("ERROR: error frame size in M2VAD_init\n");
        return -1;
    }
    if(enableNS)
    {
        rh->ns_handle = M2NS_create();
        M2NS_init(rh->ns_handle, sample_rate);
        rh->ns_buffer = (unsigned char*)malloc(rh->frame_size * 4);
        rh->ns_buffer_size = rh->frame_size * 4;
    }
    WebRtcVad_Create(&rh->vad_handle);
    WebRtcVad_Init(rh->vad_handle);
    M2VAD_set_sensitivity(rh, 2);

    rh->init_state = 1;
    return 0;
}

int M2VAD_set_sensitivity(M2VAD* handle, int level)
{
    M2VADInst* rh = (M2VADInst*)handle;
    if(NULL == rh || level < 0 || level > 5 || rh->init_state == 0)
    {
        return -1;
    }

    WebRtcVad_SetMode3Params(g_vad_params[level]);
    WebRtcVad_set_mode(rh->vad_handle, 3);
    return 0;
}
M2VADDetectResult M2VAD_process_for_ep(M2VAD* handle, void* data, unsigned int dataSize)
{
    M2VADInst* rh = (M2VADInst*)handle;
    M2VADDetectResult vadRet = M2_VAD_POINT_NORMAL;
    int vadCount = 0;
    int ret = 0;

    if(NULL == rh || rh-> init_state == 0 || dataSize < 0 || NULL == data)
    {
        return M2_VAD_ERROR_IN_PROCESS;
    }

    if (rh->now_check_size + dataSize >= M2_VAD_CHECK_DATA_MAX_SIZE)
    {
        printf("ERROR: overflow in M2VAD_process_for_ep frame size is:%d", dataSize);
        return M2_VAD_ERROR_IN_PROCESS;
    }

    memcpy(rh->check_data + rh->now_check_size, data, dataSize);
    rh->now_check_size += dataSize;

    int loopCount = rh->now_check_size / rh->frame_size;
    if (loopCount > 0)
    {
        int i = 0;
        short* p_frame = NULL;
        int frame_len = rh->frame_size / 2;
        for (; i < loopCount; i++)
        {
            if(rh->ns_handle)
            {
                ret = M2NS_process(rh->ns_handle,
                                   rh->check_data + i * rh->frame_size, rh->frame_size,
                                   rh->ns_buffer, rh->ns_buffer_size);

                if(ret != rh->frame_size)
                {
                    printf("ERROR: M2NS_process failed\n");
                    break;
                }
                p_frame = (short*)rh->ns_buffer;
            }
            else
            {
                p_frame = (short*)(rh->check_data + i * rh->frame_size);
            }

            ret = WebRtcVad_Process(rh->vad_handle, rh->sample_rate,
                                    p_frame, frame_len);
            rh->check_count += 1;
            if (ret == -1)
            {
                printf("ERROR: err in M2VAD_process_for_ep");
                break;
            }

            rh->e_array[rh->check_count % rh->e_check_num] = ret;

            if (rh->check_count >= rh->e_check_num)
            {
                if ((rh->check_count % 10) != 0)
                {
                    continue;
                }
                for (int j = 0; j < rh->e_check_num; j++)
                {
                    if (rh->e_array[j] == 0)
                    {
                        vadCount++;
                    }
                }
                if (vadCount* 100 / rh->e_check_num > 80)
                {
                    vadRet = M2_VAD_END_POINT_DETECTED;
                    break;
                }
            }
        }
        //update buffer
        if (i < loopCount)
        {
            i++;
        }
        rh->now_check_size = rh->now_check_size - i * rh->frame_size;
        memcpy(rh->check_data, rh->check_data + i * rh->frame_size, rh->now_check_size);
    }

    if(vadRet == M2_VAD_END_POINT_DETECTED)
    {
        rh->check_count = 0;
        if (rh->e_array)
        {
            memset(rh->e_array, 1, rh->e_check_num * sizeof(int));
        }
        if(rh->ns_handle)
        {
            M2NS_reset(rh->ns_handle);
        }
    }
    return vadRet;
}

M2VADDetectResult M2VAD_process(M2VAD* handle, void* data, unsigned int dataSize)
{
    M2VADInst* rh = (M2VADInst*)handle;
    M2VADDetectResult vadRet = M2_VAD_POINT_NORMAL;
    int vadCount = 0;
    int ret = 0;

    if(NULL == rh || rh-> init_state == 0 || dataSize < 0 || NULL == data)
    {
        return M2_VAD_ERROR_IN_PROCESS;
    }

    if (rh->now_check_size + dataSize >= M2_VAD_CHECK_DATA_MAX_SIZE)
    {
        printf("ERROR: overflow in M2VAD_process frame size is:%d", dataSize);
        return M2_VAD_ERROR_IN_PROCESS;
    }
    memcpy(rh->check_data + rh->now_check_size, data, dataSize);
    rh->now_check_size += dataSize;

    int loopCount = rh->now_check_size / rh->frame_size;
    if (loopCount > 0)
    {
        int i = 0;
        short* p_frame = NULL;
        int frame_len = rh->frame_size / 2;
        for (; i < loopCount; i++)
        {
            if(rh->ns_handle)
            {
                ret = M2NS_process(rh->ns_handle,
                                   rh->check_data + i * rh->frame_size, rh->frame_size,
                                   rh->ns_buffer, rh->ns_buffer_size);

                if(ret != rh->frame_size)
                {
                    printf("ERROR: M2NS_process failed\n");
                    break;
                }
                p_frame = (short*)rh->ns_buffer;
            }
            else
            {
                p_frame = (short*)(rh->check_data + i * rh->frame_size);
            }

            ret = WebRtcVad_Process(rh->vad_handle, rh->sample_rate,
                                    p_frame, frame_len);
            rh->check_count += 1;
            if (ret == -1)
            {
                printf("ERROR: err in M2VAD_send_data");
                break;
            }
            if (rh->in_speech)
            {
                rh->e_array[rh->check_count % rh->e_check_num] = ret;

                if (rh->check_count >= rh->e_check_num)
                {
                    if ((rh->check_count % 5) != 0)
                    {
                        continue;
                    }
                    for (int j = 0; j < rh->e_check_num; j++)
                    {
                        if (rh->e_array[j] == 0)
                        {
                            vadCount++;
                        }
                    }
                    if (vadCount* 100 / rh->e_check_num > 80)
                    {
                        vadRet = M2_VAD_END_POINT_DETECTED;
                        break;
                    }
                }
            }
            else
            {
                rh->s_array[rh->check_count % rh->s_check_num] = ret;

                if (rh->check_count >= rh->s_check_num)
                {
                    if ((rh->check_count % 5) != 0)
                    {
                        continue;
                    }

                    for (int j = 0; j < rh->s_check_num; j++)
                    {
                        if (rh->s_array[j] == 1)
                        {
                            vadCount++;
                        }
                    }
                    if (vadCount * 100 / rh->s_check_num > 80)
                    {
                        vadRet = M2_VAD_START_POINT_DETECTED;
                        break;
                    }
                }
            }
        }
        //Update buffer
        if (i < loopCount)
        {
            i++;
        }
        rh->now_check_size = rh->now_check_size - i * rh->frame_size;
        memcpy(rh->check_data, rh->check_data + i * rh->frame_size, rh->now_check_size);
    }
    if(vadRet == M2_VAD_END_POINT_DETECTED)
    {
        rh->check_count = 0;
        rh->in_speech = 0;
        if (rh->e_array)
        {
            memset(rh->e_array, 1, rh->e_check_num * sizeof(int));
        }
        if(rh->ns_handle)
        {
            M2NS_reset(rh->ns_handle);
        }
    }
    else if(vadRet == M2_VAD_START_POINT_DETECTED)
    {
        rh->check_count = 0;
        rh->in_speech = 1;
        if (rh->s_array)
        {
            memset(rh->s_array, 0, rh->s_check_num * sizeof(int));
        }
    }
    return vadRet;
}

int M2VAD_destroy(M2VAD* handle)
{
    M2VADInst* rh = (M2VADInst*)handle;
    if(!rh)
    {
        return -1;
    }
    if(rh->check_data)
    {
        free(rh->check_data);
    }
    if(rh->e_array)
    {
        free(rh->e_array);
    }
    if(rh->s_array)
    {
        free(rh->s_array);
    }
    if(rh->vad_handle)
    {
        WebRtcVad_Free(rh->vad_handle);
    }
    if(rh->ns_buffer)
    {
        free(rh->ns_buffer);
    }
    if(rh->ns_handle)
    {
        M2NS_destroy(rh->ns_handle);
    }
    free(rh);
    return 0;
}
