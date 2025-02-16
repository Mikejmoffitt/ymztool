#include "3rdparty/dr_wav/dr_wav_pcm_conv.h"
#include "3rdparty/dr_wav/dr_wav_init.h"
#include "3rdparty/dr_wav/dr_wav_pcm.h"
#include "3rdparty/dr_wav/dr_wav_util.h"

#ifndef DR_WAV_NO_CONVERSION_API
static unsigned short g_drwavAlawTable[256] = {
    0xEA80, 0xEB80, 0xE880, 0xE980, 0xEE80, 0xEF80, 0xEC80, 0xED80, 0xE280,
    0xE380, 0xE080, 0xE180, 0xE680, 0xE780, 0xE480, 0xE580, 0xF540, 0xF5C0,
    0xF440, 0xF4C0, 0xF740, 0xF7C0, 0xF640, 0xF6C0, 0xF140, 0xF1C0, 0xF040,
    0xF0C0, 0xF340, 0xF3C0, 0xF240, 0xF2C0, 0xAA00, 0xAE00, 0xA200, 0xA600,
    0xBA00, 0xBE00, 0xB200, 0xB600, 0x8A00, 0x8E00, 0x8200, 0x8600, 0x9A00,
    0x9E00, 0x9200, 0x9600, 0xD500, 0xD700, 0xD100, 0xD300, 0xDD00, 0xDF00,
    0xD900, 0xDB00, 0xC500, 0xC700, 0xC100, 0xC300, 0xCD00, 0xCF00, 0xC900,
    0xCB00, 0xFEA8, 0xFEB8, 0xFE88, 0xFE98, 0xFEE8, 0xFEF8, 0xFEC8, 0xFED8,
    0xFE28, 0xFE38, 0xFE08, 0xFE18, 0xFE68, 0xFE78, 0xFE48, 0xFE58, 0xFFA8,
    0xFFB8, 0xFF88, 0xFF98, 0xFFE8, 0xFFF8, 0xFFC8, 0xFFD8, 0xFF28, 0xFF38,
    0xFF08, 0xFF18, 0xFF68, 0xFF78, 0xFF48, 0xFF58, 0xFAA0, 0xFAE0, 0xFA20,
    0xFA60, 0xFBA0, 0xFBE0, 0xFB20, 0xFB60, 0xF8A0, 0xF8E0, 0xF820, 0xF860,
    0xF9A0, 0xF9E0, 0xF920, 0xF960, 0xFD50, 0xFD70, 0xFD10, 0xFD30, 0xFDD0,
    0xFDF0, 0xFD90, 0xFDB0, 0xFC50, 0xFC70, 0xFC10, 0xFC30, 0xFCD0, 0xFCF0,
    0xFC90, 0xFCB0, 0x1580, 0x1480, 0x1780, 0x1680, 0x1180, 0x1080, 0x1380,
    0x1280, 0x1D80, 0x1C80, 0x1F80, 0x1E80, 0x1980, 0x1880, 0x1B80, 0x1A80,
    0x0AC0, 0x0A40, 0x0BC0, 0x0B40, 0x08C0, 0x0840, 0x09C0, 0x0940, 0x0EC0,
    0x0E40, 0x0FC0, 0x0F40, 0x0CC0, 0x0C40, 0x0DC0, 0x0D40, 0x5600, 0x5200,
    0x5E00, 0x5A00, 0x4600, 0x4200, 0x4E00, 0x4A00, 0x7600, 0x7200, 0x7E00,
    0x7A00, 0x6600, 0x6200, 0x6E00, 0x6A00, 0x2B00, 0x2900, 0x2F00, 0x2D00,
    0x2300, 0x2100, 0x2700, 0x2500, 0x3B00, 0x3900, 0x3F00, 0x3D00, 0x3300,
    0x3100, 0x3700, 0x3500, 0x0158, 0x0148, 0x0178, 0x0168, 0x0118, 0x0108,
    0x0138, 0x0128, 0x01D8, 0x01C8, 0x01F8, 0x01E8, 0x0198, 0x0188, 0x01B8,
    0x01A8, 0x0058, 0x0048, 0x0078, 0x0068, 0x0018, 0x0008, 0x0038, 0x0028,
    0x00D8, 0x00C8, 0x00F8, 0x00E8, 0x0098, 0x0088, 0x00B8, 0x00A8, 0x0560,
    0x0520, 0x05E0, 0x05A0, 0x0460, 0x0420, 0x04E0, 0x04A0, 0x0760, 0x0720,
    0x07E0, 0x07A0, 0x0660, 0x0620, 0x06E0, 0x06A0, 0x02B0, 0x0290, 0x02F0,
    0x02D0, 0x0230, 0x0210, 0x0270, 0x0250, 0x03B0, 0x0390, 0x03F0, 0x03D0,
    0x0330, 0x0310, 0x0370, 0x0350};

static unsigned short g_drwavMulawTable[256] = {
    0x8284, 0x8684, 0x8A84, 0x8E84, 0x9284, 0x9684, 0x9A84, 0x9E84, 0xA284,
    0xA684, 0xAA84, 0xAE84, 0xB284, 0xB684, 0xBA84, 0xBE84, 0xC184, 0xC384,
    0xC584, 0xC784, 0xC984, 0xCB84, 0xCD84, 0xCF84, 0xD184, 0xD384, 0xD584,
    0xD784, 0xD984, 0xDB84, 0xDD84, 0xDF84, 0xE104, 0xE204, 0xE304, 0xE404,
    0xE504, 0xE604, 0xE704, 0xE804, 0xE904, 0xEA04, 0xEB04, 0xEC04, 0xED04,
    0xEE04, 0xEF04, 0xF004, 0xF0C4, 0xF144, 0xF1C4, 0xF244, 0xF2C4, 0xF344,
    0xF3C4, 0xF444, 0xF4C4, 0xF544, 0xF5C4, 0xF644, 0xF6C4, 0xF744, 0xF7C4,
    0xF844, 0xF8A4, 0xF8E4, 0xF924, 0xF964, 0xF9A4, 0xF9E4, 0xFA24, 0xFA64,
    0xFAA4, 0xFAE4, 0xFB24, 0xFB64, 0xFBA4, 0xFBE4, 0xFC24, 0xFC64, 0xFC94,
    0xFCB4, 0xFCD4, 0xFCF4, 0xFD14, 0xFD34, 0xFD54, 0xFD74, 0xFD94, 0xFDB4,
    0xFDD4, 0xFDF4, 0xFE14, 0xFE34, 0xFE54, 0xFE74, 0xFE8C, 0xFE9C, 0xFEAC,
    0xFEBC, 0xFECC, 0xFEDC, 0xFEEC, 0xFEFC, 0xFF0C, 0xFF1C, 0xFF2C, 0xFF3C,
    0xFF4C, 0xFF5C, 0xFF6C, 0xFF7C, 0xFF88, 0xFF90, 0xFF98, 0xFFA0, 0xFFA8,
    0xFFB0, 0xFFB8, 0xFFC0, 0xFFC8, 0xFFD0, 0xFFD8, 0xFFE0, 0xFFE8, 0xFFF0,
    0xFFF8, 0x0000, 0x7D7C, 0x797C, 0x757C, 0x717C, 0x6D7C, 0x697C, 0x657C,
    0x617C, 0x5D7C, 0x597C, 0x557C, 0x517C, 0x4D7C, 0x497C, 0x457C, 0x417C,
    0x3E7C, 0x3C7C, 0x3A7C, 0x387C, 0x367C, 0x347C, 0x327C, 0x307C, 0x2E7C,
    0x2C7C, 0x2A7C, 0x287C, 0x267C, 0x247C, 0x227C, 0x207C, 0x1EFC, 0x1DFC,
    0x1CFC, 0x1BFC, 0x1AFC, 0x19FC, 0x18FC, 0x17FC, 0x16FC, 0x15FC, 0x14FC,
    0x13FC, 0x12FC, 0x11FC, 0x10FC, 0x0FFC, 0x0F3C, 0x0EBC, 0x0E3C, 0x0DBC,
    0x0D3C, 0x0CBC, 0x0C3C, 0x0BBC, 0x0B3C, 0x0ABC, 0x0A3C, 0x09BC, 0x093C,
    0x08BC, 0x083C, 0x07BC, 0x075C, 0x071C, 0x06DC, 0x069C, 0x065C, 0x061C,
    0x05DC, 0x059C, 0x055C, 0x051C, 0x04DC, 0x049C, 0x045C, 0x041C, 0x03DC,
    0x039C, 0x036C, 0x034C, 0x032C, 0x030C, 0x02EC, 0x02CC, 0x02AC, 0x028C,
    0x026C, 0x024C, 0x022C, 0x020C, 0x01EC, 0x01CC, 0x01AC, 0x018C, 0x0174,
    0x0164, 0x0154, 0x0144, 0x0134, 0x0124, 0x0114, 0x0104, 0x00F4, 0x00E4,
    0x00D4, 0x00C4, 0x00B4, 0x00A4, 0x0094, 0x0084, 0x0078, 0x0070, 0x0068,
    0x0060, 0x0058, 0x0050, 0x0048, 0x0040, 0x0038, 0x0030, 0x0028, 0x0020,
    0x0018, 0x0010, 0x0008, 0x0000};

