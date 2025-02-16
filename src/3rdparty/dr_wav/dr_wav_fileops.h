#ifndef DR_WAV_FILEOPS_H
#define DR_WAV_FILEOPS_H

#include "3rdparty/dr_wav/dr_wav_types.h"
#include "3rdparty/dr_wav/dr_wav_util.h"

FILE *drwav_fopen(const char *filePath, const char *openMode);
FILE *drwav_wfopen(const wchar_t *pFilePath, const wchar_t *pOpenMode,
                   const drwav_allocation_callbacks *pAllocationCallbacks);

size_t drwav__on_read(drwav_read_proc onRead, void *pUserData, void *pBufferOut,
                      size_t bytesToRead, drwav_uint64_t *pCursor);
drwav_bool32 drwav__on_seek(drwav_seek_proc onSeek, void *pUserData, int offset,
                            drwav_seek_origin origin, drwav_uint64_t *pCursor);

size_t drwav__on_read_stdio(void *pUserData, void *pBufferOut,
                            size_t bytesToRead);
size_t drwav__on_write_stdio(void *pUserData, const void *pData,
                             size_t bytesToWrite);

drwav_bool32 drwav__read_fmt(drwav_read_proc onRead, drwav_seek_proc onSeek,
                             void *pUserData, drwav_container container,
                             drwav_uint64_t *pRunningBytesReadOut,
                             drwav_fmt *fmtOut);
drwav_result drwav__read_chunk_header(drwav_read_proc onRead, void *pUserData,
                                      drwav_container container,
                                      drwav_uint64_t *pRunningBytesReadOut,
                                      drwav_chunk_header *pHeaderOut);

drwav_bool32 drwav__seek_forward(drwav_seek_proc onSeek, drwav_uint64_t offset,
                                 void *pUserData);
drwav_bool32 drwav__seek_from_start(drwav_seek_proc onSeek,
                                    drwav_uint64_t offset, void *pUserData);

size_t drwav__on_read_memory(void *pUserData, void *pBufferOut,
                             size_t bytesToRead);
drwav_bool32 drwav__on_seek_memory(void *pUserData, int offset,
                                   drwav_seek_origin origin);
size_t drwav__on_write_memory(void *pUserData, const void *pDataIn,
                              size_t bytesToWrite);
drwav_bool32 drwav__on_seek_memory_write(void *pUserData, int offset,
                                         drwav_seek_origin origin);

#endif // DR_WAV_FILEOPS_H
