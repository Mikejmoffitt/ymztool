#ifndef DR_WAV_UTIL_H
#define DR_WAV_UTIL_H

#include "3rdparty/dr_wav/dr_wav_types.h"

#include <limits.h> /* For INT_MAX */
#include <stdlib.h>
#include <string.h> /* For memcpy(), memset() */

#ifndef DR_WAV_NO_STDIO
#include <stdio.h>
#include <wchar.h>
#endif

unsigned int drwav__chunk_padding_size_riff(drwav_uint64_t chunkSize);
unsigned int drwav__chunk_padding_size_w64(drwav_uint64_t chunkSize);

drwav_allocation_callbacks drwav_copy_allocation_callbacks_or_defaults(
    const drwav_allocation_callbacks *pAllocationCallbacks);

void *drwav__malloc_from_callbacks(
    size_t sz, const drwav_allocation_callbacks *pAllocationCallbacks);
void *drwav__realloc_from_callbacks(
    void *p, size_t szNew, size_t szOld,
    const drwav_allocation_callbacks *pAllocationCallbacks);
void drwav__free_from_callbacks(
    void *p, const drwav_allocation_callbacks *pAllocationCallbacks);

static void *drwav__malloc_default(size_t sz, void *pUserData)
{
	(void)pUserData;
	return DRWAV_MALLOC(sz);
}

static void *drwav__realloc_default(void *p, size_t sz, void *pUserData)
{
	(void)pUserData;
	return DRWAV_REALLOC(p, sz);
}

static void drwav__free_default(void *p, void *pUserData)
{
	(void)pUserData;
	DRWAV_FREE(p);
}

static DRWAV_INLINE drwav_bool32 drwav__guid_equal(const drwav_uint8_t a[16],
                                                   const drwav_uint8_t b[16])
{
	const drwav_uint32_t *a32 = (const drwav_uint32_t *)a;
	const drwav_uint32_t *b32 = (const drwav_uint32_t *)b;

	return a32[0] == b32[0] && a32[1] == b32[1] && a32[2] == b32[2] &&
	       a32[3] == b32[3];
}

