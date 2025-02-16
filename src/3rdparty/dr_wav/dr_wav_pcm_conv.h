#ifndef DR_WAV_PCM_CONV_H
#define DR_WAV_PCM_CONV_H

#include "3rdparty/dr_wav/dr_wav_types.h"

/* High-Level Convenience Helpers */

#ifndef DR_WAV_NO_CONVERSION_API
/*
Opens and reads an entire wav file in a single operation.

The return value is a heap-allocated buffer containing the audio data. Use
drwav_free() to free the buffer.
*/
drwav_int16_t *drwav_open_and_read_pcm_frames_s16(
    drwav_read_proc onRead, drwav_seek_proc onSeek, void *pUserData,
    unsigned int *channelsOut, unsigned int *sampleRateOut,
    drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks);
float *drwav_open_and_read_pcm_frames_f32(
    drwav_read_proc onRead, drwav_seek_proc onSeek, void *pUserData,
    unsigned int *channelsOut, unsigned int *sampleRateOut,
    drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_int32_t *drwav_open_and_read_pcm_frames_s32(
    drwav_read_proc onRead, drwav_seek_proc onSeek, void *pUserData,
    unsigned int *channelsOut, unsigned int *sampleRateOut,
    drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks);
#ifndef DR_WAV_NO_STDIO
/*
Opens and decodes an entire wav file in a single operation.

The return value is a heap-allocated buffer containing the audio data. Use
drwav_free() to free the buffer.
*/
drwav_int16_t *drwav_open_file_and_read_pcm_frames_s16(
    const char *filename, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks);
float *drwav_open_file_and_read_pcm_frames_f32(
    const char *filename, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_int32_t *drwav_open_file_and_read_pcm_frames_s32(
    const char *filename, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_int16_t *drwav_open_file_and_read_pcm_frames_s16_w(
    const wchar_t *filename, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks);
float *drwav_open_file_and_read_pcm_frames_f32_w(
    const wchar_t *filename, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_int32_t *drwav_open_file_and_read_pcm_frames_s32_w(
    const wchar_t *filename, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks);
#endif
/*
Opens and decodes an entire wav file from a block of memory in a single
operation.

The return value is a heap-allocated buffer containing the audio data. Use
drwav_free() to free the buffer.
*/
drwav_int16_t *drwav_open_memory_and_read_pcm_frames_s16(
    const void *data, size_t dataSize, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks);
float *drwav_open_memory_and_read_pcm_frames_f32(
    const void *data, size_t dataSize, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_int32_t *drwav_open_memory_and_read_pcm_frames_s32(
    const void *data, size_t dataSize, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks);
#endif

/* Conversion Utilities */
#ifndef DR_WAV_NO_CONVERSION_API

/*
Reads a chunk of audio data and converts it to signed 16-bit PCM samples.

Returns the number of PCM frames actually read.

If the return value is less than <framesToRead> it means the end of the file has
been reached.
*/
drwav_uint64_t drwav_read_pcm_frames_s16(drwav *pWav,
                                         drwav_uint64_t framesToRead,
                                         drwav_int16_t *pBufferOut);
drwav_uint64_t drwav_read_pcm_frames_s16le(drwav *pWav,
                                           drwav_uint64_t framesToRead,
                                           drwav_int16_t *pBufferOut);
drwav_uint64_t drwav_read_pcm_frames_s16be(drwav *pWav,
                                           drwav_uint64_t framesToRead,
                                           drwav_int16_t *pBufferOut);

/* Low-level function for converting unsigned 8-bit PCM samples to signed 16-bit
 * PCM samples. */
void drwav_u8_to_s16(drwav_int16_t *pOut, const drwav_uint8_t *pIn,
                     size_t sampleCount);

/* Low-level function for converting signed 24-bit PCM samples to signed 16-bit
 * PCM samples. */
void drwav_s24_to_s16(drwav_int16_t *pOut, const drwav_uint8_t *pIn,
                      size_t sampleCount);

/* Low-level function for converting signed 32-bit PCM samples to signed 16-bit
 * PCM samples. */
void drwav_s32_to_s16(drwav_int16_t *pOut, const drwav_int32_t *pIn,
                      size_t sampleCount);

/* Low-level function for converting IEEE 32-bit floating point samples to
 * signed 16-bit PCM samples. */
void drwav_f32_to_s16(drwav_int16_t *pOut, const float *pIn,
                      size_t sampleCount);

/* Low-level function for converting IEEE 64-bit floating point samples to
 * signed 16-bit PCM samples. */
void drwav_f64_to_s16(drwav_int16_t *pOut, const double *pIn,
                      size_t sampleCount);

/* Low-level function for converting A-law samples to signed 16-bit PCM samples.
 */
void drwav_alaw_to_s16(drwav_int16_t *pOut, const drwav_uint8_t *pIn,
                       size_t sampleCount);

/* Low-level function for converting u-law samples to signed 16-bit PCM samples.
 */
void drwav_mulaw_to_s16(drwav_int16_t *pOut, const drwav_uint8_t *pIn,
                        size_t sampleCount);

/*
Reads a chunk of audio data and converts it to IEEE 32-bit floating point
samples.

Returns the number of PCM frames actually read.

If the return value is less than <framesToRead> it means the end of the file has
been reached.
*/
drwav_uint64_t drwav_read_pcm_frames_f32(drwav *pWav,
                                         drwav_uint64_t framesToRead,
                                         float *pBufferOut);
drwav_uint64_t drwav_read_pcm_frames_f32le(drwav *pWav,
                                           drwav_uint64_t framesToRead,
                                           float *pBufferOut);
drwav_uint64_t drwav_read_pcm_frames_f32be(drwav *pWav,
                                           drwav_uint64_t framesToRead,
                                           float *pBufferOut);

/* Low-level function for converting unsigned 8-bit PCM samples to IEEE 32-bit
 * floating point samples. */
void drwav_u8_to_f32(float *pOut, const drwav_uint8_t *pIn, size_t sampleCount);

/* Low-level function for converting signed 16-bit PCM samples to IEEE 32-bit
 * floating point samples. */
void drwav_s16_to_f32(float *pOut, const drwav_int16_t *pIn,
                      size_t sampleCount);

/* Low-level function for converting signed 24-bit PCM samples to IEEE 32-bit
 * floating point samples. */
void drwav_s24_to_f32(float *pOut, const drwav_uint8_t *pIn,
                      size_t sampleCount);

/* Low-level function for converting signed 32-bit PCM samples to IEEE 32-bit
 * floating point samples. */
void drwav_s32_to_f32(float *pOut, const drwav_int32_t *pIn,
                      size_t sampleCount);

/* Low-level function for converting IEEE 64-bit floating point samples to IEEE
 * 32-bit floating point samples. */
void drwav_f64_to_f32(float *pOut, const double *pIn, size_t sampleCount);

/* Low-level function for converting A-law samples to IEEE 32-bit floating point
 * samples. */
void drwav_alaw_to_f32(float *pOut, const drwav_uint8_t *pIn,
                       size_t sampleCount);

/* Low-level function for converting u-law samples to IEEE 32-bit floating point
 * samples. */
void drwav_mulaw_to_f32(float *pOut, const drwav_uint8_t *pIn,
                        size_t sampleCount);

/*
Reads a chunk of audio data and converts it to signed 32-bit PCM samples.

Returns the number of PCM frames actually read.

If the return value is less than <framesToRead> it means the end of the file has
been reached.
*/
drwav_uint64_t drwav_read_pcm_frames_s32(drwav *pWav,
                                         drwav_uint64_t framesToRead,
                                         drwav_int32_t *pBufferOut);
drwav_uint64_t drwav_read_pcm_frames_s32le(drwav *pWav,
                                           drwav_uint64_t framesToRead,
                                           drwav_int32_t *pBufferOut);
drwav_uint64_t drwav_read_pcm_frames_s32be(drwav *pWav,
                                           drwav_uint64_t framesToRead,
                                           drwav_int32_t *pBufferOut);

/* Low-level function for converting unsigned 8-bit PCM samples to signed 32-bit
 * PCM samples. */
void drwav_u8_to_s32(drwav_int32_t *pOut, const drwav_uint8_t *pIn,
                     size_t sampleCount);

/* Low-level function for converting signed 16-bit PCM samples to signed 32-bit
 * PCM samples. */
void drwav_s16_to_s32(drwav_int32_t *pOut, const drwav_int16_t *pIn,
                      size_t sampleCount);

/* Low-level function for converting signed 24-bit PCM samples to signed 32-bit
 * PCM samples. */
void drwav_s24_to_s32(drwav_int32_t *pOut, const drwav_uint8_t *pIn,
                      size_t sampleCount);

/* Low-level function for converting IEEE 32-bit floating point samples to
 * signed 32-bit PCM samples. */
void drwav_f32_to_s32(drwav_int32_t *pOut, const float *pIn,
                      size_t sampleCount);

/* Low-level function for converting IEEE 64-bit floating point samples to
 * signed 32-bit PCM samples. */
void drwav_f64_to_s32(drwav_int32_t *pOut, const double *pIn,
                      size_t sampleCount);

/* Low-level function for converting A-law samples to signed 32-bit PCM samples.
 */
void drwav_alaw_to_s32(drwav_int32_t *pOut, const drwav_uint8_t *pIn,
                       size_t sampleCount);

/* Low-level function for converting u-law samples to signed 32-bit PCM samples.
 */
void drwav_mulaw_to_s32(drwav_int32_t *pOut, const drwav_uint8_t *pIn,
                        size_t sampleCount);

#endif /* DR_WAV_NO_CONVERSION_API */

#endif // DR_WAV_PCM_CONV_H
