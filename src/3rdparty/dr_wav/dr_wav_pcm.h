#ifndef DR_WAV_PCM_H
#define DR_WAV_PCM_H

#include "3rdparty/dr_wav/dr_wav_types.h"

drwav_uint64_t drwav_read_pcm_frames_s16__msadpcm(drwav *pWav,
                                                  drwav_uint64_t framesToRead,
                                                  drwav_int16_t *pBufferOut);
drwav_uint64_t drwav_read_pcm_frames_s16__ima(drwav *pWav,
                                              drwav_uint64_t framesToRead,
                                              drwav_int16_t *pBufferOut);

/*
Reads up to the specified number of PCM frames from the WAV file.

The output data will be in the file's internal format, converted to
native-endian byte order. Use drwav_read_pcm_frames_s16/f32/s32() to read data
in a specific format.

If the return value is less than <framesToRead> it means the end of the file has
been reached or you have requested more PCM frames than can possibly fit in the
output buffer.

This function will only work when sample data is of a fixed size and
uncompressed. If you are using a compressed format consider using
drwav_read_raw() or drwav_read_pcm_frames_s16/s32/f32().
*/
drwav_uint64_t drwav_read_pcm_frames(drwav *pWav, drwav_uint64_t framesToRead,
                                     void *pBufferOut);
drwav_uint64_t drwav_read_pcm_frames_le(drwav *pWav,
                                        drwav_uint64_t framesToRead,
                                        void *pBufferOut);
drwav_uint64_t drwav_read_pcm_frames_be(drwav *pWav,
                                        drwav_uint64_t framesToRead,
                                        void *pBufferOut);

/*
Seeks to the given PCM frame.

Returns true if successful; false otherwise.
*/
drwav_bool32 drwav_seek_to_pcm_frame(drwav *pWav,
                                     drwav_uint64_t targetFrameIndex);

/*
Writes raw audio data.

Returns the number of bytes actually written. If this differs from bytesToWrite,
it indicates an error.
*/
size_t drwav_write_raw(drwav *pWav, size_t bytesToWrite, const void *pData);

/*
Writes PCM frames.

Returns the number of PCM frames written.

Input samples need to be in native-endian byte order. On big-endian
architectures the input data will be converted to little-endian. Use
drwav_write_raw() to write raw audio data without performing any conversion.
*/
drwav_uint64_t drwav_write_pcm_frames(drwav *pWav, drwav_uint64_t framesToWrite,
                                      const void *pData);
drwav_uint64_t drwav_write_pcm_frames_le(drwav *pWav,
                                         drwav_uint64_t framesToWrite,
                                         const void *pData);
drwav_uint64_t drwav_write_pcm_frames_be(drwav *pWav,
                                         drwav_uint64_t framesToWrite,
                                         const void *pData);

#endif // DR_WAV_PCM_H
