#ifndef DR_WAV_TYPES_H
#define DR_WAV_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_MSC_VER) && _MSC_VER < 1600
	typedef signed char drwav_int8_t;
	typedef unsigned char drwav_uint8_t;
	typedef signed short drwav_int16_t;
	typedef unsigned short drwav_uint16_t;
	typedef signed int drwav_int32_t;
	typedef unsigned int drwav_uint32_t;
	typedef signed __int64_t drwav_int64_t;
	typedef unsigned __int64_t drwav_uint64_t;
#else
#include <stdint.h>
#include <stdlib.h>
typedef int8_t drwav_int8_t;
typedef uint8_t drwav_uint8_t;
typedef int16_t drwav_int16_t;
typedef uint16_t drwav_uint16_t;
typedef int32_t drwav_int32_t;
typedef uint32_t drwav_uint32_t;
typedef int64_t drwav_int64_t;
typedef uint64_t drwav_uint64_t;
#endif
	typedef drwav_uint8_t drwav_bool8;
	typedef drwav_uint32_t drwav_bool32;
#define DRWAV_TRUE 1
#define DRWAV_FALSE 0

	typedef drwav_int32_t drwav_result;
#define DRWAV_SUCCESS 0
#define DRWAV_ERROR -1
#define DRWAV_INVALID_ARGS -2
#define DRWAV_INVALID_OPERATION -3
#define DRWAV_INVALID_FILE -100
#define DRWAV_EOF -101

/* Common data formats. */
#define DR_WAVE_FORMAT_PCM 0x1
#define DR_WAVE_FORMAT_ADPCM 0x2
#define DR_WAVE_FORMAT_IEEE_FLOAT 0x3
#define DR_WAVE_FORMAT_ALAW 0x6
#define DR_WAVE_FORMAT_MULAW 0x7
#define DR_WAVE_FORMAT_DVI_ADPCM 0x11
#define DR_WAVE_FORMAT_EXTENSIBLE 0xFFFE

/* Constants. */
#ifndef DRWAV_MAX_SMPL_LOOPS
#define DRWAV_MAX_SMPL_LOOPS 1
#endif

