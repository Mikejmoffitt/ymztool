#include "3rdparty/dr_wav/dr_wav_pcm.h"
#include "3rdparty/dr_wav/dr_wav_init.h"
#include "3rdparty/dr_wav/dr_wav_util.h"

drwav_uint64_t drwav_read_pcm_frames_s16__msadpcm(drwav *pWav,
                                                  drwav_uint64_t framesToRead,
                                                  drwav_int16_t *pBufferOut)
{
	drwav_uint64_t totalFramesRead = 0;

	DRWAV_ASSERT(pWav != NULL);
	DRWAV_ASSERT(framesToRead > 0);
	DRWAV_ASSERT(pBufferOut != NULL);

	/* TODO: Lots of room for optimization here. */

	while (framesToRead > 0 &&
	       pWav->compressed.iCurrentPCMFrame < pWav->totalPCMFrameCount)
	{
		/* If there are no cached frames we need to load a new block. */
		if (pWav->msadpcm.cachedFrameCount == 0 &&
		    pWav->msadpcm.bytesRemainingInBlock == 0)
		{
			if (pWav->channels == 1)
			{
				/* Mono. */
				drwav_uint8_t header[7];
				if (pWav->onRead(pWav->pUserData, header, sizeof(header)) !=
				    sizeof(header))
				{
					return totalFramesRead;
				}
				pWav->msadpcm.bytesRemainingInBlock =
				    pWav->fmt.blockAlign - sizeof(header);

				pWav->msadpcm.predictor[0] = header[0];
				pWav->msadpcm.delta[0] = drwav__bytes_to_s16(header + 1);
				pWav->msadpcm.prevFrames[0][1] =
				    (drwav_int32_t)drwav__bytes_to_s16(header + 3);
				pWav->msadpcm.prevFrames[0][0] =
				    (drwav_int32_t)drwav__bytes_to_s16(header + 5);
				pWav->msadpcm.cachedFrames[2] = pWav->msadpcm.prevFrames[0][0];
				pWav->msadpcm.cachedFrames[3] = pWav->msadpcm.prevFrames[0][1];
				pWav->msadpcm.cachedFrameCount = 2;
			}
			else
			{
				/* Stereo. */
				drwav_uint8_t header[14];
				if (pWav->onRead(pWav->pUserData, header, sizeof(header)) !=
				    sizeof(header))
				{
					return totalFramesRead;
				}
				pWav->msadpcm.bytesRemainingInBlock =
				    pWav->fmt.blockAlign - sizeof(header);

				pWav->msadpcm.predictor[0] = header[0];
				pWav->msadpcm.predictor[1] = header[1];
				pWav->msadpcm.delta[0] = drwav__bytes_to_s16(header + 2);
				pWav->msadpcm.delta[1] = drwav__bytes_to_s16(header + 4);
				pWav->msadpcm.prevFrames[0][1] =
				    (drwav_int32_t)drwav__bytes_to_s16(header + 6);
				pWav->msadpcm.prevFrames[1][1] =
				    (drwav_int32_t)drwav__bytes_to_s16(header + 8);
				pWav->msadpcm.prevFrames[0][0] =
				    (drwav_int32_t)drwav__bytes_to_s16(header + 10);
				pWav->msadpcm.prevFrames[1][0] =
				    (drwav_int32_t)drwav__bytes_to_s16(header + 12);

				pWav->msadpcm.cachedFrames[0] = pWav->msadpcm.prevFrames[0][0];
				pWav->msadpcm.cachedFrames[1] = pWav->msadpcm.prevFrames[1][0];
				pWav->msadpcm.cachedFrames[2] = pWav->msadpcm.prevFrames[0][1];
				pWav->msadpcm.cachedFrames[3] = pWav->msadpcm.prevFrames[1][1];
				pWav->msadpcm.cachedFrameCount = 2;
			}
		}

		/* Output anything that's cached. */
		while (framesToRead > 0 && pWav->msadpcm.cachedFrameCount > 0 &&
		       pWav->compressed.iCurrentPCMFrame < pWav->totalPCMFrameCount)
		{
			drwav_uint32_t iSample = 0;
			for (iSample = 0; iSample < pWav->channels; iSample += 1)
			{
				pBufferOut[iSample] =
				    (drwav_int16_t)pWav->msadpcm.cachedFrames
				        [(drwav_countof(pWav->msadpcm.cachedFrames) -
				          (pWav->msadpcm.cachedFrameCount * pWav->channels)) +
				         iSample];
			}

			pBufferOut += pWav->channels;
			framesToRead -= 1;
			totalFramesRead += 1;
			pWav->compressed.iCurrentPCMFrame += 1;
			pWav->msadpcm.cachedFrameCount -= 1;
		}

		if (framesToRead == 0)
		{
			return totalFramesRead;
		}

		/*
		If there's nothing left in the cache, just go ahead and load more. If
		there's nothing left to load in the current block we just continue to
		the next loop iteration which will trigger the loading of a new block.
		*/
		if (pWav->msadpcm.cachedFrameCount == 0)
		{
			if (pWav->msadpcm.bytesRemainingInBlock == 0)
			{
				continue;
			}
			else
			{
				static drwav_int32_t adaptationTable[] = {
				    230, 230, 230, 230, 307, 409, 512, 614,
				    768, 614, 512, 409, 307, 230, 230, 230};
				static drwav_int32_t coeff1Table[] = {256, 512, 0,  192,
				                                      240, 460, 392};
				static drwav_int32_t coeff2Table[] = {0, -256, 0,   64,
				                                      0, -208, -232};

				drwav_uint8_t nibbles;
				drwav_int32_t nibble0;
				drwav_int32_t nibble1;

				if (pWav->onRead(pWav->pUserData, &nibbles, 1) != 1)
				{
					return totalFramesRead;
				}
				pWav->msadpcm.bytesRemainingInBlock -= 1;

				/* TODO: Optimize away these if statements. */
				nibble0 = ((nibbles & 0xF0) >> 4);
				if ((nibbles & 0x80))
				{
					nibble0 |= 0xFFFFFFF0UL;
				}
				nibble1 = ((nibbles & 0x0F) >> 0);
				if ((nibbles & 0x08))
				{
					nibble1 |= 0xFFFFFFF0UL;
				}

				if (pWav->channels == 1)
				{
					/* Mono. */
					drwav_int32_t newSample0;
					drwav_int32_t newSample1;

					newSample0 = ((pWav->msadpcm.prevFrames[0][1] *
					               coeff1Table[pWav->msadpcm.predictor[0]]) +
					              (pWav->msadpcm.prevFrames[0][0] *
					               coeff2Table[pWav->msadpcm.predictor[0]])) >>
					             8;
					newSample0 += nibble0 * pWav->msadpcm.delta[0];
					newSample0 = drwav_clamp(newSample0, -32768, 32767);

					pWav->msadpcm.delta[0] =
					    (adaptationTable[((nibbles & 0xF0) >> 4)] *
					     pWav->msadpcm.delta[0]) >>
					    8;
					if (pWav->msadpcm.delta[0] < 16)
					{
						pWav->msadpcm.delta[0] = 16;
					}

					pWav->msadpcm.prevFrames[0][0] =
					    pWav->msadpcm.prevFrames[0][1];
					pWav->msadpcm.prevFrames[0][1] = newSample0;

					newSample1 = ((pWav->msadpcm.prevFrames[0][1] *
					               coeff1Table[pWav->msadpcm.predictor[0]]) +
					              (pWav->msadpcm.prevFrames[0][0] *
					               coeff2Table[pWav->msadpcm.predictor[0]])) >>
					             8;
					newSample1 += nibble1 * pWav->msadpcm.delta[0];
					newSample1 = drwav_clamp(newSample1, -32768, 32767);

					pWav->msadpcm.delta[0] =
					    (adaptationTable[((nibbles & 0x0F) >> 0)] *
					     pWav->msadpcm.delta[0]) >>
					    8;
					if (pWav->msadpcm.delta[0] < 16)
					{
						pWav->msadpcm.delta[0] = 16;
					}

					pWav->msadpcm.prevFrames[0][0] =
					    pWav->msadpcm.prevFrames[0][1];
					pWav->msadpcm.prevFrames[0][1] = newSample1;

					pWav->msadpcm.cachedFrames[2] = newSample0;
					pWav->msadpcm.cachedFrames[3] = newSample1;
					pWav->msadpcm.cachedFrameCount = 2;
				}
				else
				{
					/* Stereo. */
					drwav_int32_t newSample0;
					drwav_int32_t newSample1;

					/* Left. */
					newSample0 = ((pWav->msadpcm.prevFrames[0][1] *
					               coeff1Table[pWav->msadpcm.predictor[0]]) +
					              (pWav->msadpcm.prevFrames[0][0] *
					               coeff2Table[pWav->msadpcm.predictor[0]])) >>
					             8;
					newSample0 += nibble0 * pWav->msadpcm.delta[0];
					newSample0 = drwav_clamp(newSample0, -32768, 32767);

					pWav->msadpcm.delta[0] =
					    (adaptationTable[((nibbles & 0xF0) >> 4)] *
					     pWav->msadpcm.delta[0]) >>
					    8;
					if (pWav->msadpcm.delta[0] < 16)
					{
						pWav->msadpcm.delta[0] = 16;
					}

					pWav->msadpcm.prevFrames[0][0] =
					    pWav->msadpcm.prevFrames[0][1];
					pWav->msadpcm.prevFrames[0][1] = newSample0;

					/* Right. */
					newSample1 = ((pWav->msadpcm.prevFrames[1][1] *
					               coeff1Table[pWav->msadpcm.predictor[1]]) +
					              (pWav->msadpcm.prevFrames[1][0] *
					               coeff2Table[pWav->msadpcm.predictor[1]])) >>
					             8;
					newSample1 += nibble1 * pWav->msadpcm.delta[1];
					newSample1 = drwav_clamp(newSample1, -32768, 32767);

					pWav->msadpcm.delta[1] =
					    (adaptationTable[((nibbles & 0x0F) >> 0)] *
					     pWav->msadpcm.delta[1]) >>
					    8;
					if (pWav->msadpcm.delta[1] < 16)
					{
						pWav->msadpcm.delta[1] = 16;
					}

					pWav->msadpcm.prevFrames[1][0] =
					    pWav->msadpcm.prevFrames[1][1];
					pWav->msadpcm.prevFrames[1][1] = newSample1;

					pWav->msadpcm.cachedFrames[2] = newSample0;
					pWav->msadpcm.cachedFrames[3] = newSample1;
					pWav->msadpcm.cachedFrameCount = 1;
				}
			}
		}
	}

	return totalFramesRead;
}

