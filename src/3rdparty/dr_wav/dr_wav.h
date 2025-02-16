/*
WAV audio loader and writer. Choice of public domain or MIT-0. See license
statements at the end of this file. dr_wav - v0.11.1 - 2019-10-07

David Reid - mackron@gmail.com
*/

/*
RELEASE NOTES - v0.11.0
=======================
Version 0.11.0 has breaking API changes.

Improved Client-Defined Memory Allocation
-----------------------------------------
The main change with this release is the addition of a more flexible way of
implementing custom memory allocation routines. The existing system of
DRWAV_MALLOC, DRWAV_REALLOC and DRWAV_FREE are still in place and will be used
by default when no custom allocation callbacks are specified.

To use the new system, you pass in a pointer to a drwav_allocation_callbacks
object to drwav_init() and family, like this:

    void* my_malloc(size_t sz, void* pUserData)
    {
        return malloc(sz);
    }
    void* my_realloc(void* p, size_t sz, void* pUserData)
    {
        return realloc(p, sz);
    }
    void my_free(void* p, void* pUserData)
    {
        free(p);
    }

    ...

    drwav_allocation_callbacks allocationCallbacks;
    allocationCallbacks.pUserData = &myData;
    allocationCallbacks.onMalloc  = my_malloc;
    allocationCallbacks.onRealloc = my_realloc;
    allocationCallbacks.onFree    = my_free;
    drwav_init_file(&wav, "my_file.wav", &allocationCallbacks);

The advantage of this new system is that it allows you to specify user data
which will be passed in to the allocation routines.

Passing in null for the allocation callbacks object will cause dr_wav to use
defaults which is the same as DRWAV_MALLOC, DRWAV_REALLOC and DRWAV_FREE and the
equivalent of how it worked in previous versions.

Every API that opens a drwav object now takes this extra parameter. These
include the following:

    drwav_init()
    drwav_init_ex()
    drwav_init_file()
    drwav_init_file_ex()
    drwav_init_file_w()
    drwav_init_file_w_ex()
    drwav_init_memory()
    drwav_init_memory_ex()
    drwav_init_write()
    drwav_init_write_sequential()
    drwav_init_write_sequential_pcm_frames()
    drwav_init_file_write()
    drwav_init_file_write_sequential()
    drwav_init_file_write_sequential_pcm_frames()
    drwav_init_file_write_w()
    drwav_init_file_write_sequential_w()
    drwav_init_file_write_sequential_pcm_frames_w()
    drwav_init_memory_write()
    drwav_init_memory_write_sequential()
    drwav_init_memory_write_sequential_pcm_frames()
    drwav_open_and_read_pcm_frames_s16()
    drwav_open_and_read_pcm_frames_f32()
    drwav_open_and_read_pcm_frames_s32()
    drwav_open_file_and_read_pcm_frames_s16()
    drwav_open_file_and_read_pcm_frames_f32()
    drwav_open_file_and_read_pcm_frames_s32()
    drwav_open_file_and_read_pcm_frames_s16_w()
    drwav_open_file_and_read_pcm_frames_f32_w()
    drwav_open_file_and_read_pcm_frames_s32_w()
    drwav_open_memory_and_read_pcm_frames_s16()
    drwav_open_memory_and_read_pcm_frames_f32()
    drwav_open_memory_and_read_pcm_frames_s32()

Endian Improvements
-------------------
Previously, the following APIs returned little-endian audio data. These now
return native-endian data. This improves compatibility on big-endian
architectures.

    drwav_read_pcm_frames()
    drwav_read_pcm_frames_s16()
    drwav_read_pcm_frames_s32()
    drwav_read_pcm_frames_f32()
    drwav_open_and_read_pcm_frames_s16()
    drwav_open_and_read_pcm_frames_s32()
    drwav_open_and_read_pcm_frames_f32()
    drwav_open_file_and_read_pcm_frames_s16()
    drwav_open_file_and_read_pcm_frames_s32()
    drwav_open_file_and_read_pcm_frames_f32()
    drwav_open_file_and_read_pcm_frames_s16_w()
    drwav_open_file_and_read_pcm_frames_s32_w()
    drwav_open_file_and_read_pcm_frames_f32_w()
    drwav_open_memory_and_read_pcm_frames_s16()
    drwav_open_memory_and_read_pcm_frames_s32()
    drwav_open_memory_and_read_pcm_frames_f32()

APIs have been added to give you explicit control over whether or not audio data
is read or written in big- or little-endian byte order:

    drwav_read_pcm_frames_le()
    drwav_read_pcm_frames_be()
    drwav_read_pcm_frames_s16le()
    drwav_read_pcm_frames_s16be()
    drwav_read_pcm_frames_f32le()
    drwav_read_pcm_frames_f32be()
    drwav_read_pcm_frames_s32le()
    drwav_read_pcm_frames_s32be()
    drwav_write_pcm_frames_le()
    drwav_write_pcm_frames_be()

Removed APIs
------------
The following APIs were deprecated in version 0.10.0 and have now been removed:

    drwav_open()
    drwav_open_ex()
    drwav_open_write()
    drwav_open_write_sequential()
    drwav_open_file()
    drwav_open_file_ex()
    drwav_open_file_write()
    drwav_open_file_write_sequential()
    drwav_open_memory()
    drwav_open_memory_ex()
    drwav_open_memory_write()
    drwav_open_memory_write_sequential()
    drwav_close()



RELEASE NOTES - v0.10.0
=======================
Version 0.10.0 has breaking API changes. There are no significant bug fixes in
this release, so if you are affected you do not need to upgrade.

Removed APIs
------------
The following APIs were deprecated in version 0.9.0 and have been completely
removed in version 0.10.0:

    drwav_read()
    drwav_read_s16()
    drwav_read_f32()
    drwav_read_s32()
    drwav_seek_to_sample()
    drwav_write()
    drwav_open_and_read_s16()
    drwav_open_and_read_f32()
    drwav_open_and_read_s32()
    drwav_open_file_and_read_s16()
    drwav_open_file_and_read_f32()
    drwav_open_file_and_read_s32()
    drwav_open_memory_and_read_s16()
    drwav_open_memory_and_read_f32()
    drwav_open_memory_and_read_s32()
    drwav::totalSampleCount

See release notes for version 0.9.0 at the bottom of this file for replacement
APIs.

Deprecated APIs
---------------
The following APIs have been deprecated. There is a confusing and completely
arbitrary difference between drwav_init*() and drwav_open*(), where
drwav_init*() initializes a pre-allocated drwav object, whereas drwav_open*()
will first allocated a drwav object on the heap and then initialize it.
drwav_open*() has been deprecated which means you must now use a pre- allocated
drwav object with drwav_init*(). If you need the previous functionality, you can
just do a malloc() followed by a called to one of the drwav_init*() APIs.

    drwav_open()
    drwav_open_ex()
    drwav_open_write()
    drwav_open_write_sequential()
    drwav_open_file()
    drwav_open_file_ex()
    drwav_open_file_write()
    drwav_open_file_write_sequential()
    drwav_open_memory()
    drwav_open_memory_ex()
    drwav_open_memory_write()
    drwav_open_memory_write_sequential()
    drwav_close()

These APIs will be removed completely in a future version. The rationale for
this change is to remove confusion between the two different ways to initialize
a drwav object.
*/