static DRWAV_INLINE drwav_int16_t drwav__alaw_to_s16(drwav_uint8_t sampleIn)
{
	return (short)g_drwavAlawTable[sampleIn];
}

static DRWAV_INLINE drwav_int16_t drwav__mulaw_to_s16(drwav_uint8_t sampleIn)
{
	return (short)g_drwavMulawTable[sampleIn];
}

static void drwav__pcm_to_s16(drwav_int16_t *pOut, const unsigned char *pIn,
                              size_t totalSampleCount,
                              unsigned int bytesPerSample)
{
	unsigned int i;

	/* Special case for 8-bit sample data because it's treated as unsigned. */
	if (bytesPerSample == 1)
	{
		drwav_u8_to_s16(pOut, pIn, totalSampleCount);
		return;
	}

	/* Slightly more optimal implementation for common formats. */
	if (bytesPerSample == 2)
	{
		for (i = 0; i < totalSampleCount; ++i)
		{
			*pOut++ = ((const drwav_int16_t *)pIn)[i];
		}
		return;
	}
	if (bytesPerSample == 3)
	{
		drwav_s24_to_s16(pOut, pIn, totalSampleCount);
		return;
	}
	if (bytesPerSample == 4)
	{
		drwav_s32_to_s16(pOut, (const drwav_int32_t *)pIn, totalSampleCount);
		return;
	}

	/* Anything more than 64 bits per sample is not supported. */
	if (bytesPerSample > 8)
	{
		DRWAV_ZERO_MEMORY(pOut, totalSampleCount * sizeof(*pOut));
		return;
	}

	/* Generic, slow converter. */
	for (i = 0; i < totalSampleCount; ++i)
	{
		drwav_uint64_t sample = 0;
		unsigned int shift = (8 - bytesPerSample) * 8;

		unsigned int j;
		for (j = 0; j < bytesPerSample && j < 8; j += 1)
		{
			sample |= (drwav_uint64_t)(pIn[j]) << shift;
			shift += 8;
		}

		pIn += j;
		*pOut++ = (drwav_int16_t)((drwav_int64_t)sample >> 48);
	}
}

static void drwav__ieee_to_s16(drwav_int16_t *pOut, const unsigned char *pIn,
                               size_t totalSampleCount,
                               unsigned int bytesPerSample)
{
	if (bytesPerSample == 4)
	{
		drwav_f32_to_s16(pOut, (const float *)pIn, totalSampleCount);
		return;
	}
	else if (bytesPerSample == 8)
	{
		drwav_f64_to_s16(pOut, (const double *)pIn, totalSampleCount);
		return;
	}
	else
	{
		/* Only supporting 32- and 64-bit float. Output silence in all other
		 * cases. Contributions welcome for 16-bit float. */
		DRWAV_ZERO_MEMORY(pOut, totalSampleCount * sizeof(*pOut));
		return;
	}
}