drwav_uint64_t drwav_read_pcm_frames_s16__ima(drwav *pWav,
                                              drwav_uint64_t framesToRead,
                                              drwav_int16_t *pBufferOut)
{
	drwav_uint64_t totalFramesRead = 0;

	DRWAV_ASSERT(pWav != NULL);
	DRWAV_ASSERT(framesToRead > 0);
	DRWAV_ASSERT(pBufferOut != NULL);

	/* TODO: Lots of room for optimization here. */

	while (framesToRead > 0 &&
	       pWav->compressed.iCurrentPCMFrame < pWav->totalPCMFrameCount)
	{
		/* If there are no cached samples we need to load a new block. */
		if (pWav->ima.cachedFrameCount == 0 &&
		    pWav->ima.bytesRemainingInBlock == 0)
		{
			if (pWav->channels == 1)
			{
				/* Mono. */
				drwav_uint8_t header[4];
				if (pWav->onRead(pWav->pUserData, header, sizeof(header)) !=
				    sizeof(header))
				{
					return totalFramesRead;
				}
				pWav->ima.bytesRemainingInBlock =
				    pWav->fmt.blockAlign - sizeof(header);

				pWav->ima.predictor[0] = drwav__bytes_to_s16(header + 0);
				pWav->ima.stepIndex[0] = header[2];
				pWav->ima
				    .cachedFrames[drwav_countof(pWav->ima.cachedFrames) - 1] =
				    pWav->ima.predictor[0];
				pWav->ima.cachedFrameCount = 1;
			}
			else
			{
				/* Stereo. */
				drwav_uint8_t header[8];
				if (pWav->onRead(pWav->pUserData, header, sizeof(header)) !=
				    sizeof(header))
				{
					return totalFramesRead;
				}
				pWav->ima.bytesRemainingInBlock =
				    pWav->fmt.blockAlign - sizeof(header);

				pWav->ima.predictor[0] = drwav__bytes_to_s16(header + 0);
				pWav->ima.stepIndex[0] = header[2];
				pWav->ima.predictor[1] = drwav__bytes_to_s16(header + 4);
				pWav->ima.stepIndex[1] = header[6];

				pWav->ima
				    .cachedFrames[drwav_countof(pWav->ima.cachedFrames) - 2] =
				    pWav->ima.predictor[0];
				pWav->ima
				    .cachedFrames[drwav_countof(pWav->ima.cachedFrames) - 1] =
				    pWav->ima.predictor[1];
				pWav->ima.cachedFrameCount = 1;
			}
		}

		/* Output anything that's cached. */
		while (framesToRead > 0 && pWav->ima.cachedFrameCount > 0 &&
		       pWav->compressed.iCurrentPCMFrame < pWav->totalPCMFrameCount)
		{
			drwav_uint32_t iSample;
			for (iSample = 0; iSample < pWav->channels; iSample += 1)
			{
				pBufferOut[iSample] =
				    (drwav_int16_t)pWav->ima
				        .cachedFrames[(drwav_countof(pWav->ima.cachedFrames) -
				                       (pWav->ima.cachedFrameCount *
				                        pWav->channels)) +
				                      iSample];
			}

			pBufferOut += pWav->channels;
			framesToRead -= 1;
			totalFramesRead += 1;
			pWav->compressed.iCurrentPCMFrame += 1;
			pWav->ima.cachedFrameCount -= 1;
		}

		if (framesToRead == 0)
		{
			return totalFramesRead;
		}

		/*
		If there's nothing left in the cache, just go ahead and load more. If
		there's nothing left to load in the current block we just continue to
		the next loop iteration which will trigger the loading of a new block.
		*/
		if (pWav->ima.cachedFrameCount == 0)
		{
			if (pWav->ima.bytesRemainingInBlock == 0)
			{
				continue;
			}
			else
			{
				static drwav_int32_t indexTable[16] = {
				    -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

				static drwav_int32_t stepTable[89] = {
				    7,     8,     9,     10,    11,    12,    13,    14,
				    16,    17,    19,    21,    23,    25,    28,    31,
				    34,    37,    41,    45,    50,    55,    60,    66,
				    73,    80,    88,    97,    107,   118,   130,   143,
				    157,   173,   190,   209,   230,   253,   279,   307,
				    337,   371,   408,   449,   494,   544,   598,   658,
				    724,   796,   876,   963,   1060,  1166,  1282,  1411,
				    1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
				    3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
				    7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
				    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
				    32767};

				drwav_uint32_t iChannel;

				/*
				From what I can tell with stereo streams, it looks like every 4
				bytes (8 samples) is for one channel. So it goes 4 bytes for the
				left channel, 4 bytes for the right channel.
				*/
				pWav->ima.cachedFrameCount = 8;
				for (iChannel = 0; iChannel < pWav->channels; ++iChannel)
				{
					drwav_uint32_t iByte;
					drwav_uint8_t nibbles[4];
					if (pWav->onRead(pWav->pUserData, &nibbles, 4) != 4)
					{
						return totalFramesRead;
					}
					pWav->ima.bytesRemainingInBlock -= 4;

					for (iByte = 0; iByte < 4; ++iByte)
					{
						drwav_uint8_t nibble0 = ((nibbles[iByte] & 0x0F) >> 0);
						drwav_uint8_t nibble1 = ((nibbles[iByte] & 0xF0) >> 4);

						drwav_int32_t step =
						    stepTable[pWav->ima.stepIndex[iChannel]];
						drwav_int32_t predictor = pWav->ima.predictor[iChannel];

						drwav_int32_t diff = step >> 3;
						if (nibble0 & 1)
							diff += step >> 2;
						if (nibble0 & 2)
							diff += step >> 1;
						if (nibble0 & 4)
							diff += step;
						if (nibble0 & 8)
							diff = -diff;

						predictor =
						    drwav_clamp(predictor + diff, -32768, 32767);
						pWav->ima.predictor[iChannel] = predictor;
						pWav->ima.stepIndex[iChannel] = drwav_clamp(
						    pWav->ima.stepIndex[iChannel] + indexTable[nibble0],
						    0, (drwav_int32_t)drwav_countof(stepTable) - 1);
						pWav->ima.cachedFrames
						    [(drwav_countof(pWav->ima.cachedFrames) -
						      (pWav->ima.cachedFrameCount * pWav->channels)) +
						     (iByte * 2 + 0) * pWav->channels + iChannel] =
						    predictor;

						step = stepTable[pWav->ima.stepIndex[iChannel]];
						predictor = pWav->ima.predictor[iChannel];

						diff = step >> 3;
						if (nibble1 & 1)
							diff += step >> 2;
						if (nibble1 & 2)
							diff += step >> 1;
						if (nibble1 & 4)
							diff += step;
						if (nibble1 & 8)
							diff = -diff;

						predictor =
						    drwav_clamp(predictor + diff, -32768, 32767);
						pWav->ima.predictor[iChannel] = predictor;
						pWav->ima.stepIndex[iChannel] = drwav_clamp(
						    pWav->ima.stepIndex[iChannel] + indexTable[nibble1],
						    0, (drwav_int32_t)drwav_countof(stepTable) - 1);
						pWav->ima.cachedFrames
						    [(drwav_countof(pWav->ima.cachedFrames) -
						      (pWav->ima.cachedFrameCount * pWav->channels)) +
						     (iByte * 2 + 1) * pWav->channels + iChannel] =
						    predictor;
					}
				}
			}
		}
	}

	return totalFramesRead;
}

static DRWAV_INLINE drwav_bool32
drwav__is_compressed_format_tag(drwav_uint16_t formatTag)
{
	return formatTag == DR_WAVE_FORMAT_ADPCM ||
	       formatTag == DR_WAVE_FORMAT_DVI_ADPCM;
}

drwav_uint64_t drwav_read_pcm_frames_s16__msadpcm(drwav *pWav,
                                                  drwav_uint64_t samplesToRead,
                                                  drwav_int16_t *pBufferOut);
drwav_uint64_t drwav_read_pcm_frames_s16__ima(drwav *pWav,
                                              drwav_uint64_t samplesToRead,
                                              drwav_int16_t *pBufferOut);
drwav_uint64_t drwav_read_pcm_frames_le(drwav *pWav,
                                        drwav_uint64_t framesToRead,
                                        void *pBufferOut)
{
	drwav_uint32_t bytesPerFrame;

	if (pWav == NULL || framesToRead == 0 || pBufferOut == NULL)
	{
		return 0;
	}

	/* Cannot use this function for compressed formats. */
	if (drwav__is_compressed_format_tag(pWav->translatedFormatTag))
	{
		return 0;
	}

	bytesPerFrame = drwav_get_bytes_per_pcm_frame(pWav);
	if (bytesPerFrame == 0)
	{
		return 0;
	}

	/* Don't try to read more samples than can potentially fit in the output
	 * buffer. */
	if (framesToRead * bytesPerFrame > DRWAV_SIZE_MAX)
	{
		framesToRead = DRWAV_SIZE_MAX / bytesPerFrame;
	}

	return drwav_read_raw(pWav, (size_t)(framesToRead * bytesPerFrame),
	                      pBufferOut) /
	       bytesPerFrame;
}

drwav_uint64_t drwav_read_pcm_frames_be(drwav *pWav,
                                        drwav_uint64_t framesToRead,
                                        void *pBufferOut)
{
	drwav_uint64_t framesRead =
	    drwav_read_pcm_frames_le(pWav, framesToRead, pBufferOut);
	drwav__bswap_samples(pBufferOut, framesRead * pWav->channels,
	                     drwav_get_bytes_per_pcm_frame(pWav) / pWav->channels,
	                     pWav->translatedFormatTag);

	return framesRead;
}

drwav_uint64_t drwav_read_pcm_frames(drwav *pWav, drwav_uint64_t framesToRead,
                                     void *pBufferOut)
{
	if (drwav__is_little_endian())
	{
		return drwav_read_pcm_frames_le(pWav, framesToRead, pBufferOut);
	}
	else
	{
		return drwav_read_pcm_frames_be(pWav, framesToRead, pBufferOut);
	}
}

drwav_bool32 drwav_seek_to_first_pcm_frame(drwav *pWav)
{
	if (pWav->onWrite != NULL)
	{
		return DRWAV_FALSE; /* No seeking in write mode. */
	}

	if (!pWav->onSeek(pWav->pUserData, (int)pWav->dataChunkDataPos,
	                  drwav_seek_origin_start))
	{
		return DRWAV_FALSE;
	}

	if (drwav__is_compressed_format_tag(pWav->translatedFormatTag))
	{
		pWav->compressed.iCurrentPCMFrame = 0;
	}

	pWav->bytesRemaining = pWav->dataChunkDataSize;
	return DRWAV_TRUE;
}
drwav_bool32 drwav_seek_to_pcm_frame(drwav *pWav,
                                     drwav_uint64_t targetFrameIndex)
{
	/* Seeking should be compatible with wave files > 2GB. */

	if (pWav->onWrite != NULL)
	{
		return DRWAV_FALSE; /* No seeking in write mode. */
	}

	if (pWav == NULL || pWav->onSeek == NULL)
	{
		return DRWAV_FALSE;
	}

	/* If there are no samples, just return DRWAV_TRUE without doing anything.
	 */
	if (pWav->totalPCMFrameCount == 0)
	{
		return DRWAV_TRUE;
	}

	/* Make sure the sample is clamped. */
	if (targetFrameIndex >= pWav->totalPCMFrameCount)
	{
		targetFrameIndex = pWav->totalPCMFrameCount - 1;
	}

	/*
	For compressed formats we just use a slow generic seek. If we are seeking
	forward we just seek forward. If we are going backwards we need to seek back
	to the start.
	*/
	if (drwav__is_compressed_format_tag(pWav->translatedFormatTag))
	{
		/* TODO: This can be optimized. */

		/*
		If we're seeking forward it's simple - just keep reading samples until
		we hit the sample we're requesting. If we're seeking backwards, we first
		need to seek back to the start and then just do the same thing as a
		forward seek.
		*/
		if (targetFrameIndex < pWav->compressed.iCurrentPCMFrame)
		{
			if (!drwav_seek_to_first_pcm_frame(pWav))
			{
				return DRWAV_FALSE;
			}
		}

		if (targetFrameIndex > pWav->compressed.iCurrentPCMFrame)
		{
			drwav_uint64_t offsetInFrames =
			    targetFrameIndex - pWav->compressed.iCurrentPCMFrame;

			drwav_int16_t devnull[2048];
			while (offsetInFrames > 0)
			{
				drwav_uint64_t framesRead = 0;
				drwav_uint64_t framesToRead = offsetInFrames;
				if (framesToRead > drwav_countof(devnull) / pWav->channels)
				{
					framesToRead = drwav_countof(devnull) / pWav->channels;
				}

				if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM)
				{
					framesRead = drwav_read_pcm_frames_s16__msadpcm(
					    pWav, framesToRead, devnull);
				}
				else if (pWav->translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM)
				{
					framesRead = drwav_read_pcm_frames_s16__ima(
					    pWav, framesToRead, devnull);
				}
				else
				{
					assert(DRWAV_FALSE); /* If this assertion is triggered it
					                        means I've implemented a new
					                        compressed format but forgot to add
					                        a branch for it here. */
				}

				if (framesRead != framesToRead)
				{
					return DRWAV_FALSE;
				}

				offsetInFrames -= framesRead;
			}
		}
	}
	else
	{
		drwav_uint64_t totalSizeInBytes;
		drwav_uint64_t currentBytePos;
		drwav_uint64_t targetBytePos;
		drwav_uint64_t offset;

		totalSizeInBytes =
		    pWav->totalPCMFrameCount * drwav_get_bytes_per_pcm_frame(pWav);
		DRWAV_ASSERT(totalSizeInBytes >= pWav->bytesRemaining);

		currentBytePos = totalSizeInBytes - pWav->bytesRemaining;
		targetBytePos = targetFrameIndex * drwav_get_bytes_per_pcm_frame(pWav);

		if (currentBytePos < targetBytePos)
		{
			/* Offset forwards. */
			offset = (targetBytePos - currentBytePos);
		}
		else
		{
			/* Offset backwards. */
			if (!drwav_seek_to_first_pcm_frame(pWav))
			{
				return DRWAV_FALSE;
			}
			offset = targetBytePos;
		}

		while (offset > 0)
		{
			int offset32 = ((offset > INT_MAX) ? INT_MAX : (int)offset);
			if (!pWav->onSeek(pWav->pUserData, offset32,
			                  drwav_seek_origin_current))
			{
				return DRWAV_FALSE;
			}

			pWav->bytesRemaining -= offset32;
			offset -= offset32;
		}
	}

	return DRWAV_TRUE;
}