/*
USAGE
=====
This is a single-file library. To use it, do something like the following in one
.c file. #define DR_WAV_IMPLEMENTATION #include "dr_wav.h"

You can then #include this file in other parts of the program as you would with
any other header file. Do something like the following to read audio data:

    drwav wav;
    if (!drwav_init_file(&wav, "my_song.wav")) {
        // Error opening WAV file.
    }

    drwav_int32_t* pDecodedInterleavedPCMFrames = malloc(wav.totalPCMFrameCount
* wav.channels * sizeof(drwav_int32_t)); size_t numberOfSamplesActuallyDecoded =
drwav_read_pcm_frames_s32(&wav, wav.totalPCMFrameCount,
pDecodedInterleavedPCMFrames);

    ...

    drwav_uninit(&wav);

If you just want to quickly open and read the audio data in a single operation
you can do something like this:

    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64_t totalPCMFrameCount;
    float* pSampleData = drwav_open_file_and_read_pcm_frames_f32("my_song.wav",
&channels, &sampleRate, &totalPCMFrameCount); if (pSampleData == NULL) {
        // Error opening and reading WAV file.
    }

    ...

    drwav_free(pSampleData);

The examples above use versions of the API that convert the audio data to a
consistent format (32-bit signed PCM, in this case), but you can still output
the audio data in its internal format (see notes below for supported formats):

    size_t framesRead = drwav_read_pcm_frames(&wav, wav.totalPCMFrameCount,
pDecodedInterleavedPCMFrames);

You can also read the raw bytes of audio data, which could be useful if dr_wav
does not have native support for a particular data format:

    size_t bytesRead = drwav_read_raw(&wav, bytesToRead, pRawDataBuffer);


dr_wav can also be used to output WAV files. This does not currently support
compressed formats. To use this, look at drwav_init_write(),
drwav_init_file_write(), etc. Use drwav_write_pcm_frames() to write samples, or
drwav_write_raw() to write raw data in the "data" chunk.

    drwav_data_format format;
    format.container = drwav_container_riff;     // <-- drwav_container_riff =
normal WAV files, drwav_container_w64 = Sony Wave64. format.format =
DR_WAVE_FORMAT_PCM;          // <-- Any of the DR_WAVE_FORMAT_* codes.
    format.channels = 2;
    format.sampleRate = 44100;
    format.bitsPerSample = 16;
    drwav_init_file_write(&wav, "data/recording.wav", &format);

    ...

    drwav_uint64_t framesWritten = drwav_write_pcm_frames(pWav, frameCount,
pSamples);


dr_wav has seamless support the Sony Wave64 format. The decoder will
automatically detect it and it should Just Work without any manual intervention.


OPTIONS
=======
#define these options before including this file.

#define DR_WAV_NO_CONVERSION_API
  Disables conversion APIs such as drwav_read_pcm_frames_f32() and
drwav_s16_to_f32().

#define DR_WAV_NO_STDIO
  Disables APIs that initialize a decoder from a file such as drwav_init_file(),
drwav_init_file_write(), etc.



QUICK NOTES
===========
- Samples are always interleaved.
- The default read function does not do any data conversion. Use
drwav_read_pcm_frames_f32(), drwav_read_pcm_frames_s32() and
drwav_read_pcm_frames_s16() to read and convert audio data to 32-bit floating
point, signed 32-bit integer and signed 16-bit integer samples respectively.
Tested and supported internal formats include the following:
  - Unsigned 8-bit PCM
  - Signed 12-bit PCM
  - Signed 16-bit PCM
  - Signed 24-bit PCM
  - Signed 32-bit PCM
  - IEEE 32-bit floating point
  - IEEE 64-bit floating point
  - A-law and u-law
  - Microsoft ADPCM
  - IMA ADPCM (DVI, format code 0x11)
- dr_wav will try to read the WAV file as best it can, even if it's not strictly
conformant to the WAV format.
*/