/* Flags to pass into drwav_init_ex(), etc. */
#define DRWAV_SEQUENTIAL 0x00000001

	typedef enum
	{
		drwav_seek_origin_start,
		drwav_seek_origin_current
	} drwav_seek_origin;

	typedef enum
	{
		drwav_container_riff,
		drwav_container_w64
	} drwav_container;

	typedef struct
	{
		union
		{
			drwav_uint8_t fourcc[4];
			drwav_uint8_t guid[16];
		} id;

		/* The size in bytes of the chunk. */
		drwav_uint64_t sizeInBytes;

		/*
		RIFF = 2 byte alignment.
		W64  = 8 byte alignment.
		*/
		unsigned int paddingSize;
	} drwav_chunk_header;

	/*
	Callback for when data is read. Return value is the number of bytes actually
	read.

	pUserData   [in]  The user data that was passed to drwav_init() and family.
	pBufferOut  [out] The output buffer.
	bytesToRead [in]  The number of bytes to read.

	Returns the number of bytes actually read.

	A return value of less than bytesToRead indicates the end of the stream. Do
	_not_ return from this callback until either the entire bytesToRead is
	filled or you have reached the end of the stream.
	*/
	typedef size_t (*drwav_read_proc)(void *pUserData, void *pBufferOut,
	                                  size_t bytesToRead);

	/*
	Callback for when data is written. Returns value is the number of bytes
	actually written.

	pUserData    [in]  The user data that was passed to drwav_init_write() and
	family. pData        [out] A pointer to the data to write. bytesToWrite [in]
	The number of bytes to write.

	Returns the number of bytes actually written.

	If the return value differs from bytesToWrite, it indicates an error.
	*/
	typedef size_t (*drwav_write_proc)(void *pUserData, const void *pData,
	                                   size_t bytesToWrite);

	/*
	Callback for when data needs to be seeked.

	pUserData [in] The user data that was passed to drwav_init() and family.
	offset    [in] The number of bytes to move, relative to the origin. Will
	never be negative. origin    [in] The origin of the seek - the current
	position or the start of the stream.

	Returns whether or not the seek was successful.

	Whether or not it is relative to the beginning or current position is
	determined by the "origin" parameter which will be either
	drwav_seek_origin_start or drwav_seek_origin_current.
	*/
	typedef drwav_bool32 (*drwav_seek_proc)(void *pUserData, int offset,
	                                        drwav_seek_origin origin);

	/*
	Callback for when drwav_init_ex() finds a chunk.

	pChunkUserData    [in] The user data that was passed to the pChunkUserData
	parameter of drwav_init_ex() and family. onRead            [in] A pointer to
	the function to call when reading. onSeek            [in] A pointer to the
	function to call when seeking. pReadSeekUserData [in] The user data that was
	passed to the pReadSeekUserData parameter of drwav_init_ex() and family.
	pChunkHeader      [in] A pointer to an object containing basic header
	information about the chunk. Use this to identify the chunk.

	Returns the number of bytes read + seeked.

	To read data from the chunk, call onRead(), passing in pReadSeekUserData as
	the first parameter. Do the same for seeking with onSeek(). The return value
	must be the total number of bytes you have read _plus_ seeked.

	You must not attempt to read beyond the boundary of the chunk.
	*/
	typedef drwav_uint64_t (*drwav_chunk_proc)(
	    void *pChunkUserData, drwav_read_proc onRead, drwav_seek_proc onSeek,
	    void *pReadSeekUserData, const drwav_chunk_header *pChunkHeader);

	typedef struct
	{
		void *pUserData;
		void *(*onMalloc)(size_t sz, void *pUserData);
		void *(*onRealloc)(void *p, size_t sz, void *pUserData);
		void (*onFree)(void *p, void *pUserData);
	} drwav_allocation_callbacks;

	/* Structure for internal use. Only used for loaders opened with
	 * drwav_init_memory(). */
	typedef struct
	{
		const drwav_uint8_t *data;
		size_t dataSize;
		size_t currentReadPos;
	} drwav__memory_stream;

	/* Structure for internal use. Only used for writers opened with
	 * drwav_init_memory_write(). */
	typedef struct
	{
		void **ppData;
		size_t *pDataSize;
		size_t dataSize;
		size_t dataCapacity;
		size_t currentWritePos;
	} drwav__memory_stream_write;

	typedef struct
	{
		drwav_container container; /* RIFF, W64. */
		drwav_uint32_t format;     /* DR_WAVE_FORMAT_* */
		drwav_uint32_t channels;
		drwav_uint32_t sampleRate;
		drwav_uint32_t bitsPerSample;
	} drwav_data_format;

	typedef struct
	{
		/*
		The format tag exactly as specified in the wave file's "fmt" chunk. This
		can be used by applications that require support for data formats not
		natively supported by dr_wav.
		*/
		drwav_uint16_t formatTag;

		/* The number of channels making up the audio data. When this is set to
		 * 1 it is mono, 2 is stereo, etc. */
		drwav_uint16_t channels;

		/* The sample rate. Usually set to something like 44100. */
		drwav_uint32_t sampleRate;

		/* Average bytes per second. You probably don't need this, but it's left
		 * here for informational purposes. */
		drwav_uint32_t avgBytesPerSec;

		/* Block align. This is equal to the number of channels * bytes per
		 * sample. */
		drwav_uint16_t blockAlign;

		/* Bits per sample. */
		drwav_uint16_t bitsPerSample;

		/* The size of the extended data. Only used internally for validation,
		 * but left here for informational purposes. */
		drwav_uint16_t extendedSize;

		/*
		The number of valid bits per sample. When <formatTag> is equal to
		WAVE_FORMAT_EXTENSIBLE, <bitsPerSample> is always rounded up to the
		nearest multiple of 8. This variable contains information about exactly
		how many bits a valid per sample. Mainly used for informational
		purposes.
		*/
		drwav_uint16_t validBitsPerSample;

		/* The channel mask. Not used at the moment. */
		drwav_uint32_t channelMask;

		/* The sub-format, exactly as specified by the wave file. */
		drwav_uint8_t subFormat[16];
	} drwav_fmt;

	typedef struct
	{
		drwav_uint32_t cuePointId;
		drwav_uint32_t type;
		drwav_uint32_t start;
		drwav_uint32_t end;
		drwav_uint32_t fraction;
		drwav_uint32_t playCount;
	} drwav_smpl_loop;

	typedef struct
	{
		drwav_uint32_t manufacturer;
		drwav_uint32_t product;
		drwav_uint32_t samplePeriod;
		drwav_uint32_t midiUnityNotes;
		drwav_uint32_t midiPitchFraction;
		drwav_uint32_t smpteFormat;
		drwav_uint32_t smpteOffset;
		drwav_uint32_t numSampleLoops;
		drwav_uint32_t samplerData;
		drwav_smpl_loop loops[DRWAV_MAX_SMPL_LOOPS];
	} drwav_smpl;

	typedef struct
	{
		/* A pointer to the function to call when more data is needed. */
		drwav_read_proc onRead;

		/* A pointer to the function to call when data needs to be written. Only
		 * used when the drwav object is opened in write mode. */
		drwav_write_proc onWrite;

		/* A pointer to the function to call when the wav file needs to be
		 * seeked. */
		drwav_seek_proc onSeek;

		/* The user data to pass to callbacks. */
		void *pUserData;

		/* Allocation callbacks. */
		drwav_allocation_callbacks allocationCallbacks;

		/* Whether or not the WAV file is formatted as a standard RIFF file or
		 * W64. */
		drwav_container container;

		/* Structure containing format information exactly as specified by the
		 * wav file. */
		drwav_fmt fmt;

		/* The sample rate. Will be set to something like 44100. */
		drwav_uint32_t sampleRate;

		/* The number of channels. This will be set to 1 for monaural streams, 2
		 * for stereo, etc. */
		drwav_uint16_t channels;

		/* The bits per sample. Will be set to something like 16, 24, etc. */
		drwav_uint16_t bitsPerSample;

		/* Equal to fmt.formatTag, or the value specified by fmt.subFormat if
		 * fmt.formatTag is equal to 65534 (WAVE_FORMAT_EXTENSIBLE). */
		drwav_uint16_t translatedFormatTag;

		/* The total number of PCM frames making up the audio data. */
		drwav_uint64_t totalPCMFrameCount;

		/* The size in bytes of the data chunk. */
		drwav_uint64_t dataChunkDataSize;

		/* The position in the stream of the first byte of the data chunk. This
		 * is used for seeking. */
		drwav_uint64_t dataChunkDataPos;

		/* The number of bytes remaining in the data chunk. */
		drwav_uint64_t bytesRemaining;

		/*
		Only used in sequential write mode. Keeps track of the desired size of
		the "data" chunk at the point of initialization time. Always set to 0
		for non-sequential writes and when the drwav object is opened in read
		mode. Used for validation.
		*/
		drwav_uint64_t dataChunkDataSizeTargetWrite;

		/* Keeps track of whether or not the wav writer was initialized in
		 * sequential mode. */
		drwav_bool32 isSequentialWrite;

		/* smpl chunk. */
		drwav_smpl smpl;

		/* A hack to avoid a DRWAV_MALLOC() when opening a decoder with
		 * drwav_init_memory(). */
		drwav__memory_stream memoryStream;
		drwav__memory_stream_write memoryStreamWrite;

		/* Generic data for compressed formats. This data is shared across all
		 * block-compressed formats. */
		struct
		{
			drwav_uint64_t
			    iCurrentPCMFrame; /* The index of the next PCM frame that will
			                         be read by drwav_read_*(). This is used
			                         with "totalPCMFrameCount" to ensure we
			                         don't read excess samples at the end of the
			                         last block. */
		} compressed;

		/* Microsoft ADPCM specific data. */
		struct
		{
			drwav_uint32_t bytesRemainingInBlock;
			drwav_uint16_t predictor[2];
			drwav_int32_t delta[2];
			drwav_int32_t cachedFrames[4]; /* Samples are stored in this cache
			                                  during decoding. */
			drwav_uint32_t cachedFrameCount;
			drwav_int32_t prevFrames[2][2]; /* The previous 2 samples for each
			                                   channel (2 channels at most). */
		} msadpcm;

		/* IMA ADPCM specific data. */
		struct
		{
			drwav_uint32_t bytesRemainingInBlock;
			drwav_int32_t predictor[2];
			drwav_int32_t stepIndex[2];
			drwav_int32_t cachedFrames[16]; /* Samples are stored in this cache
			                                   during decoding. */
			drwav_uint32_t cachedFrameCount;
		} ima;
	} drwav;