size_t drwav_write_raw(drwav *pWav, size_t bytesToWrite, const void *pData)
{
	size_t bytesWritten;

	if (pWav == NULL || bytesToWrite == 0 || pData == NULL)
	{
		return 0;
	}

	bytesWritten = pWav->onWrite(pWav->pUserData, pData, bytesToWrite);
	pWav->dataChunkDataSize += bytesWritten;

	return bytesWritten;
}

drwav_uint64_t drwav_write_pcm_frames_le(drwav *pWav,
                                         drwav_uint64_t framesToWrite,
                                         const void *pData)
{
	drwav_uint64_t bytesToWrite;
	drwav_uint64_t bytesWritten;
	const drwav_uint8_t *pRunningData;

	if (pWav == NULL || framesToWrite == 0 || pData == NULL)
	{
		return 0;
	}

	bytesToWrite = ((framesToWrite * pWav->channels * pWav->bitsPerSample) / 8);
	if (bytesToWrite > DRWAV_SIZE_MAX)
	{
		return 0;
	}

	bytesWritten = 0;
	pRunningData = (const drwav_uint8_t *)pData;

	while (bytesToWrite > 0)
	{
		size_t bytesJustWritten;
		drwav_uint64_t bytesToWriteThisIteration = bytesToWrite;
		if (bytesToWriteThisIteration > DRWAV_SIZE_MAX)
		{
			bytesToWriteThisIteration = DRWAV_SIZE_MAX;
		}

		bytesJustWritten = drwav_write_raw(
		    pWav, (size_t)bytesToWriteThisIteration, pRunningData);
		if (bytesJustWritten == 0)
		{
			break;
		}

		bytesToWrite -= bytesJustWritten;
		bytesWritten += bytesJustWritten;
		pRunningData += bytesJustWritten;
	}

	return (bytesWritten * 8) / pWav->bitsPerSample / pWav->channels;
}

