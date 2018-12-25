#ifndef M2_VAD_H_
#define M2_VAD_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void M2VAD;

#define M2_VAD_CHECK_DATA_MAX_SIZE 9600
typedef enum
{
    M2_VAD_ERROR_IN_PROCESS = -1,
    M2_VAD_POINT_NORMAL = 0,
    M2_VAD_START_POINT_DETECTED,
    M2_VAD_END_POINT_DETECTED,
} M2VADDetectResult;

#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
// create M2 VAD instance.
//
// returns      : the pointer of M2 VAD instance
M2VAD* M2VAD_create();

// initialize  M2 VAD instance.
//
// - handle     : Pointer to VAD instance
// - sampleRate : input pcm data stream sample rate 8000 or 16000
// - sp_time    : the audio start point check time in milliseconds(recommend 200 ~ 400)
// - ep_time    : the audio end point check time in milliseconds(recommend 800 ~ 1200)
// - enable   : If set to TRUE, the noise suppression function will be turned on.
// returns      : 0 - (OK), -1 - (ERROR)
int M2VAD_init(M2VAD* handle, int sample_rate, int sp_time, int ep_time, BOOL enableNS);

// set the  M2 VAD sensitivity.(Be sure to call this function after call M2VAD_init)
//
// - handle     : Pointer to VAD instance
// - level      : (0~5, default is 2) The greater the value, the higher the sensitivity
// returns      : 0 - (OK), -1 - (ERROR)
int M2VAD_set_sensitivity(M2VAD* handle, int level);

// do process only for endpoint detect
//
// - data       : the stream pcm data buffer
// - dataSize   : the stream pcm data buffer size
// returns      : result
M2VADDetectResult M2VAD_process_for_ep(M2VAD* handle, void* data, unsigned int dataSize);

// do process  both for start point and endpoint detect
//
// - data       : the stream pcm data buffer
// - dataSize   : the stream pcm data buffer size
// returns      : result
M2VADDetectResult M2VAD_process(M2VAD* handle, void* data, unsigned int dataSize);

// destroy M2 VAD instance
//
// - handle     : the pointer of M2 VAD instance
// returns      : 0 - (OK), -1 - (ERROR)
int M2VAD_destroy(M2VAD* handle);

#ifdef __cplusplus
}
#endif
#endif //M2_VAD_H_