/* Standard library stuff. */
#ifndef DRWAV_ASSERT
#include <assert.h>
#define DRWAV_ASSERT(expression) assert(expression)
#endif
#ifndef DRWAV_MALLOC
#define DRWAV_MALLOC(sz) malloc((sz))
#endif
#ifndef DRWAV_REALLOC
#define DRWAV_REALLOC(p, sz) realloc((p), (sz))
#endif
#ifndef DRWAV_FREE
#define DRWAV_FREE(p) free((p))
#endif
#ifndef DRWAV_COPY_MEMORY
#define DRWAV_COPY_MEMORY(dst, src, sz) memcpy((dst), (src), (sz))
#endif
#ifndef DRWAV_ZERO_MEMORY
#define DRWAV_ZERO_MEMORY(p, sz) memset((p), 0, (sz))
#endif

#define drwav_countof(x) (sizeof(x) / sizeof(x[0]))
#define drwav_align(x, a) ((((x) + (a)-1) / (a)) * (a))
#define drwav_min(a, b) (((a) < (b)) ? (a) : (b))
#define drwav_max(a, b) (((a) > (b)) ? (a) : (b))
#define drwav_clamp(x, lo, hi) (drwav_max((lo), drwav_min((hi), (x))))

#define DRWAV_MAX_SIMD_VECTOR_SIZE 64 /* 64 for AVX-512 in the future. */

