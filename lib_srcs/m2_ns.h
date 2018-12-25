#ifndef M2_NS_H_
#define M2_NS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void M2NS;

// create M2 NS instance.
//
// returns      : the pointer of M2 noise suppression instance
M2NS* M2NS_create();

// initialize  M2 NS instance.
//
// - handle     : Pointer to noise suppression instance
// - sampleRate : input pcm data stream sample rate 8000 or 16000
// returns      : 0 - (OK), -1 - (ERROR)
int M2NS_init(M2NS* handle, int sample_rate);

// do noise suppressing
//
// - handle     : Pointer to noise suppression instance
// - data       : the stream pcm data buffer
// - dataSize   : the stream pcm data buffer size(must > input data size)
// returns      : > 0 -(the return buf size), 0 - (not enough data), -1 - (ERROR), -2 - (out buffer size small than data)
int M2NS_process(M2NS* handle, void* data, unsigned int dataSize, void* pOutBuffer, unsigned int out_buf_size);

// reset the handle state
//
// - handle     : Pointer to noise suppression instance
// returns      : 0 - (OK), -1 - (ERROR)
int M2NS_reset(M2NS* handle);

// destroy M2 NS instance
//
// - handle     : the pointer of noise suppression instance
// returns      : 0 - (OK), -1 - (ERROR)
int M2NS_destroy(M2NS* handle);

#ifdef __cplusplus
}
#endif
#endif //M2_NS_H_