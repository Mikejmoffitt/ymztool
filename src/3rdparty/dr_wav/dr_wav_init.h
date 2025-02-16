#ifndef DR_WAV_INIT_H
#define DR_WAV_INIT_H

#include "3rdparty/dr_wav/dr_wav_types.h"

drwav_bool32
drwav_preinit(drwav *pWav, drwav_read_proc onRead, drwav_seek_proc onSeek,
              void *pReadSeekUserData,
              const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32 drwav_init__internal(drwav *pWav, drwav_chunk_proc onChunk,
                                  void *pChunkUserData, drwav_uint32_t flags);
drwav_bool32
drwav_preinit_write(drwav *pWav, const drwav_data_format *pFormat,
                    drwav_bool32 isSequential, drwav_write_proc onWrite,
                    drwav_seek_proc onSeek, void *pUserData,
                    const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32 drwav_init_write__internal(drwav *pWav,
                                        const drwav_data_format *pFormat,
                                        drwav_uint64_t totalSampleCount);

/*
Initializes a pre-allocated drwav object for reading.

pWav                         [out]          A pointer to the drwav object being
initialized. onRead                       [in]           The function to call
when data needs to be read from the client. onSeek                       [in]
The function to call when the read position of the client data needs to move.
onChunk                      [in, optional] The function to call when a chunk is
enumerated at initialized time. pUserData, pReadSeekUserData [in, optional] A
pointer to application defined data that will be passed to onRead and onSeek.
pChunkUserData               [in, optional] A pointer to application defined
data that will be passed to onChunk. flags                        [in, optional]
A set of flags for controlling how things are loaded.

Returns true if successful; false otherwise.

Close the loader with drwav_uninit().

This is the lowest level function for initializing a WAV file. You can also use
drwav_init_file() and drwav_init_memory() to open the stream from a file or from
a block of memory respectively.

Possible values for flags:
  DRWAV_SEQUENTIAL: Never perform a backwards seek while loading. This disables
the chunk callback and will cause this function to return as soon as the data
chunk is found. Any chunks after the data chunk will be ignored.

drwav_init() is equivalent to "drwav_init_ex(pWav, onRead, onSeek, NULL,
pUserData, NULL, 0);".

The onChunk callback is not called for the WAVE or FMT chunks. The contents of
the FMT chunk can be read from pWav->fmt after the function returns.

See also: drwav_init_file(), drwav_init_memory(), drwav_uninit()
*/
drwav_bool32 drwav_init(drwav *pWav, drwav_read_proc onRead,
                        drwav_seek_proc onSeek, void *pUserData,
                        const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32
drwav_init_ex(drwav *pWav, drwav_read_proc onRead, drwav_seek_proc onSeek,
              drwav_chunk_proc onChunk, void *pReadSeekUserData,
              void *pChunkUserData, drwav_uint32_t flags,
              const drwav_allocation_callbacks *pAllocationCallbacks);

/*
Initializes a pre-allocated drwav object for writing.

onWrite   [in]           The function to call when data needs to be written.
onSeek    [in]           The function to call when the write position needs to
move. pUserData [in, optional] A pointer to application defined data that will
be passed to onWrite and onSeek.

Returns true if successful; false otherwise.

Close the writer with drwav_uninit().

This is the lowest level function for initializing a WAV file. You can also use
drwav_init_file_write() and drwav_init_memory_write() to open the stream from a
file or from a block of memory respectively.

If the total sample count is known, you can use drwav_init_write_sequential().
This avoids the need for dr_wav to perform a post-processing step for storing
the total sample count and the size of the data chunk which requires a backwards
seek.

See also: drwav_init_file_write(), drwav_init_memory_write(), drwav_uninit()
*/
drwav_bool32
drwav_init_write(drwav *pWav, const drwav_data_format *pFormat,
                 drwav_write_proc onWrite, drwav_seek_proc onSeek,
                 void *pUserData,
                 const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32 drwav_init_write_sequential(
    drwav *pWav, const drwav_data_format *pFormat,
    drwav_uint64_t totalSampleCount, drwav_write_proc onWrite, void *pUserData,
    const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32 drwav_init_write_sequential_pcm_frames(
    drwav *pWav, const drwav_data_format *pFormat,
    drwav_uint64_t totalPCMFrameCount, drwav_write_proc onWrite,
    void *pUserData, const drwav_allocation_callbacks *pAllocationCallbacks);

/*
Uninitializes the given drwav object.

Use this only for objects initialized with drwav_init*() functions
(drwav_init(), drwav_init_ex(), drwav_init_write(),
drwav_init_write_sequential()).
*/
drwav_result drwav_uninit(drwav *pWav);

#ifndef DR_WAV_NO_STDIO
/*
Helper for initializing a wave file for reading using stdio.

This holds the internal FILE object until drwav_uninit() is called. Keep this in
mind if you're caching drwav objects because the operating system may restrict
the number of file handles an application can have open at any given time.
*/
drwav_bool32
drwav_init_file(drwav *pWav, const char *filename,
                const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32
drwav_init_file_ex(drwav *pWav, const char *filename, drwav_chunk_proc onChunk,
                   void *pChunkUserData, drwav_uint32_t flags,
                   const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32
drwav_init_file_w(drwav *pWav, const wchar_t *filename,
                  const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32
drwav_init_file_ex_w(drwav *pWav, const wchar_t *filename,
                     drwav_chunk_proc onChunk, void *pChunkUserData,
                     drwav_uint32_t flags,
                     const drwav_allocation_callbacks *pAllocationCallbacks);

/*
Helper for initializing a wave file for writing using stdio.

This holds the internal FILE object until drwav_uninit() is called. Keep this in
mind if you're caching drwav objects because the operating system may restrict
the number of file handles an application can have open at any given time.
*/
drwav_bool32
drwav_init_file_write(drwav *pWav, const char *filename,
                      const drwav_data_format *pFormat,
                      const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32 drwav_init_file_write_sequential(
    drwav *pWav, const char *filename, const drwav_data_format *pFormat,
    drwav_uint64_t totalSampleCount,
    const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32 drwav_init_file_write_sequential_pcm_frames(
    drwav *pWav, const char *filename, const drwav_data_format *pFormat,
    drwav_uint64_t totalPCMFrameCount,
    const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32
drwav_init_file_write_w(drwav *pWav, const wchar_t *filename,
                        const drwav_data_format *pFormat,
                        const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32 drwav_init_file_write_sequential_w(
    drwav *pWav, const wchar_t *filename, const drwav_data_format *pFormat,
    drwav_uint64_t totalSampleCount,
    const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32 drwav_init_file_write_sequential_pcm_frames_w(
    drwav *pWav, const wchar_t *filename, const drwav_data_format *pFormat,
    drwav_uint64_t totalPCMFrameCount,
    const drwav_allocation_callbacks *pAllocationCallbacks);
#endif /* DR_WAV_NO_STDIO */

/*
Helper for initializing a loader from a pre-allocated memory buffer.

This does not create a copy of the data. It is up to the application to ensure
the buffer remains valid for the lifetime of the drwav object.

The buffer should contain the contents of the entire wave file, not just the
sample data.
*/
drwav_bool32
drwav_init_memory(drwav *pWav, const void *data, size_t dataSize,
                  const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32
drwav_init_memory_ex(drwav *pWav, const void *data, size_t dataSize,
                     drwav_chunk_proc onChunk, void *pChunkUserData,
                     drwav_uint32_t flags,
                     const drwav_allocation_callbacks *pAllocationCallbacks);

/*
Helper for initializing a writer which outputs data to a memory buffer.

dr_wav will manage the memory allocations, however it is up to the caller to
free the data with drwav_free().

The buffer will remain allocated even after drwav_uninit() is called. Indeed,
the buffer should not be considered valid until after drwav_uninit() has been
called anyway.
*/
drwav_bool32
drwav_init_memory_write(drwav *pWav, void **ppData, size_t *pDataSize,
                        const drwav_data_format *pFormat,
                        const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32 drwav_init_memory_write_sequential(
    drwav *pWav, void **ppData, size_t *pDataSize,
    const drwav_data_format *pFormat, drwav_uint64_t totalSampleCount,
    const drwav_allocation_callbacks *pAllocationCallbacks);
drwav_bool32 drwav_init_memory_write_sequential_pcm_frames(
    drwav *pWav, void **ppData, size_t *pDataSize,
    const drwav_data_format *pFormat, drwav_uint64_t totalPCMFrameCount,
    const drwav_allocation_callbacks *pAllocationCallbacks);

#endif // DR_WAV_INIT_H