static DRWAV_INLINE drwav_bool32 drwav__fourcc_equal(const unsigned char *a,
                                                     const char *b)
{
	return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

static DRWAV_INLINE int drwav__is_little_endian()
{
#if defined(DRWAV_X86) || defined(DRWAV_X64)
	return DRWAV_TRUE;
#elif defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) &&                     \
    __BYTE_ORDER == __LITTLE_ENDIAN
	return DRWAV_TRUE;
#else
	int n = 1;
	return (*(char *)&n) == 1;
#endif
}

static DRWAV_INLINE unsigned short
drwav__bytes_to_u16(const unsigned char *data)
{
	return (data[0] << 0) | (data[1] << 8);
}

static DRWAV_INLINE short drwav__bytes_to_s16(const unsigned char *data)
{
	return (short)drwav__bytes_to_u16(data);
}

static DRWAV_INLINE unsigned int drwav__bytes_to_u32(const unsigned char *data)
{
	return (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

static DRWAV_INLINE drwav_uint64_t
drwav__bytes_to_u64(const unsigned char *data)
{
	return ((drwav_uint64_t)data[0] << 0) | ((drwav_uint64_t)data[1] << 8) |
	       ((drwav_uint64_t)data[2] << 16) | ((drwav_uint64_t)data[3] << 24) |
	       ((drwav_uint64_t)data[4] << 32) | ((drwav_uint64_t)data[5] << 40) |
	       ((drwav_uint64_t)data[6] << 48) | ((drwav_uint64_t)data[7] << 56);
}

static DRWAV_INLINE void drwav__bytes_to_guid(const unsigned char *data,
                                              drwav_uint8_t *guid)
{
	int i;
	for (i = 0; i < 16; ++i) { guid[i] = data[i]; }
}
static DRWAV_INLINE drwav_uint16_t drwav__bswap16(drwav_uint16_t n)
{
#ifdef DRWAV_HAS_BYTESWAP16_INTRINSIC
#if defined(_MSC_VER)
	return _byteswap_ushort(n);
#elif defined(__GNUC__) || defined(__clang__)
	return __builtin_bswap16(n);
#else
#error "This compiler does not support the byte swap intrinsic."
#endif
#else
	return ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
#endif
}

static DRWAV_INLINE drwav_uint32_t drwav__bswap32(drwav_uint32_t n)
{
#ifdef DRWAV_HAS_BYTESWAP32_INTRINSIC
#if defined(_MSC_VER)
	return _byteswap_ulong(n);
#elif defined(__GNUC__) || defined(__clang__)
#if defined(DRWAV_ARM) && (defined(__ARM_ARCH) && __ARM_ARCH >= 6) &&          \
    !defined(DRWAV_64BIT) /* <-- 64-bit inline assembly has not been tested,   \
                             so disabling for now. */
	/* Inline assembly optimized implementation for ARM. In my testing, GCC does
	 * not generate optimized code with __builtin_bswap32(). */
	drwav_uint32_t r;
	__asm__ __volatile__(
#if defined(DRWAV_64BIT)
	    "rev %w[out], %w[in]"
	    : [out] "=r"(r)
	    : [in] "r"(n) /* <-- This is untested. If someone in the community could
	                     test this, that would be appreciated! */
#else
	    "rev %[out], %[in]"
	    : [out] "=r"(r)
	    : [in] "r"(n)
#endif
	);
	return r;
#else
	return __builtin_bswap32(n);
#endif
#else
#error "This compiler does not support the byte swap intrinsic."
#endif
#else
	return ((n & 0xFF000000) >> 24) | ((n & 0x00FF0000) >> 8) |
	       ((n & 0x0000FF00) << 8) | ((n & 0x000000FF) << 24);
#endif
}

static DRWAV_INLINE drwav_uint64_t drwav__bswap64(drwav_uint64_t n)
{
	// TODO: Determine why MSVC isn't having it with this one. Furthermore, why
	// was it ok with Gimmick?
	/*
#ifdef DRWAV_HAS_BYTESWAP64_INTRINSIC and 0
	#if defined(_MSC_VER)
	    return _byteswap_uint64_t(n);
	#elif defined(__GNUC__) || defined(__clang__)
	    return __builtin_bswap64(n);
	#else
	    #error "This compiler does not support the byte swap intrinsic."
	#endif
#else
	*/
	return ((n & (drwav_uint64_t)0xFF00000000000000) >> 56) |
	       ((n & (drwav_uint64_t)0x00FF000000000000) >> 40) |
	       ((n & (drwav_uint64_t)0x0000FF0000000000) >> 24) |
	       ((n & (drwav_uint64_t)0x000000FF00000000) >> 8) |
	       ((n & (drwav_uint64_t)0x00000000FF000000) << 8) |
	       ((n & (drwav_uint64_t)0x0000000000FF0000) << 24) |
	       ((n & (drwav_uint64_t)0x000000000000FF00) << 40) |
	       ((n & (drwav_uint64_t)0x00000000000000FF) << 56);
	// #endif
}

static DRWAV_INLINE drwav_int16_t drwav__bswap_s16(drwav_int16_t n)
{
	return (drwav_int16_t)drwav__bswap16((drwav_uint16_t)n);
}

static DRWAV_INLINE void drwav__bswap_samples_s16(drwav_int16_t *pSamples,
                                                  drwav_uint64_t sampleCount)
{
	drwav_uint64_t iSample;
	for (iSample = 0; iSample < sampleCount; iSample += 1)
	{
		pSamples[iSample] = drwav__bswap_s16(pSamples[iSample]);
	}
}

static DRWAV_INLINE void drwav__bswap_s24(drwav_uint8_t *p)
{
	drwav_uint8_t t;
	t = p[0];
	p[0] = p[2];
	p[2] = t;
}

static DRWAV_INLINE void drwav__bswap_samples_s24(drwav_uint8_t *pSamples,
                                                  drwav_uint64_t sampleCount)
{
	drwav_uint64_t iSample;
	for (iSample = 0; iSample < sampleCount; iSample += 1)
	{
		drwav_uint8_t *pSample = pSamples + (iSample * 3);
		drwav__bswap_s24(pSample);
	}
}

static DRWAV_INLINE drwav_int32_t drwav__bswap_s32(drwav_int32_t n)
{
	return (drwav_int32_t)drwav__bswap32((drwav_uint32_t)n);
}

static DRWAV_INLINE void drwav__bswap_samples_s32(drwav_int32_t *pSamples,
                                                  drwav_uint64_t sampleCount)
{
	drwav_uint64_t iSample;
	for (iSample = 0; iSample < sampleCount; iSample += 1)
	{
		pSamples[iSample] = drwav__bswap_s32(pSamples[iSample]);
	}
}

static DRWAV_INLINE float drwav__bswap_f32(float n)
{
	union
	{
		drwav_uint32_t i;
		float f;
	} x;
	x.f = n;
	x.i = drwav__bswap32(x.i);

	return x.f;
}

static DRWAV_INLINE void drwav__bswap_samples_f32(float *pSamples,
                                                  drwav_uint64_t sampleCount)
{
	drwav_uint64_t iSample;
	for (iSample = 0; iSample < sampleCount; iSample += 1)
	{
		pSamples[iSample] = drwav__bswap_f32(pSamples[iSample]);
	}
}

static DRWAV_INLINE double drwav__bswap_f64(double n)
{
	union
	{
		drwav_uint64_t i;
		double f;
	} x;
	x.f = n;
	x.i = drwav__bswap64(x.i);

	return x.f;
}

static DRWAV_INLINE void drwav__bswap_samples_f64(double *pSamples,
                                                  drwav_uint64_t sampleCount)
{
	drwav_uint64_t iSample;
	for (iSample = 0; iSample < sampleCount; iSample += 1)
	{
		pSamples[iSample] = drwav__bswap_f64(pSamples[iSample]);
	}
}

static DRWAV_INLINE void drwav__bswap_samples_pcm(void *pSamples,
                                                  drwav_uint64_t sampleCount,
                                                  drwav_uint32_t bytesPerSample)
{
	/* Assumes integer PCM. Floating point PCM is done in
	 * drwav__bswap_samples_ieee(). */
	switch (bytesPerSample)
	{
		case 2: /* s16, s12 (loosely packed) */
		{
			drwav__bswap_samples_s16((drwav_int16_t *)pSamples, sampleCount);
		}
		break;
		case 3: /* s24 */
		{
			drwav__bswap_samples_s24((drwav_uint8_t *)pSamples, sampleCount);
		}
		break;
		case 4: /* s32 */
		{
			drwav__bswap_samples_s32((drwav_int32_t *)pSamples, sampleCount);
		}
		break;
		default:
		{
			/* Unsupported format. */
			DRWAV_ASSERT(DRWAV_FALSE);
		}
		break;
	}
}

static DRWAV_INLINE void
drwav__bswap_samples_ieee(void *pSamples, drwav_uint64_t sampleCount,
                          drwav_uint32_t bytesPerSample)
{
	switch (bytesPerSample)
	{
#if 0 /* Contributions welcome for f16 support. */
        case 2: /* f16 */
        {
            drwav__bswap_samples_f16((drwav_float16*)pSamples, sampleCount);
        } break;
#endif
		case 4: /* f32 */
		{
			drwav__bswap_samples_f32((float *)pSamples, sampleCount);
		}
		break;
		case 8: /* f64 */
		{
			drwav__bswap_samples_f64((double *)pSamples, sampleCount);
		}
		break;
		default:
		{
			/* Unsupported format. */
			DRWAV_ASSERT(DRWAV_FALSE);
		}
		break;
	}
}

static DRWAV_INLINE void drwav__bswap_samples(void *pSamples,
                                              drwav_uint64_t sampleCount,
                                              drwav_uint32_t bytesPerSample,
                                              drwav_uint16_t format)
{
	switch (format)
	{
		case DR_WAVE_FORMAT_PCM:
		{
			drwav__bswap_samples_pcm(pSamples, sampleCount, bytesPerSample);
		}
		break;

		case DR_WAVE_FORMAT_IEEE_FLOAT:
		{
			drwav__bswap_samples_ieee(pSamples, sampleCount, bytesPerSample);
		}
		break;

		case DR_WAVE_FORMAT_ALAW:
		case DR_WAVE_FORMAT_MULAW:
		{
			drwav__bswap_samples_s16((drwav_int16_t *)pSamples, sampleCount);
		}
		break;

		case DR_WAVE_FORMAT_ADPCM:
		case DR_WAVE_FORMAT_DVI_ADPCM:
		default:
		{
			/* Unsupported format. */
			DRWAV_ASSERT(DRWAV_FALSE);
		}
		break;
	}
}

drwav_uint32_t drwav_get_bytes_per_pcm_frame(drwav *pWav);
drwav_uint32_t drwav__riff_chunk_size_riff(drwav_uint64_t dataChunkSize);
drwav_uint32_t drwav__data_chunk_size_riff(drwav_uint64_t dataChunkSize);
drwav_uint64_t drwav__riff_chunk_size_w64(drwav_uint64_t dataChunkSize);
drwav_uint64_t drwav__data_chunk_size_w64(drwav_uint64_t dataChunkSize);

/*
Utility function to determine the target size of the entire data to be written
(including all headers and chunks).

Returns the target size in bytes.

Useful if the application needs to know the size to allocate.

Only writing to the RIFF chunk and one data chunk is currently supported.

See also: drwav_init_write(), drwav_init_file_write(), drwav_init_memory_write()
*/
drwav_uint64_t drwav_target_write_size_bytes(drwav_data_format const *format,
                                             drwav_uint64_t totalSampleCount);

/*
Reads raw audio data.

This is the lowest level function for reading audio data. It simply reads the
given number of bytes of the raw internal sample data.

Consider using drwav_read_pcm_frames_s16(), drwav_read_pcm_frames_s32() or
drwav_read_pcm_frames_f32() for reading sample data in a consistent format.

Returns the number of bytes actually read.
*/
size_t drwav_read_raw(drwav *pWav, size_t bytesToRead, void *pBufferOut);

/* Frees data that was allocated internally by dr_wav. */
void drwav_free(void *p,
                const drwav_allocation_callbacks *pAllocationCallbacks);

#endif // DR_WAV_UTIL_H