drwav_uint64_t drwav_write_pcm_frames_be(drwav *pWav,
                                         drwav_uint64_t framesToWrite,
                                         const void *pData)
{
	drwav_uint64_t bytesToWrite;
	drwav_uint64_t bytesWritten;
	drwav_uint32_t bytesPerSample;
	const drwav_uint8_t *pRunningData;

	if (pWav == NULL || framesToWrite == 0 || pData == NULL)
	{
		return 0;
	}

	bytesToWrite = ((framesToWrite * pWav->channels * pWav->bitsPerSample) / 8);
	if (bytesToWrite > DRWAV_SIZE_MAX)
	{
		return 0;
	}

	bytesWritten = 0;
	pRunningData = (const drwav_uint8_t *)pData;

	bytesPerSample = drwav_get_bytes_per_pcm_frame(pWav) / pWav->channels;

	while (bytesToWrite > 0)
	{
		drwav_uint8_t temp[4096];
		drwav_uint32_t sampleCount;
		size_t bytesJustWritten;
		drwav_uint64_t bytesToWriteThisIteration;

		bytesToWriteThisIteration = bytesToWrite;
		if (bytesToWriteThisIteration > DRWAV_SIZE_MAX)
		{
			bytesToWriteThisIteration = DRWAV_SIZE_MAX;
		}

		/*
		WAV files are always little-endian. We need to byte swap on big-endian
		architectures. Since our input buffer is read-only we need to use an
		intermediary buffer for the conversion.
		*/
		sampleCount = sizeof(temp) / bytesPerSample;

		if (bytesToWriteThisIteration > sampleCount * bytesPerSample)
		{
			bytesToWriteThisIteration = sampleCount * bytesPerSample;
		}

		DRWAV_COPY_MEMORY(temp, pRunningData,
		                  (size_t)bytesToWriteThisIteration);
		drwav__bswap_samples(temp, sampleCount, bytesPerSample,
		                     pWav->translatedFormatTag);

		bytesJustWritten =
		    drwav_write_raw(pWav, (size_t)bytesToWriteThisIteration, temp);
		if (bytesJustWritten == 0)
		{
			break;
		}

		bytesToWrite -= bytesJustWritten;
		bytesWritten += bytesJustWritten;
		pRunningData += bytesJustWritten;
	}

	return (bytesWritten * 8) / pWav->bitsPerSample / pWav->channels;
}

drwav_uint64_t drwav_write_pcm_frames(drwav *pWav, drwav_uint64_t framesToWrite,
                                      const void *pData)
{
	if (drwav__is_little_endian())
	{
		return drwav_write_pcm_frames_le(pWav, framesToWrite, pData);
	}
	else
	{
		return drwav_write_pcm_frames_be(pWav, framesToWrite, pData);
	}
}