/* CPU architecture. */
#if defined(__x86_64__) || defined(_M_X64)
#define DRWAV_X64
#elif defined(__i386) || defined(_M_IX86)
#define DRWAV_X86
#elif defined(__arm__) || defined(_M_ARM)
#define DRWAV_ARM
#endif

#ifdef _MSC_VER
#define DRWAV_INLINE __forceinline
#elif defined(__GNUC__)
/*
I've had a bug report where GCC is emitting warnings about functions possibly
not being inlineable. This warning happens when the
__attribute__((always_inline)) attribute is defined without an "inline"
statement. I think therefore there must be some case where "__inline__" is not
always defined, thus the compiler emitting these warnings. When using -std=c89
or -ansi on the command line, we cannot use the "inline" keyword and instead
need to use "__inline__". In an attempt to work around this issue I am using
"__inline__" only when we're compiling in strict ANSI mode.
*/
#if defined(__STRICT_ANSI__)
#define DRWAV_INLINE __inline__ __attribute__((always_inline))
#else
#define DRWAV_INLINE inline __attribute__((always_inline))
#endif
#else
#define DRWAV_INLINE
#endif

#if defined(SIZE_MAX)
#define DRWAV_SIZE_MAX SIZE_MAX
#else
#if defined(_WIN64) || defined(_LP64) || defined(__LP64__)
#define DRWAV_SIZE_MAX ((drwav_uint64_t)0xFFFFFFFFFFFFFFFF)
#else
#define DRWAV_SIZE_MAX 0xFFFFFFFF
#endif
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1300
#define DRWAV_HAS_BYTESWAP16_INTRINSIC
#define DRWAV_HAS_BYTESWAP32_INTRINSIC
#define DRWAV_HAS_BYTESWAP64_INTRINSIC
#elif defined(__clang__)
#if defined(__has_builtin)
#if __has_builtin(__builtin_bswap16)
#define DRWAV_HAS_BYTESWAP16_INTRINSIC
#endif
#if __has_builtin(__builtin_bswap32)
#define DRWAV_HAS_BYTESWAP32_INTRINSIC
#endif
#if __has_builtin(__builtin_bswap64)
#define DRWAV_HAS_BYTESWAP64_INTRINSIC
#endif
#endif
#elif defined(__GNUC__)
#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
#define DRWAV_HAS_BYTESWAP32_INTRINSIC
#define DRWAV_HAS_BYTESWAP64_INTRINSIC
#endif
#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
#define DRWAV_HAS_BYTESWAP16_INTRINSIC
#endif
#endif

	extern const drwav_uint8_t
	    drwavGUID_W64_RIFF[16]; /* 66666972-912E-11CF-A5D6-28DB04C10000 */
	extern const drwav_uint8_t
	    drwavGUID_W64_WAVE[16]; /* 65766177-ACF3-11D3-8CD1-00C04F8EDB8A */
	extern const drwav_uint8_t
	    drwavGUID_W64_JUNK[16]; /* 6B6E756A-ACF3-11D3-8CD1-00C04F8EDB8A */
	extern const drwav_uint8_t
	    drwavGUID_W64_FMT[16]; /* 20746D66-ACF3-11D3-8CD1-00C04F8EDB8A */
	extern const drwav_uint8_t
	    drwavGUID_W64_FACT[16]; /* 74636166-ACF3-11D3-8CD1-00C04F8EDB8A */
	extern const drwav_uint8_t
	    drwavGUID_W64_DATA[16]; /* 61746164-ACF3-11D3-8CD1-00C04F8EDB8A */
	extern const drwav_uint8_t
	    drwavGUID_W64_SMPL[16]; /* 6C706D73-ACF3-11D3-8CD1-00C04F8EDB8A */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // DR_WAV_TYPES_H