#ifndef dr_wav_h
#define dr_wav_h

#include "3rdparty/dr_wav/dr_wav_fileops.h"
#include "3rdparty/dr_wav/dr_wav_init.h"
#include "3rdparty/dr_wav/dr_wav_pcm.h"
#include "3rdparty/dr_wav/dr_wav_pcm_conv.h"
#include "3rdparty/dr_wav/dr_wav_types.h"
#include "3rdparty/dr_wav/dr_wav_util.h"

#include <stddef.h>
#endif /* dr_wav_h */

/*
REVISION HISTORY
================
v0.11.1 - 2019-10-07
  - Internal code clean up.

v0.11.0 - 2019-10-06
  - API CHANGE: Add support for user defined memory allocation routines. This
system allows the program to specify their own memory allocation routines with a
user data pointer for client-specific contextual data. This adds an extra
parameter to the end of the following APIs:
    - drwav_init()
    - drwav_init_ex()
    - drwav_init_file()
    - drwav_init_file_ex()
    - drwav_init_file_w()
    - drwav_init_file_w_ex()
    - drwav_init_memory()
    - drwav_init_memory_ex()
    - drwav_init_write()
    - drwav_init_write_sequential()
    - drwav_init_write_sequential_pcm_frames()
    - drwav_init_file_write()
    - drwav_init_file_write_sequential()
    - drwav_init_file_write_sequential_pcm_frames()
    - drwav_init_file_write_w()
    - drwav_init_file_write_sequential_w()
    - drwav_init_file_write_sequential_pcm_frames_w()
    - drwav_init_memory_write()
    - drwav_init_memory_write_sequential()
    - drwav_init_memory_write_sequential_pcm_frames()
    - drwav_open_and_read_pcm_frames_s16()
    - drwav_open_and_read_pcm_frames_f32()
    - drwav_open_and_read_pcm_frames_s32()
    - drwav_open_file_and_read_pcm_frames_s16()
    - drwav_open_file_and_read_pcm_frames_f32()
    - drwav_open_file_and_read_pcm_frames_s32()
    - drwav_open_file_and_read_pcm_frames_s16_w()
    - drwav_open_file_and_read_pcm_frames_f32_w()
    - drwav_open_file_and_read_pcm_frames_s32_w()
    - drwav_open_memory_and_read_pcm_frames_s16()
    - drwav_open_memory_and_read_pcm_frames_f32()
    - drwav_open_memory_and_read_pcm_frames_s32()
    Set this extra parameter to NULL to use defaults which is the same as the
previous behaviour. Setting this NULL will use DRWAV_MALLOC, DRWAV_REALLOC and
DRWAV_FREE.
  - Add support for reading and writing PCM frames in an explicit endianness.
New APIs:
    - drwav_read_pcm_frames_le()
    - drwav_read_pcm_frames_be()
    - drwav_read_pcm_frames_s16le()
    - drwav_read_pcm_frames_s16be()
    - drwav_read_pcm_frames_f32le()
    - drwav_read_pcm_frames_f32be()
    - drwav_read_pcm_frames_s32le()
    - drwav_read_pcm_frames_s32be()
    - drwav_write_pcm_frames_le()
    - drwav_write_pcm_frames_be()
  - Remove deprecated APIs.
  - API CHANGE: The following APIs now return native-endian data. Previously
they returned little-endian data.
    - drwav_read_pcm_frames()
    - drwav_read_pcm_frames_s16()
    - drwav_read_pcm_frames_s32()
    - drwav_read_pcm_frames_f32()
    - drwav_open_and_read_pcm_frames_s16()
    - drwav_open_and_read_pcm_frames_s32()
    - drwav_open_and_read_pcm_frames_f32()
    - drwav_open_file_and_read_pcm_frames_s16()
    - drwav_open_file_and_read_pcm_frames_s32()
    - drwav_open_file_and_read_pcm_frames_f32()
    - drwav_open_file_and_read_pcm_frames_s16_w()
    - drwav_open_file_and_read_pcm_frames_s32_w()
    - drwav_open_file_and_read_pcm_frames_f32_w()
    - drwav_open_memory_and_read_pcm_frames_s16()
    - drwav_open_memory_and_read_pcm_frames_s32()
    - drwav_open_memory_and_read_pcm_frames_f32()

v0.10.1 - 2019-08-31
  - Correctly handle partial trailing ADPCM blocks.

v0.10.0 - 2019-08-04
  - Remove deprecated APIs.
  - Add wchar_t variants for file loading APIs:
      drwav_init_file_w()
      drwav_init_file_ex_w()
      drwav_init_file_write_w()
      drwav_init_file_write_sequential_w()
  - Add drwav_target_write_size_bytes() which calculates the total size in bytes
of a WAV file given a format and sample count.
  - Add APIs for specifying the PCM frame count instead of the sample count when
opening in sequential write mode: drwav_init_write_sequential_pcm_frames()
      drwav_init_file_write_sequential_pcm_frames()
      drwav_init_file_write_sequential_pcm_frames_w()
      drwav_init_memory_write_sequential_pcm_frames()
  - Deprecate drwav_open*() and drwav_close():
      drwav_open()
      drwav_open_ex()
      drwav_open_write()
      drwav_open_write_sequential()
      drwav_open_file()
      drwav_open_file_ex()
      drwav_open_file_write()
      drwav_open_file_write_sequential()
      drwav_open_memory()
      drwav_open_memory_ex()
      drwav_open_memory_write()
      drwav_open_memory_write_sequential()
      drwav_close()
  - Minor documentation updates.

v0.9.2 - 2019-05-21
  - Fix warnings.

v0.9.1 - 2019-05-05
  - Add support for C89.
  - Change license to choice of public domain or MIT-0.

v0.9.0 - 2018-12-16
  - API CHANGE: Add new reading APIs for reading by PCM frames instead of
samples. Old APIs have been deprecated and will be removed in v0.10.0.
Deprecated APIs and their replacements: drwav_read()                     ->
drwav_read_pcm_frames() drwav_read_s16()                 ->
drwav_read_pcm_frames_s16() drwav_read_f32()                 ->
drwav_read_pcm_frames_f32() drwav_read_s32()                 ->
drwav_read_pcm_frames_s32() drwav_seek_to_sample()           ->
drwav_seek_to_pcm_frame() drwav_write()                    ->
drwav_write_pcm_frames() drwav_open_and_read_s16()        ->
drwav_open_and_read_pcm_frames_s16() drwav_open_and_read_f32()        ->
drwav_open_and_read_pcm_frames_f32() drwav_open_and_read_s32()        ->
drwav_open_and_read_pcm_frames_s32() drwav_open_file_and_read_s16()   ->
drwav_open_file_and_read_pcm_frames_s16() drwav_open_file_and_read_f32()   ->
drwav_open_file_and_read_pcm_frames_f32() drwav_open_file_and_read_s32()   ->
drwav_open_file_and_read_pcm_frames_s32() drwav_open_memory_and_read_s16() ->
drwav_open_memory_and_read_pcm_frames_s16() drwav_open_memory_and_read_f32() ->
drwav_open_memory_and_read_pcm_frames_f32() drwav_open_memory_and_read_s32() ->
drwav_open_memory_and_read_pcm_frames_s32() drwav::totalSampleCount          ->
drwav::totalPCMFrameCount
  - API CHANGE: Rename drwav_open_and_read_file_*() to
drwav_open_file_and_read_*().
  - API CHANGE: Rename drwav_open_and_read_memory_*() to
drwav_open_memory_and_read_*().
  - Add built-in support for smpl chunks.
  - Add support for firing a callback for each chunk in the file at
initialization time.
    - This is enabled through the drwav_init_ex(), etc. family of APIs.
  - Handle invalid FMT chunks more robustly.

v0.8.5 - 2018-09-11
  - Const correctness.
  - Fix a potential stack overflow.

v0.8.4 - 2018-08-07
  - Improve 64-bit detection.

v0.8.3 - 2018-08-05
  - Fix C++ build on older versions of GCC.

v0.8.2 - 2018-08-02
  - Fix some big-endian bugs.

v0.8.1 - 2018-06-29
  - Add support for sequential writing APIs.
  - Disable seeking in write mode.
  - Fix bugs with Wave64.
  - Fix typos.

v0.8 - 2018-04-27
  - Bug fix.
  - Start using major.minor.revision versioning.

v0.7f - 2018-02-05
  - Restrict ADPCM formats to a maximum of 2 channels.

v0.7e - 2018-02-02
  - Fix a crash.

v0.7d - 2018-02-01
  - Fix a crash.

v0.7c - 2018-02-01
  - Set drwav.bytesPerSample to 0 for all compressed formats.
  - Fix a crash when reading 16-bit floating point WAV files. In this case
dr_wav will output silence for all format conversion reading APIs (*_s16, *_s32,
*_f32 APIs).
  - Fix some divide-by-zero errors.

v0.7b - 2018-01-22
  - Fix errors with seeking of compressed formats.
  - Fix compilation error when DR_WAV_NO_CONVERSION_API

v0.7a - 2017-11-17
  - Fix some GCC warnings.

v0.7 - 2017-11-04
  - Add writing APIs.

v0.6 - 2017-08-16
  - API CHANGE: Rename dr_* types to drwav_*.
  - Add support for custom implementations of malloc(), realloc(), etc.
  - Add support for Microsoft ADPCM.
  - Add support for IMA ADPCM (DVI, format code 0x11).
  - Optimizations to drwav_read_s16().
  - Bug fixes.

v0.5g - 2017-07-16
  - Change underlying type for booleans to unsigned.

v0.5f - 2017-04-04
  - Fix a minor bug with drwav_open_and_read_s16() and family.

v0.5e - 2016-12-29
  - Added support for reading samples as signed 16-bit integers. Use the _s16()
family of APIs for this.
  - Minor fixes to documentation.

v0.5d - 2016-12-28
  - Use drwav_int* and drwav_uint* sized types to improve compiler support.

v0.5c - 2016-11-11
  - Properly handle JUNK chunks that come before the FMT chunk.

v0.5b - 2016-10-23
  - A minor change to drwav_bool8 and drwav_bool32 types.

v0.5a - 2016-10-11
  - Fixed a bug with drwav_open_and_read() and family due to incorrect argument
ordering.
  - Improve A-law and mu-law efficiency.

v0.5 - 2016-09-29
  - API CHANGE. Swap the order of "channels" and "sampleRate" parameters in
drwav_open_and_read*(). Rationale for this is to keep it consistent with
dr_audio and dr_flac.

v0.4b - 2016-09-18
  - Fixed a typo in documentation.

v0.4a - 2016-09-18
  - Fixed a typo.
  - Change date format to ISO 8601 (YYYY-MM-DD)

v0.4 - 2016-07-13
  - API CHANGE. Make onSeek consistent with dr_flac.
  - API CHANGE. Rename drwav_seek() to drwav_seek_to_sample() for clarity and
consistency with dr_flac.
  - Added support for Sony Wave64.

v0.3a - 2016-05-28
  - API CHANGE. Return drwav_bool32 instead of int in onSeek callback.
  - Fixed a memory leak.

v0.3 - 2016-05-22
  - Lots of API changes for consistency.

v0.2a - 2016-05-16
  - Fixed Linux/GCC build.

v0.2 - 2016-05-11
  - Added support for reading data as signed 32-bit PCM for consistency with
dr_flac.

v0.1a - 2016-05-07
  - Fixed a bug in drwav_open_file() where the file handle would not be closed
if the loader failed to initialize.

v0.1 - 2016-05-04
  - Initial versioned release.
*/

/*
This software is available as a choice of the following licenses. Choose
whichever you prefer.

===============================================================================
ALTERNATIVE 1 - Public Domain (www.unlicense.org)
===============================================================================
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

===============================================================================
ALTERNATIVE 2 - MIT No Attribution
===============================================================================
Copyright 2018 David Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