drwav_uint64_t drwav_read_pcm_frames_s16__pcm(drwav *pWav,
                                              drwav_uint64_t framesToRead,
                                              drwav_int16_t *pBufferOut)
{
	drwav_uint32_t bytesPerFrame;
	drwav_uint64_t totalFramesRead;
	unsigned char sampleData[4096];

	/* Fast path. */
	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_PCM &&
	    pWav->bitsPerSample == 16)
	{
		return drwav_read_pcm_frames(pWav, framesToRead, pBufferOut);
	}

	bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
	if (bytesPerFrame == 0)
	{
		return 0;
	}

	totalFramesRead = 0;

	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames(
		    pWav, drwav_min(framesToRead, sizeof(sampleData) / bytesPerFrame),
		    sampleData);
		if (framesRead == 0)
		{
			break;
		}

		drwav__pcm_to_s16(pBufferOut, sampleData,
		                  (size_t)(framesRead * pWav->channels),
		                  bytesPerFrame / pWav->channels);

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_s16__ieee(drwav *pWav,
                                               drwav_uint64_t framesToRead,
                                               drwav_int16_t *pBufferOut)
{
	drwav_uint64_t totalFramesRead;
	unsigned char sampleData[4096];

	drwav_uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
	if (bytesPerFrame == 0)
	{
		return 0;
	}

	totalFramesRead = 0;

	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames(
		    pWav, drwav_min(framesToRead, sizeof(sampleData) / bytesPerFrame),
		    sampleData);
		if (framesRead == 0)
		{
			break;
		}

		drwav__ieee_to_s16(pBufferOut, sampleData,
		                   (size_t)(framesRead * pWav->channels),
		                   bytesPerFrame / pWav->channels);

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_s16__alaw(drwav *pWav,
                                               drwav_uint64_t framesToRead,
                                               drwav_int16_t *pBufferOut)
{
	drwav_uint64_t totalFramesRead;
	unsigned char sampleData[4096];

	drwav_uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
	if (bytesPerFrame == 0)
	{
		return 0;
	}

	totalFramesRead = 0;

	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames(
		    pWav, drwav_min(framesToRead, sizeof(sampleData) / bytesPerFrame),
		    sampleData);
		if (framesRead == 0)
		{
			break;
		}

		drwav_alaw_to_s16(pBufferOut, sampleData,
		                  (size_t)(framesRead * pWav->channels));

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_s16__mulaw(drwav *pWav,
                                                drwav_uint64_t framesToRead,
                                                drwav_int16_t *pBufferOut)
{
	drwav_uint64_t totalFramesRead;
	unsigned char sampleData[4096];

	drwav_uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
	if (bytesPerFrame == 0)
	{
		return 0;
	}

	totalFramesRead = 0;

	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames(
		    pWav, drwav_min(framesToRead, sizeof(sampleData) / bytesPerFrame),
		    sampleData);
		if (framesRead == 0)
		{
			break;
		}

		drwav_mulaw_to_s16(pBufferOut, sampleData,
		                   (size_t)(framesRead * pWav->channels));

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_s16(drwav *pWav,
                                         drwav_uint64_t framesToRead,
                                         drwav_int16_t *pBufferOut)
{
	if (pWav == NULL || framesToRead == 0 || pBufferOut == NULL)
	{
		return 0;
	}

	/* Don't try to read more samples than can potentially fit in the output
	 * buffer. */
	if (framesToRead * pWav->channels * sizeof(drwav_int16_t) > DRWAV_SIZE_MAX)
	{
		framesToRead = DRWAV_SIZE_MAX / sizeof(drwav_int16_t) / pWav->channels;
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_PCM)
	{
		return drwav_read_pcm_frames_s16__pcm(pWav, framesToRead, pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_IEEE_FLOAT)
	{
		return drwav_read_pcm_frames_s16__ieee(pWav, framesToRead, pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ALAW)
	{
		return drwav_read_pcm_frames_s16__alaw(pWav, framesToRead, pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_MULAW)
	{
		return drwav_read_pcm_frames_s16__mulaw(pWav, framesToRead, pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM)
	{
		return drwav_read_pcm_frames_s16__msadpcm(pWav, framesToRead,
		                                          pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM)
	{
		return drwav_read_pcm_frames_s16__ima(pWav, framesToRead, pBufferOut);
	}

	return 0;
}

drwav_uint64_t drwav_read_pcm_frames_s16le(drwav *pWav,
                                           drwav_uint64_t framesToRead,
                                           drwav_int16_t *pBufferOut)
{
	drwav_uint64_t framesRead =
	    drwav_read_pcm_frames_s16(pWav, framesToRead, pBufferOut);
	if (!drwav__is_little_endian())
	{
		drwav__bswap_samples_s16(pBufferOut, framesRead * pWav->channels);
	}

	return framesRead;
}

drwav_uint64_t drwav_read_pcm_frames_s16be(drwav *pWav,
                                           drwav_uint64_t framesToRead,
                                           drwav_int16_t *pBufferOut)
{
	drwav_uint64_t framesRead =
	    drwav_read_pcm_frames_s16(pWav, framesToRead, pBufferOut);
	if (drwav__is_little_endian())
	{
		drwav__bswap_samples_s16(pBufferOut, framesRead * pWav->channels);
	}

	return framesRead;
}

void drwav_u8_to_s16(drwav_int16_t *pOut, const drwav_uint8_t *pIn,
                     size_t sampleCount)
{
	int r;
	size_t i;
	for (i = 0; i < sampleCount; ++i)
	{
		int x = pIn[i];
		r = x << 8;
		r = r - 32768;
		pOut[i] = (short)r;
	}
}

void drwav_s24_to_s16(drwav_int16_t *pOut, const drwav_uint8_t *pIn,
                      size_t sampleCount)
{
	int r;
	size_t i;
	for (i = 0; i < sampleCount; ++i)
	{
		int x = ((int)(((unsigned int)(((const unsigned char *)pIn)[i * 3 + 0])
		                << 8) |
		               ((unsigned int)(((const unsigned char *)pIn)[i * 3 + 1])
		                << 16) |
		               ((unsigned int)(((const unsigned char *)pIn)[i * 3 + 2]))
		                   << 24)) >>
		        8;
		r = x >> 8;
		pOut[i] = (short)r;
	}
}

void drwav_s32_to_s16(drwav_int16_t *pOut, const drwav_int32_t *pIn,
                      size_t sampleCount)
{
	int r;
	size_t i;
	for (i = 0; i < sampleCount; ++i)
	{
		int x = pIn[i];
		r = x >> 16;
		pOut[i] = (short)r;
	}
}

void drwav_f32_to_s16(drwav_int16_t *pOut, const float *pIn, size_t sampleCount)
{
	int r;
	size_t i;
	for (i = 0; i < sampleCount; ++i)
	{
		float x = pIn[i];
		float c;
		c = ((x < -1) ? -1 : ((x > 1) ? 1 : x));
		c = c + 1;
		r = (int)(c * 32767.5f);
		r = r - 32768;
		pOut[i] = (short)r;
	}
}

void drwav_f64_to_s16(drwav_int16_t *pOut, const double *pIn,
                      size_t sampleCount)
{
	int r;
	size_t i;
	for (i = 0; i < sampleCount; ++i)
	{
		double x = pIn[i];
		double c;
		c = ((x < -1) ? -1 : ((x > 1) ? 1 : x));
		c = c + 1;
		r = (int)(c * 32767.5);
		r = r - 32768;
		pOut[i] = (short)r;
	}
}

void drwav_alaw_to_s16(drwav_int16_t *pOut, const drwav_uint8_t *pIn,
                       size_t sampleCount)
{
	size_t i;
	for (i = 0; i < sampleCount; ++i) { pOut[i] = drwav__alaw_to_s16(pIn[i]); }
}

void drwav_mulaw_to_s16(drwav_int16_t *pOut, const drwav_uint8_t *pIn,
                        size_t sampleCount)
{
	size_t i;
	for (i = 0; i < sampleCount; ++i) { pOut[i] = drwav__mulaw_to_s16(pIn[i]); }
}

static void drwav__pcm_to_f32(float *pOut, const unsigned char *pIn,
                              size_t sampleCount, unsigned int bytesPerSample)
{
	unsigned int i;

	/* Special case for 8-bit sample data because it's treated as unsigned. */
	if (bytesPerSample == 1)
	{
		drwav_u8_to_f32(pOut, pIn, sampleCount);
		return;
	}

	/* Slightly more optimal implementation for common formats. */
	if (bytesPerSample == 2)
	{
		drwav_s16_to_f32(pOut, (const drwav_int16_t *)pIn, sampleCount);
		return;
	}
	if (bytesPerSample == 3)
	{
		drwav_s24_to_f32(pOut, pIn, sampleCount);
		return;
	}
	if (bytesPerSample == 4)
	{
		drwav_s32_to_f32(pOut, (const drwav_int32_t *)pIn, sampleCount);
		return;
	}

	/* Anything more than 64 bits per sample is not supported. */
	if (bytesPerSample > 8)
	{
		DRWAV_ZERO_MEMORY(pOut, sampleCount * sizeof(*pOut));
		return;
	}

	/* Generic, slow converter. */
	for (i = 0; i < sampleCount; ++i)
	{
		drwav_uint64_t sample = 0;
		unsigned int shift = (8 - bytesPerSample) * 8;

		unsigned int j;
		for (j = 0; j < bytesPerSample && j < 8; j += 1)
		{
			sample |= (drwav_uint64_t)(pIn[j]) << shift;
			shift += 8;
		}

		pIn += j;
		*pOut++ = (float)((drwav_int64_t)sample / 9223372036854775807.0);
	}
}

static void drwav__ieee_to_f32(float *pOut, const unsigned char *pIn,
                               size_t sampleCount, unsigned int bytesPerSample)
{
	if (bytesPerSample == 4)
	{
		unsigned int i;
		for (i = 0; i < sampleCount; ++i) { *pOut++ = ((const float *)pIn)[i]; }
		return;
	}
	else if (bytesPerSample == 8)
	{
		drwav_f64_to_f32(pOut, (const double *)pIn, sampleCount);
		return;
	}
	else
	{
		/* Only supporting 32- and 64-bit float. Output silence in all other
		 * cases. Contributions welcome for 16-bit float. */
		DRWAV_ZERO_MEMORY(pOut, sampleCount * sizeof(*pOut));
		return;
	}
}

drwav_uint64_t drwav_read_pcm_frames_f32__pcm(drwav *pWav,
                                              drwav_uint64_t framesToRead,
                                              float *pBufferOut)
{
	drwav_uint64_t totalFramesRead;
	unsigned char sampleData[4096];

	drwav_uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
	if (bytesPerFrame == 0)
	{
		return 0;
	}

	totalFramesRead = 0;

	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames(
		    pWav, drwav_min(framesToRead, sizeof(sampleData) / bytesPerFrame),
		    sampleData);
		if (framesRead == 0)
		{
			break;
		}

		drwav__pcm_to_f32(pBufferOut, sampleData,
		                  (size_t)framesRead * pWav->channels,
		                  bytesPerFrame / pWav->channels);

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_f32__msadpcm(drwav *pWav,
                                                  drwav_uint64_t framesToRead,
                                                  float *pBufferOut)
{
	/*
	We're just going to borrow the implementation from the drwav_read_s16()
	since ADPCM is a little bit more complicated than other formats and I don't
	want to duplicate that code.
	*/
	drwav_uint64_t totalFramesRead = 0;
	drwav_int16_t samples16[2048];
	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames_s16(
		    pWav,
		    drwav_min(framesToRead, drwav_countof(samples16) / pWav->channels),
		    samples16);
		if (framesRead == 0)
		{
			break;
		}

		drwav_s16_to_f32(
		    pBufferOut, samples16,
		    (size_t)(framesRead *
		             pWav->channels)); /* <-- Safe cast because we're clamping
		                                  to 2048. */

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_f32__ima(drwav *pWav,
                                              drwav_uint64_t framesToRead,
                                              float *pBufferOut)
{
	/*
	We're just going to borrow the implementation from the drwav_read_s16()
	since IMA-ADPCM is a little bit more complicated than other formats and I
	don't want to duplicate that code.
	*/
	drwav_uint64_t totalFramesRead = 0;
	drwav_int16_t samples16[2048];
	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames_s16(
		    pWav,
		    drwav_min(framesToRead, drwav_countof(samples16) / pWav->channels),
		    samples16);
		if (framesRead == 0)
		{
			break;
		}

		drwav_s16_to_f32(
		    pBufferOut, samples16,
		    (size_t)(framesRead *
		             pWav->channels)); /* <-- Safe cast because we're clamping
		                                  to 2048. */

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_f32__ieee(drwav *pWav,
                                               drwav_uint64_t framesToRead,
                                               float *pBufferOut)
{
	drwav_uint64_t totalFramesRead;
	unsigned char sampleData[4096];
	drwav_uint32_t bytesPerFrame;

	/* Fast path. */
	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_IEEE_FLOAT &&
	    pWav->bitsPerSample == 32)
	{
		return drwav_read_pcm_frames(pWav, framesToRead, pBufferOut);
	}

	bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
	if (bytesPerFrame == 0)
	{
		return 0;
	}

	totalFramesRead = 0;

	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames(
		    pWav, drwav_min(framesToRead, sizeof(sampleData) / bytesPerFrame),
		    sampleData);
		if (framesRead == 0)
		{
			break;
		}

		drwav__ieee_to_f32(pBufferOut, sampleData,
		                   (size_t)(framesRead * pWav->channels),
		                   bytesPerFrame / pWav->channels);

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_f32__alaw(drwav *pWav,
                                               drwav_uint64_t framesToRead,
                                               float *pBufferOut)
{
	drwav_uint64_t totalFramesRead;
	unsigned char sampleData[4096];
	drwav_uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
	if (bytesPerFrame == 0)
	{
		return 0;
	}

	totalFramesRead = 0;

	while (bytesPerFrame > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames(
		    pWav, drwav_min(framesToRead, sizeof(sampleData) / bytesPerFrame),
		    sampleData);
		if (framesRead == 0)
		{
			break;
		}

		drwav_alaw_to_f32(pBufferOut, sampleData,
		                  (size_t)(framesRead * pWav->channels));

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_f32__mulaw(drwav *pWav,
                                                drwav_uint64_t framesToRead,
                                                float *pBufferOut)
{
	drwav_uint64_t totalFramesRead;
	unsigned char sampleData[4096];

	drwav_uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
	if (bytesPerFrame == 0)
	{
		return 0;
	}

	totalFramesRead = 0;

	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames(
		    pWav, drwav_min(framesToRead, sizeof(sampleData) / bytesPerFrame),
		    sampleData);
		if (framesRead == 0)
		{
			break;
		}

		drwav_mulaw_to_f32(pBufferOut, sampleData,
		                   (size_t)(framesRead * pWav->channels));

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_f32(drwav *pWav,
                                         drwav_uint64_t framesToRead,
                                         float *pBufferOut)
{
	if (pWav == NULL || framesToRead == 0 || pBufferOut == NULL)
	{
		return 0;
	}

	/* Don't try to read more samples than can potentially fit in the output
	 * buffer. */
	if (framesToRead * pWav->channels * sizeof(float) > DRWAV_SIZE_MAX)
	{
		framesToRead = DRWAV_SIZE_MAX / sizeof(float) / pWav->channels;
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_PCM)
	{
		return drwav_read_pcm_frames_f32__pcm(pWav, framesToRead, pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM)
	{
		return drwav_read_pcm_frames_f32__msadpcm(pWav, framesToRead,
		                                          pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_IEEE_FLOAT)
	{
		return drwav_read_pcm_frames_f32__ieee(pWav, framesToRead, pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ALAW)
	{
		return drwav_read_pcm_frames_f32__alaw(pWav, framesToRead, pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_MULAW)
	{
		return drwav_read_pcm_frames_f32__mulaw(pWav, framesToRead, pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM)
	{
		return drwav_read_pcm_frames_f32__ima(pWav, framesToRead, pBufferOut);
	}

	return 0;
}

drwav_uint64_t drwav_read_pcm_frames_f32le(drwav *pWav,
                                           drwav_uint64_t framesToRead,
                                           float *pBufferOut)
{
	drwav_uint64_t framesRead =
	    drwav_read_pcm_frames_f32(pWav, framesToRead, pBufferOut);
	if (!drwav__is_little_endian())
	{
		drwav__bswap_samples_f32(pBufferOut, framesRead * pWav->channels);
	}

	return framesRead;
}

drwav_uint64_t drwav_read_pcm_frames_f32be(drwav *pWav,
                                           drwav_uint64_t framesToRead,
                                           float *pBufferOut)
{
	drwav_uint64_t framesRead =
	    drwav_read_pcm_frames_f32(pWav, framesToRead, pBufferOut);
	if (drwav__is_little_endian())
	{
		drwav__bswap_samples_f32(pBufferOut, framesRead * pWav->channels);
	}

	return framesRead;
}

void drwav_u8_to_f32(float *pOut, const drwav_uint8_t *pIn, size_t sampleCount)
{
	size_t i;

	if (pOut == NULL || pIn == NULL)
	{
		return;
	}

#ifdef DR_WAV_LIBSNDFILE_COMPAT
	/*
	It appears libsndfile uses slightly different logic for the u8 -> f32
	conversion to dr_wav, which in my opinion is incorrect. It appears
	libsndfile performs the conversion something like "f32 = (u8 / 256) * 2 -
	1", however I think it should be "f32 = (u8 / 255) * 2 - 1" (note the
	divisor of 256 vs 255). I use libsndfile as a benchmark for testing, so I'm
	therefore leaving this block here just for my automated correctness testing.
	This is disabled by default.
	*/
	for (i = 0; i < sampleCount; ++i) { *pOut++ = (pIn[i] / 256.0f) * 2 - 1; }
#else
	for (i = 0; i < sampleCount; ++i) { *pOut++ = (pIn[i] / 255.0f) * 2 - 1; }
#endif
}

void drwav_s16_to_f32(float *pOut, const drwav_int16_t *pIn, size_t sampleCount)
{
	size_t i;

	if (pOut == NULL || pIn == NULL)
	{
		return;
	}

	for (i = 0; i < sampleCount; ++i) { *pOut++ = pIn[i] / 32768.0f; }
}

void drwav_s24_to_f32(float *pOut, const drwav_uint8_t *pIn, size_t sampleCount)
{
	size_t i;

	if (pOut == NULL || pIn == NULL)
	{
		return;
	}

	for (i = 0; i < sampleCount; ++i)
	{
		unsigned int s0 = pIn[i * 3 + 0];
		unsigned int s1 = pIn[i * 3 + 1];
		unsigned int s2 = pIn[i * 3 + 2];

		int sample32 = (int)((s0 << 8) | (s1 << 16) | (s2 << 24));
		*pOut++ = (float)(sample32 / 2147483648.0);
	}
}

void drwav_s32_to_f32(float *pOut, const drwav_int32_t *pIn, size_t sampleCount)
{
	size_t i;
	if (pOut == NULL || pIn == NULL)
	{
		return;
	}

	for (i = 0; i < sampleCount; ++i)
	{
		*pOut++ = (float)(pIn[i] / 2147483648.0);
	}
}

void drwav_f64_to_f32(float *pOut, const double *pIn, size_t sampleCount)
{
	size_t i;

	if (pOut == NULL || pIn == NULL)
	{
		return;
	}

	for (i = 0; i < sampleCount; ++i) { *pOut++ = (float)pIn[i]; }
}

void drwav_alaw_to_f32(float *pOut, const drwav_uint8_t *pIn,
                       size_t sampleCount)
{
	size_t i;

	if (pOut == NULL || pIn == NULL)
	{
		return;
	}

	for (i = 0; i < sampleCount; ++i)
	{
		*pOut++ = drwav__alaw_to_s16(pIn[i]) / 32768.0f;
	}
}

void drwav_mulaw_to_f32(float *pOut, const drwav_uint8_t *pIn,
                        size_t sampleCount)
{
	size_t i;

	if (pOut == NULL || pIn == NULL)
	{
		return;
	}

	for (i = 0; i < sampleCount; ++i)
	{
		*pOut++ = drwav__mulaw_to_s16(pIn[i]) / 32768.0f;
	}
}

static void drwav__pcm_to_s32(drwav_int32_t *pOut, const unsigned char *pIn,
                              size_t totalSampleCount,
                              unsigned int bytesPerSample)
{
	unsigned int i;

	/* Special case for 8-bit sample data because it's treated as unsigned. */
	if (bytesPerSample == 1)
	{
		drwav_u8_to_s32(pOut, pIn, totalSampleCount);
		return;
	}

	/* Slightly more optimal implementation for common formats. */
	if (bytesPerSample == 2)
	{
		drwav_s16_to_s32(pOut, (const drwav_int16_t *)pIn, totalSampleCount);
		return;
	}
	if (bytesPerSample == 3)
	{
		drwav_s24_to_s32(pOut, pIn, totalSampleCount);
		return;
	}
	if (bytesPerSample == 4)
	{
		for (i = 0; i < totalSampleCount; ++i)
		{
			*pOut++ = ((const drwav_int32_t *)pIn)[i];
		}
		return;
	}

	/* Anything more than 64 bits per sample is not supported. */
	if (bytesPerSample > 8)
	{
		DRWAV_ZERO_MEMORY(pOut, totalSampleCount * sizeof(*pOut));
		return;
	}

	/* Generic, slow converter. */
	for (i = 0; i < totalSampleCount; ++i)
	{
		drwav_uint64_t sample = 0;
		unsigned int shift = (8 - bytesPerSample) * 8;

		unsigned int j;
		for (j = 0; j < bytesPerSample && j < 8; j += 1)
		{
			sample |= (drwav_uint64_t)(pIn[j]) << shift;
			shift += 8;
		}

		pIn += j;
		*pOut++ = (drwav_int32_t)((drwav_int64_t)sample >> 32);
	}
}

static void drwav__ieee_to_s32(drwav_int32_t *pOut, const unsigned char *pIn,
                               size_t totalSampleCount,
                               unsigned int bytesPerSample)
{
	if (bytesPerSample == 4)
	{
		drwav_f32_to_s32(pOut, (const float *)pIn, totalSampleCount);
		return;
	}
	else if (bytesPerSample == 8)
	{
		drwav_f64_to_s32(pOut, (const double *)pIn, totalSampleCount);
		return;
	}
	else
	{
		/* Only supporting 32- and 64-bit float. Output silence in all other
		 * cases. Contributions welcome for 16-bit float. */
		DRWAV_ZERO_MEMORY(pOut, totalSampleCount * sizeof(*pOut));
		return;
	}
}

drwav_uint64_t drwav_read_pcm_frames_s32__pcm(drwav *pWav,
                                              drwav_uint64_t framesToRead,
                                              drwav_int32_t *pBufferOut)
{
	drwav_uint64_t totalFramesRead;
	unsigned char sampleData[4096];
	drwav_uint32_t bytesPerFrame;

	/* Fast path. */
	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_PCM &&
	    pWav->bitsPerSample == 32)
	{
		return drwav_read_pcm_frames(pWav, framesToRead, pBufferOut);
	}

	bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
	if (bytesPerFrame == 0)
	{
		return 0;
	}

	totalFramesRead = 0;

	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames(
		    pWav, drwav_min(framesToRead, sizeof(sampleData) / bytesPerFrame),
		    sampleData);
		if (framesRead == 0)
		{
			break;
		}

		drwav__pcm_to_s32(pBufferOut, sampleData,
		                  (size_t)(framesRead * pWav->channels),
		                  bytesPerFrame / pWav->channels);

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_s32__msadpcm(drwav *pWav,
                                                  drwav_uint64_t framesToRead,
                                                  drwav_int32_t *pBufferOut)
{
	/*
	We're just going to borrow the implementation from the drwav_read_s16()
	since ADPCM is a little bit more complicated than other formats and I don't
	want to duplicate that code.
	*/
	drwav_uint64_t totalFramesRead = 0;
	drwav_int16_t samples16[2048];
	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames_s16(
		    pWav,
		    drwav_min(framesToRead, drwav_countof(samples16) / pWav->channels),
		    samples16);
		if (framesRead == 0)
		{
			break;
		}

		drwav_s16_to_s32(
		    pBufferOut, samples16,
		    (size_t)(framesRead *
		             pWav->channels)); /* <-- Safe cast because we're clamping
		                                  to 2048. */

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_s32__ima(drwav *pWav,
                                              drwav_uint64_t framesToRead,
                                              drwav_int32_t *pBufferOut)
{
	/*
	We're just going to borrow the implementation from the drwav_read_s16()
	since IMA-ADPCM is a little bit more complicated than other formats and I
	don't want to duplicate that code.
	*/
	drwav_uint64_t totalFramesRead = 0;
	drwav_int16_t samples16[2048];
	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames_s16(
		    pWav,
		    drwav_min(framesToRead, drwav_countof(samples16) / pWav->channels),
		    samples16);
		if (framesRead == 0)
		{
			break;
		}

		drwav_s16_to_s32(
		    pBufferOut, samples16,
		    (size_t)(framesRead *
		             pWav->channels)); /* <-- Safe cast because we're clamping
		                                  to 2048. */

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_s32__ieee(drwav *pWav,
                                               drwav_uint64_t framesToRead,
                                               drwav_int32_t *pBufferOut)
{
	drwav_uint64_t totalFramesRead;
	unsigned char sampleData[4096];

	drwav_uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
	if (bytesPerFrame == 0)
	{
		return 0;
	}

	totalFramesRead = 0;

	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames(
		    pWav, drwav_min(framesToRead, sizeof(sampleData) / bytesPerFrame),
		    sampleData);
		if (framesRead == 0)
		{
			break;
		}

		drwav__ieee_to_s32(pBufferOut, sampleData,
		                   (size_t)(framesRead * pWav->channels),
		                   bytesPerFrame / pWav->channels);

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_s32__alaw(drwav *pWav,
                                               drwav_uint64_t framesToRead,
                                               drwav_int32_t *pBufferOut)
{
	drwav_uint64_t totalFramesRead;
	unsigned char sampleData[4096];

	drwav_uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
	if (bytesPerFrame == 0)
	{
		return 0;
	}

	totalFramesRead = 0;

	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames(
		    pWav, drwav_min(framesToRead, sizeof(sampleData) / bytesPerFrame),
		    sampleData);
		if (framesRead == 0)
		{
			break;
		}

		drwav_alaw_to_s32(pBufferOut, sampleData,
		                  (size_t)(framesRead * pWav->channels));

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_s32__mulaw(drwav *pWav,
                                                drwav_uint64_t framesToRead,
                                                drwav_int32_t *pBufferOut)
{
	drwav_uint64_t totalFramesRead;
	unsigned char sampleData[4096];

	drwav_uint32_t bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
	if (bytesPerFrame == 0)
	{
		return 0;
	}

	totalFramesRead = 0;

	while (framesToRead > 0)
	{
		drwav_uint64_t framesRead = drwav_read_pcm_frames(
		    pWav, drwav_min(framesToRead, sizeof(sampleData) / bytesPerFrame),
		    sampleData);
		if (framesRead == 0)
		{
			break;
		}

		drwav_mulaw_to_s32(pBufferOut, sampleData,
		                   (size_t)(framesRead * pWav->channels));

		pBufferOut += framesRead * pWav->channels;
		framesToRead -= framesRead;
		totalFramesRead += framesRead;
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_s32(drwav *pWav,
                                         drwav_uint64_t framesToRead,
                                         drwav_int32_t *pBufferOut)
{
	if (pWav == NULL || framesToRead == 0 || pBufferOut == NULL)
	{
		return 0;
	}

	/* Don't try to read more samples than can potentially fit in the output
	 * buffer. */
	if (framesToRead * pWav->channels * sizeof(drwav_int32_t) > DRWAV_SIZE_MAX)
	{
		framesToRead = DRWAV_SIZE_MAX / sizeof(drwav_int32_t) / pWav->channels;
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_PCM)
	{
		return drwav_read_pcm_frames_s32__pcm(pWav, framesToRead, pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM)
	{
		return drwav_read_pcm_frames_s32__msadpcm(pWav, framesToRead,
		                                          pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_IEEE_FLOAT)
	{
		return drwav_read_pcm_frames_s32__ieee(pWav, framesToRead, pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ALAW)
	{
		return drwav_read_pcm_frames_s32__alaw(pWav, framesToRead, pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_MULAW)
	{
		return drwav_read_pcm_frames_s32__mulaw(pWav, framesToRead, pBufferOut);
	}

	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM)
	{
		return drwav_read_pcm_frames_s32__ima(pWav, framesToRead, pBufferOut);
	}

	return 0;
}

drwav_uint64_t drwav_read_pcm_frames_s32le(drwav *pWav,
                                           drwav_uint64_t framesToRead,
                                           drwav_int32_t *pBufferOut)
{
	drwav_uint64_t framesRead =
	    drwav_read_pcm_frames_s32(pWav, framesToRead, pBufferOut);
	if (!drwav__is_little_endian())
	{
		drwav__bswap_samples_s32(pBufferOut, framesRead * pWav->channels);
	}

	return framesRead;
}

drwav_uint64_t drwav_read_pcm_frames_s32be(drwav *pWav,
                                           drwav_uint64_t framesToRead,
                                           drwav_int32_t *pBufferOut)
{
	drwav_uint64_t framesRead =
	    drwav_read_pcm_frames_s32(pWav, framesToRead, pBufferOut);
	if (drwav__is_little_endian())
	{
		drwav__bswap_samples_s32(pBufferOut, framesRead * pWav->channels);
	}

	return framesRead;
}

void drwav_u8_to_s32(drwav_int32_t *pOut, const drwav_uint8_t *pIn,
                     size_t sampleCount)
{
	size_t i;

	if (pOut == NULL || pIn == NULL)
	{
		return;
	}

	for (i = 0; i < sampleCount; ++i) { *pOut++ = ((int)pIn[i] - 128) << 24; }
}

void drwav_s16_to_s32(drwav_int32_t *pOut, const drwav_int16_t *pIn,
                      size_t sampleCount)
{
	size_t i;

	if (pOut == NULL || pIn == NULL)
	{
		return;
	}

	for (i = 0; i < sampleCount; ++i) { *pOut++ = pIn[i] << 16; }
}

void drwav_s24_to_s32(drwav_int32_t *pOut, const drwav_uint8_t *pIn,
                      size_t sampleCount)
{
	size_t i;

	if (pOut == NULL || pIn == NULL)
	{
		return;
	}

	for (i = 0; i < sampleCount; ++i)
	{
		unsigned int s0 = pIn[i * 3 + 0];
		unsigned int s1 = pIn[i * 3 + 1];
		unsigned int s2 = pIn[i * 3 + 2];

		drwav_int32_t sample32 =
		    (drwav_int32_t)((s0 << 8) | (s1 << 16) | (s2 << 24));
		*pOut++ = sample32;
	}
}

void drwav_f32_to_s32(drwav_int32_t *pOut, const float *pIn, size_t sampleCount)
{
	size_t i;

	if (pOut == NULL || pIn == NULL)
	{
		return;
	}

	for (i = 0; i < sampleCount; ++i)
	{
		*pOut++ = (drwav_int32_t)(2147483648.0 * pIn[i]);
	}
}

void drwav_f64_to_s32(drwav_int32_t *pOut, const double *pIn,
                      size_t sampleCount)
{
	size_t i;

	if (pOut == NULL || pIn == NULL)
	{
		return;
	}

	for (i = 0; i < sampleCount; ++i)
	{
		*pOut++ = (drwav_int32_t)(2147483648.0 * pIn[i]);
	}
}

void drwav_alaw_to_s32(drwav_int32_t *pOut, const drwav_uint8_t *pIn,
                       size_t sampleCount)
{
	size_t i;

	if (pOut == NULL || pIn == NULL)
	{
		return;
	}

	for (i = 0; i < sampleCount; ++i)
	{
		*pOut++ = ((drwav_int32_t)drwav__alaw_to_s16(pIn[i])) << 16;
	}
}

void drwav_mulaw_to_s32(drwav_int32_t *pOut, const drwav_uint8_t *pIn,
                        size_t sampleCount)
{
	size_t i;

	if (pOut == NULL || pIn == NULL)
	{
		return;
	}

	for (i = 0; i < sampleCount; ++i)
	{
		*pOut++ = ((drwav_int32_t)drwav__mulaw_to_s16(pIn[i])) << 16;
	}
}

drwav_int16_t *
drwav__read_pcm_frames_and_close_s16(drwav *pWav, unsigned int *channels,
                                     unsigned int *sampleRate,
                                     drwav_uint64_t *totalFrameCount)
{
	drwav_uint64_t sampleDataSize;
	drwav_int16_t *pSampleData;
	drwav_uint64_t framesRead;

	DRWAV_ASSERT(pWav != NULL);

	sampleDataSize =
	    pWav->totalPCMFrameCount * pWav->channels * sizeof(drwav_int16_t);
	if (sampleDataSize > DRWAV_SIZE_MAX)
	{
		drwav_uninit(pWav);
		return NULL; /* File's too big. */
	}

	pSampleData = (drwav_int16_t *)drwav__malloc_from_callbacks(
	    (size_t)sampleDataSize,
	    &pWav->allocationCallbacks); /* <-- Safe cast due to the check above. */
	if (pSampleData == NULL)
	{
		drwav_uninit(pWav);
		return NULL; /* Failed to allocate memory. */
	}

	framesRead = drwav_read_pcm_frames_s16(
	    pWav, (size_t)pWav->totalPCMFrameCount, pSampleData);
	if (framesRead != pWav->totalPCMFrameCount)
	{
		drwav__free_from_callbacks(pSampleData, &pWav->allocationCallbacks);
		drwav_uninit(pWav);
		return NULL; /* There was an error reading the samples. */
	}

	drwav_uninit(pWav);

	if (sampleRate)
	{
		*sampleRate = pWav->sampleRate;
	}
	if (channels)
	{
		*channels = pWav->channels;
	}
	if (totalFrameCount)
	{
		*totalFrameCount = pWav->totalPCMFrameCount;
	}

	return pSampleData;
}

float *drwav__read_pcm_frames_and_close_f32(drwav *pWav, unsigned int *channels,
                                            unsigned int *sampleRate,
                                            drwav_uint64_t *totalFrameCount)
{
	drwav_uint64_t sampleDataSize;
	float *pSampleData;
	drwav_uint64_t framesRead;

	DRWAV_ASSERT(pWav != NULL);

	sampleDataSize = pWav->totalPCMFrameCount * pWav->channels * sizeof(float);
	if (sampleDataSize > DRWAV_SIZE_MAX)
	{
		drwav_uninit(pWav);
		return NULL; /* File's too big. */
	}

	pSampleData = (float *)drwav__malloc_from_callbacks(
	    (size_t)sampleDataSize,
	    &pWav->allocationCallbacks); /* <-- Safe cast due to the check above. */
	if (pSampleData == NULL)
	{
		drwav_uninit(pWav);
		return NULL; /* Failed to allocate memory. */
	}

	framesRead = drwav_read_pcm_frames_f32(
	    pWav, (size_t)pWav->totalPCMFrameCount, pSampleData);
	if (framesRead != pWav->totalPCMFrameCount)
	{
		drwav__free_from_callbacks(pSampleData, &pWav->allocationCallbacks);
		drwav_uninit(pWav);
		return NULL; /* There was an error reading the samples. */
	}

	drwav_uninit(pWav);

	if (sampleRate)
	{
		*sampleRate = pWav->sampleRate;
	}
	if (channels)
	{
		*channels = pWav->channels;
	}
	if (totalFrameCount)
	{
		*totalFrameCount = pWav->totalPCMFrameCount;
	}

	return pSampleData;
}

drwav_int32_t *
drwav__read_pcm_frames_and_close_s32(drwav *pWav, unsigned int *channels,
                                     unsigned int *sampleRate,
                                     drwav_uint64_t *totalFrameCount)
{
	drwav_uint64_t sampleDataSize;
	drwav_int32_t *pSampleData;
	drwav_uint64_t framesRead;

	DRWAV_ASSERT(pWav != NULL);

	sampleDataSize =
	    pWav->totalPCMFrameCount * pWav->channels * sizeof(drwav_int32_t);
	if (sampleDataSize > DRWAV_SIZE_MAX)
	{
		drwav_uninit(pWav);
		return NULL; /* File's too big. */
	}

	pSampleData = (drwav_int32_t *)drwav__malloc_from_callbacks(
	    (size_t)sampleDataSize,
	    &pWav->allocationCallbacks); /* <-- Safe cast due to the check above. */
	if (pSampleData == NULL)
	{
		drwav_uninit(pWav);
		return NULL; /* Failed to allocate memory. */
	}

	framesRead = drwav_read_pcm_frames_s32(
	    pWav, (size_t)pWav->totalPCMFrameCount, pSampleData);
	if (framesRead != pWav->totalPCMFrameCount)
	{
		drwav__free_from_callbacks(pSampleData, &pWav->allocationCallbacks);
		drwav_uninit(pWav);
		return NULL; /* There was an error reading the samples. */
	}

	drwav_uninit(pWav);

	if (sampleRate)
	{
		*sampleRate = pWav->sampleRate;
	}
	if (channels)
	{
		*channels = pWav->channels;
	}
	if (totalFrameCount)
	{
		*totalFrameCount = pWav->totalPCMFrameCount;
	}

	return pSampleData;
}

drwav_int16_t *drwav_open_and_read_pcm_frames_s16(
    drwav_read_proc onRead, drwav_seek_proc onSeek, void *pUserData,
    unsigned int *channelsOut, unsigned int *sampleRateOut,
    drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	drwav wav;

	if (channelsOut)
	{
		*channelsOut = 0;
	}
	if (sampleRateOut)
	{
		*sampleRateOut = 0;
	}
	if (totalFrameCountOut)
	{
		*totalFrameCountOut = 0;
	}

	if (!drwav_init(&wav, onRead, onSeek, pUserData, pAllocationCallbacks))
	{
		return NULL;
	}

	return drwav__read_pcm_frames_and_close_s16(
	    &wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

float *drwav_open_and_read_pcm_frames_f32(
    drwav_read_proc onRead, drwav_seek_proc onSeek, void *pUserData,
    unsigned int *channelsOut, unsigned int *sampleRateOut,
    drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	drwav wav;

	if (channelsOut)
	{
		*channelsOut = 0;
	}
	if (sampleRateOut)
	{
		*sampleRateOut = 0;
	}
	if (totalFrameCountOut)
	{
		*totalFrameCountOut = 0;
	}

	if (!drwav_init(&wav, onRead, onSeek, pUserData, pAllocationCallbacks))
	{
		return NULL;
	}

	return drwav__read_pcm_frames_and_close_f32(
	    &wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

drwav_int32_t *drwav_open_and_read_pcm_frames_s32(
    drwav_read_proc onRead, drwav_seek_proc onSeek, void *pUserData,
    unsigned int *channelsOut, unsigned int *sampleRateOut,
    drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	drwav wav;

	if (channelsOut)
	{
		*channelsOut = 0;
	}
	if (sampleRateOut)
	{
		*sampleRateOut = 0;
	}
	if (totalFrameCountOut)
	{
		*totalFrameCountOut = 0;
	}

	if (!drwav_init(&wav, onRead, onSeek, pUserData, pAllocationCallbacks))
	{
		return NULL;
	}

	return drwav__read_pcm_frames_and_close_s32(
	    &wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

#ifndef DR_WAV_NO_STDIO
drwav_int16_t *drwav_open_file_and_read_pcm_frames_s16(
    const char *filename, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	drwav wav;

	if (channelsOut)
	{
		*channelsOut = 0;
	}
	if (sampleRateOut)
	{
		*sampleRateOut = 0;
	}
	if (totalFrameCountOut)
	{
		*totalFrameCountOut = 0;
	}

	if (!drwav_init_file(&wav, filename, pAllocationCallbacks))
	{
		return NULL;
	}

	return drwav__read_pcm_frames_and_close_s16(
	    &wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

float *drwav_open_file_and_read_pcm_frames_f32(
    const char *filename, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	drwav wav;

	if (channelsOut)
	{
		*channelsOut = 0;
	}
	if (sampleRateOut)
	{
		*sampleRateOut = 0;
	}
	if (totalFrameCountOut)
	{
		*totalFrameCountOut = 0;
	}

	if (!drwav_init_file(&wav, filename, pAllocationCallbacks))
	{
		return NULL;
	}

	return drwav__read_pcm_frames_and_close_f32(
	    &wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

drwav_int32_t *drwav_open_file_and_read_pcm_frames_s32(
    const char *filename, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	drwav wav;

	if (channelsOut)
	{
		*channelsOut = 0;
	}
	if (sampleRateOut)
	{
		*sampleRateOut = 0;
	}
	if (totalFrameCountOut)
	{
		*totalFrameCountOut = 0;
	}

	if (!drwav_init_file(&wav, filename, pAllocationCallbacks))
	{
		return NULL;
	}

	return drwav__read_pcm_frames_and_close_s32(
	    &wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

drwav_int16_t *drwav_open_file_and_read_pcm_frames_s16_w(
    const wchar_t *filename, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	drwav wav;

	if (sampleRateOut)
	{
		*sampleRateOut = 0;
	}
	if (channelsOut)
	{
		*channelsOut = 0;
	}
	if (totalFrameCountOut)
	{
		*totalFrameCountOut = 0;
	}

	if (!drwav_init_file_w(&wav, filename, pAllocationCallbacks))
	{
		return NULL;
	}

	return drwav__read_pcm_frames_and_close_s16(
	    &wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

float *drwav_open_file_and_read_pcm_frames_f32_w(
    const wchar_t *filename, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	drwav wav;

	if (sampleRateOut)
	{
		*sampleRateOut = 0;
	}
	if (channelsOut)
	{
		*channelsOut = 0;
	}
	if (totalFrameCountOut)
	{
		*totalFrameCountOut = 0;
	}

	if (!drwav_init_file_w(&wav, filename, pAllocationCallbacks))
	{
		return NULL;
	}

	return drwav__read_pcm_frames_and_close_f32(
	    &wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

drwav_int32_t *drwav_open_file_and_read_pcm_frames_s32_w(
    const wchar_t *filename, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	drwav wav;

	if (sampleRateOut)
	{
		*sampleRateOut = 0;
	}
	if (channelsOut)
	{
		*channelsOut = 0;
	}
	if (totalFrameCountOut)
	{
		*totalFrameCountOut = 0;
	}

	if (!drwav_init_file_w(&wav, filename, pAllocationCallbacks))
	{
		return NULL;
	}

	return drwav__read_pcm_frames_and_close_s32(
	    &wav, channelsOut, sampleRateOut, totalFrameCountOut);
}
#endif

drwav_int16_t *drwav_open_memory_and_read_pcm_frames_s16(
    const void *data, size_t dataSize, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	drwav wav;

	if (channelsOut)
	{
		*channelsOut = 0;
	}
	if (sampleRateOut)
	{
		*sampleRateOut = 0;
	}
	if (totalFrameCountOut)
	{
		*totalFrameCountOut = 0;
	}

	if (!drwav_init_memory(&wav, data, dataSize, pAllocationCallbacks))
	{
		return NULL;
	}

	return drwav__read_pcm_frames_and_close_s16(
	    &wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

float *drwav_open_memory_and_read_pcm_frames_f32(
    const void *data, size_t dataSize, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	drwav wav;

	if (channelsOut)
	{
		*channelsOut = 0;
	}
	if (sampleRateOut)
	{
		*sampleRateOut = 0;
	}
	if (totalFrameCountOut)
	{
		*totalFrameCountOut = 0;
	}

	if (!drwav_init_memory(&wav, data, dataSize, pAllocationCallbacks))
	{
		return NULL;
	}

	return drwav__read_pcm_frames_and_close_f32(
	    &wav, channelsOut, sampleRateOut, totalFrameCountOut);
}

drwav_int32_t *drwav_open_memory_and_read_pcm_frames_s32(
    const void *data, size_t dataSize, unsigned int *channelsOut,
    unsigned int *sampleRateOut, drwav_uint64_t *totalFrameCountOut,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	drwav wav;

	if (channelsOut)
	{
		*channelsOut = 0;
	}
	if (sampleRateOut)
	{
		*sampleRateOut = 0;
	}
	if (totalFrameCountOut)
	{
		*totalFrameCountOut = 0;
	}

	if (!drwav_init_memory(&wav, data, dataSize, pAllocationCallbacks))
	{
		return NULL;
	}

	return drwav__read_pcm_frames_and_close_s32(
	    &wav, channelsOut, sampleRateOut, totalFrameCountOut);
}
#endif /* DR_WAV_NO_CONVERSION_API */
