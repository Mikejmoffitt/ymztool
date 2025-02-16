#include "3rdparty/dr_wav/dr_wav_init.h"
#include "3rdparty/dr_wav/dr_wav_fileops.h"
#include "3rdparty/dr_wav/dr_wav_util.h"

drwav_bool32
drwav_preinit(drwav *pWav, drwav_read_proc onRead, drwav_seek_proc onSeek,
              void *pReadSeekUserData,
              const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (pWav == NULL || onRead == NULL || onSeek == NULL)
	{
		return DRWAV_FALSE;
	}

	DRWAV_ZERO_MEMORY(pWav, sizeof(*pWav));
	pWav->onRead = onRead;
	pWav->onSeek = onSeek;
	pWav->pUserData = pReadSeekUserData;
	pWav->allocationCallbacks =
	    drwav_copy_allocation_callbacks_or_defaults(pAllocationCallbacks);

	if (pWav->allocationCallbacks.onFree == NULL ||
	    (pWav->allocationCallbacks.onMalloc == NULL &&
	     pWav->allocationCallbacks.onRealloc == NULL))
	{
		return DRWAV_FALSE; /* Invalid allocation callbacks. */
	}

	return DRWAV_TRUE;
}

drwav_bool32 drwav_init(drwav *pWav, drwav_read_proc onRead,
                        drwav_seek_proc onSeek, void *pUserData,
                        const drwav_allocation_callbacks *pAllocationCallbacks)
{
	return drwav_init_ex(pWav, onRead, onSeek, NULL, pUserData, NULL, 0,
	                     pAllocationCallbacks);
}

drwav_bool32
drwav_init_ex(drwav *pWav, drwav_read_proc onRead, drwav_seek_proc onSeek,
              drwav_chunk_proc onChunk, void *pReadSeekUserData,
              void *pChunkUserData, drwav_uint32_t flags,
              const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (!drwav_preinit(pWav, onRead, onSeek, pReadSeekUserData,
	                   pAllocationCallbacks))
	{
		return DRWAV_FALSE;
	}

	return drwav_init__internal(pWav, onChunk, pChunkUserData, flags);
}

drwav_bool32
drwav_preinit_write(drwav *pWav, const drwav_data_format *pFormat,
                    drwav_bool32 isSequential, drwav_write_proc onWrite,
                    drwav_seek_proc onSeek, void *pUserData,
                    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (pWav == NULL || onWrite == NULL)
	{
		return DRWAV_FALSE;
	}

	if (!isSequential && onSeek == NULL)
	{
		return DRWAV_FALSE; /* <-- onSeek is required when in non-sequential
		                       mode. */
	}

	/* Not currently supporting compressed formats. Will need to add support for
	 * the "fact" chunk before we enable this. */
	if (pFormat->format == DR_WAVE_FORMAT_EXTENSIBLE)
	{
		return DRWAV_FALSE;
	}
	if (pFormat->format == DR_WAVE_FORMAT_ADPCM ||
	    pFormat->format == DR_WAVE_FORMAT_DVI_ADPCM)
	{
		return DRWAV_FALSE;
	}

	DRWAV_ZERO_MEMORY(pWav, sizeof(*pWav));
	pWav->onWrite = onWrite;
	pWav->onSeek = onSeek;
	pWav->pUserData = pUserData;
	pWav->allocationCallbacks =
	    drwav_copy_allocation_callbacks_or_defaults(pAllocationCallbacks);

	if (pWav->allocationCallbacks.onFree == NULL ||
	    (pWav->allocationCallbacks.onMalloc == NULL &&
	     pWav->allocationCallbacks.onRealloc == NULL))
	{
		return DRWAV_FALSE; /* Invalid allocation callbacks. */
	}

	pWav->fmt.formatTag = (drwav_uint16_t)pFormat->format;
	pWav->fmt.channels = (drwav_uint16_t)pFormat->channels;
	pWav->fmt.sampleRate = pFormat->sampleRate;
	pWav->fmt.avgBytesPerSec = (drwav_uint32_t)(
	    (pFormat->bitsPerSample * pFormat->sampleRate * pFormat->channels) / 8);
	pWav->fmt.blockAlign =
	    (drwav_uint16_t)((pFormat->channels * pFormat->bitsPerSample) / 8);
	pWav->fmt.bitsPerSample = (drwav_uint16_t)pFormat->bitsPerSample;
	pWav->fmt.extendedSize = 0;
	pWav->isSequentialWrite = isSequential;

	return DRWAV_TRUE;
}

drwav_bool32 drwav_init_write__internal(drwav *pWav,
                                        const drwav_data_format *pFormat,
                                        drwav_uint64_t totalSampleCount)
{
	/* The function assumes drwav_preinit_write() was called beforehand. */

	size_t runningPos = 0;
	drwav_uint64_t initialDataChunkSize = 0;
	drwav_uint64_t chunkSizeFMT;

	/*
	The initial values for the "RIFF" and "data" chunks depends on whether or
	not we are initializing in sequential mode or not. In sequential mode we set
	this to its final values straight away since they can be calculated from the
	total sample count. In non- sequential mode we initialize it all to zero and
	fill it out in drwav_uninit() using a backwards seek.
	*/
	if (pWav->isSequentialWrite)
	{
		initialDataChunkSize = (totalSampleCount * pWav->fmt.bitsPerSample) / 8;

		/*
		The RIFF container has a limit on the number of samples. drwav is not
		allowing this. There's no practical limits for Wave64 so for the sake of
		simplicity I'm not doing any validation for that.
		*/
		if (pFormat->container == drwav_container_riff)
		{
			if (initialDataChunkSize > (0xFFFFFFFFUL - 36))
			{
				return DRWAV_FALSE; /* Not enough room to store every sample. */
			}
		}
	}

	pWav->dataChunkDataSizeTargetWrite = initialDataChunkSize;

	/* "RIFF" chunk. */
	if (pFormat->container == drwav_container_riff)
	{
		drwav_uint32_t chunkSizeRIFF =
		    36 +
		    (drwav_uint32_t)
		        initialDataChunkSize; /* +36 = "RIFF"+[RIFF Chunk Size]+"WAVE" +
		                                 [sizeof "fmt " chunk] */
		runningPos += pWav->onWrite(pWav->pUserData, "RIFF", 4);
		runningPos += pWav->onWrite(pWav->pUserData, &chunkSizeRIFF, 4);
		runningPos += pWav->onWrite(pWav->pUserData, "WAVE", 4);
	}
	else
	{
		drwav_uint64_t chunkSizeRIFF =
		    80 + 24 + initialDataChunkSize; /* +24 because W64 includes the size
		                                       of the GUID and size fields. */
		runningPos += pWav->onWrite(pWav->pUserData, drwavGUID_W64_RIFF, 16);
		runningPos += pWav->onWrite(pWav->pUserData, &chunkSizeRIFF, 8);
		runningPos += pWav->onWrite(pWav->pUserData, drwavGUID_W64_WAVE, 16);
	}

	/* "fmt " chunk. */
	if (pFormat->container == drwav_container_riff)
	{
		chunkSizeFMT = 16;
		runningPos += pWav->onWrite(pWav->pUserData, "fmt ", 4);
		runningPos += pWav->onWrite(pWav->pUserData, &chunkSizeFMT, 4);
	}
	else
	{
		chunkSizeFMT = 40;
		runningPos += pWav->onWrite(pWav->pUserData, drwavGUID_W64_FMT, 16);
		runningPos += pWav->onWrite(pWav->pUserData, &chunkSizeFMT, 8);
	}

	runningPos += pWav->onWrite(pWav->pUserData, &pWav->fmt.formatTag, 2);
	runningPos += pWav->onWrite(pWav->pUserData, &pWav->fmt.channels, 2);
	runningPos += pWav->onWrite(pWav->pUserData, &pWav->fmt.sampleRate, 4);
	runningPos += pWav->onWrite(pWav->pUserData, &pWav->fmt.avgBytesPerSec, 4);
	runningPos += pWav->onWrite(pWav->pUserData, &pWav->fmt.blockAlign, 2);
	runningPos += pWav->onWrite(pWav->pUserData, &pWav->fmt.bitsPerSample, 2);

	pWav->dataChunkDataPos = runningPos;

	/* "data" chunk. */
	if (pFormat->container == drwav_container_riff)
	{
		drwav_uint32_t chunkSizeDATA = (drwav_uint32_t)initialDataChunkSize;
		runningPos += pWav->onWrite(pWav->pUserData, "data", 4);
		runningPos += pWav->onWrite(pWav->pUserData, &chunkSizeDATA, 4);
	}
	else
	{
		drwav_uint64_t chunkSizeDATA =
		    24 + initialDataChunkSize; /* +24 because W64 includes the size of
		                                  the GUID and size fields. */
		runningPos += pWav->onWrite(pWav->pUserData, drwavGUID_W64_DATA, 16);
		runningPos += pWav->onWrite(pWav->pUserData, &chunkSizeDATA, 8);
	}

	/* Simple validation. */
	if (pFormat->container == drwav_container_riff)
	{
		if (runningPos != 20 + chunkSizeFMT + 8)
		{
			return DRWAV_FALSE;
		}
	}
	else
	{
		if (runningPos != 40 + chunkSizeFMT + 24)
		{
			return DRWAV_FALSE;
		}
	}

	/* Set some properties for the client's convenience. */
	pWav->container = pFormat->container;
	pWav->channels = (drwav_uint16_t)pFormat->channels;
	pWav->sampleRate = pFormat->sampleRate;
	pWav->bitsPerSample = (drwav_uint16_t)pFormat->bitsPerSample;
	pWav->translatedFormatTag = (drwav_uint16_t)pFormat->format;

	return DRWAV_TRUE;
}

drwav_bool32
drwav_init_write(drwav *pWav, const drwav_data_format *pFormat,
                 drwav_write_proc onWrite, drwav_seek_proc onSeek,
                 void *pUserData,
                 const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (!drwav_preinit_write(pWav, pFormat, DRWAV_FALSE, onWrite, onSeek,
	                         pUserData, pAllocationCallbacks))
	{
		return DRWAV_FALSE;
	}

	return drwav_init_write__internal(pWav, pFormat,
	                                  0); /* DRWAV_FALSE = Not Sequential */
}

drwav_bool32 drwav_init_write_sequential(
    drwav *pWav, const drwav_data_format *pFormat,
    drwav_uint64_t totalSampleCount, drwav_write_proc onWrite, void *pUserData,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (!drwav_preinit_write(pWav, pFormat, DRWAV_TRUE, onWrite, NULL,
	                         pUserData, pAllocationCallbacks))
	{
		return DRWAV_FALSE;
	}

	return drwav_init_write__internal(
	    pWav, pFormat, totalSampleCount); /* DRWAV_TRUE = Sequential */
}

drwav_bool32 drwav_init_write_sequential_pcm_frames(
    drwav *pWav, const drwav_data_format *pFormat,
    drwav_uint64_t totalPCMFrameCount, drwav_write_proc onWrite,
    void *pUserData, const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (pFormat == NULL)
	{
		return DRWAV_FALSE;
	}

	return drwav_init_write_sequential(
	    pWav, pFormat, totalPCMFrameCount * pFormat->channels, onWrite,
	    pUserData, pAllocationCallbacks);
}

drwav_bool32 drwav_init__internal(drwav *pWav, drwav_chunk_proc onChunk,
                                  void *pChunkUserData, drwav_uint32_t flags)
{
	/* This function assumes drwav_preinit() has been called beforehand. */

	drwav_uint64_t cursor; /* <-- Keeps track of the byte position so we can
	                          seek to specific locations. */
	drwav_bool32 sequential;
	unsigned char riff[4];
	drwav_fmt fmt;
	unsigned short translatedFormatTag;
	drwav_uint64_t sampleCountFromFactChunk;
	drwav_bool32 foundDataChunk;
	drwav_uint64_t dataChunkSize;
	drwav_uint64_t chunkSize;

	cursor = 0;
	sequential = (flags & DRWAV_SEQUENTIAL) != 0;

	/* The first 4 bytes should be the RIFF identifier. */
	if (drwav__on_read(pWav->onRead, pWav->pUserData, riff, sizeof(riff),
	                   &cursor) != sizeof(riff))
	{
		return DRWAV_FALSE;
	}

	/*
	The first 4 bytes can be used to identify the container. For RIFF files it
	will start with "RIFF" and for w64 it will start with "riff".
	*/
	if (drwav__fourcc_equal(riff, "RIFF"))
	{
		pWav->container = drwav_container_riff;
	}
	else if (drwav__fourcc_equal(riff, "riff"))
	{
		int i;
		drwav_uint8_t riff2[12];

		pWav->container = drwav_container_w64;

		/* Check the rest of the GUID for validity. */
		if (drwav__on_read(pWav->onRead, pWav->pUserData, riff2, sizeof(riff2),
		                   &cursor) != sizeof(riff2))
		{
			return DRWAV_FALSE;
		}

		for (i = 0; i < 12; ++i)
		{
			if (riff2[i] != drwavGUID_W64_RIFF[i + 4])
			{
				return DRWAV_FALSE;
			}
		}
	}
	else
	{
		return DRWAV_FALSE; /* Unknown or unsupported container. */
	}

	if (pWav->container == drwav_container_riff)
	{
		unsigned char chunkSizeBytes[4];
		unsigned char wave[4];

		/* RIFF/WAVE */
		if (drwav__on_read(pWav->onRead, pWav->pUserData, chunkSizeBytes,
		                   sizeof(chunkSizeBytes),
		                   &cursor) != sizeof(chunkSizeBytes))
		{
			return DRWAV_FALSE;
		}

		if (drwav__bytes_to_u32(chunkSizeBytes) < 36)
		{
			return DRWAV_FALSE; /* Chunk size should always be at least 36
			                       bytes. */
		}

		if (drwav__on_read(pWav->onRead, pWav->pUserData, wave, sizeof(wave),
		                   &cursor) != sizeof(wave))
		{
			return DRWAV_FALSE;
		}

		if (!drwav__fourcc_equal(wave, "WAVE"))
		{
			return DRWAV_FALSE; /* Expecting "WAVE". */
		}
	}
	else
	{
		unsigned char chunkSizeBytes[8];
		drwav_uint8_t wave[16];

		/* W64 */
		if (drwav__on_read(pWav->onRead, pWav->pUserData, chunkSizeBytes,
		                   sizeof(chunkSizeBytes),
		                   &cursor) != sizeof(chunkSizeBytes))
		{
			return DRWAV_FALSE;
		}

		if (drwav__bytes_to_u64(chunkSizeBytes) < 80)
		{
			return DRWAV_FALSE;
		}

		if (drwav__on_read(pWav->onRead, pWav->pUserData, wave, sizeof(wave),
		                   &cursor) != sizeof(wave))
		{
			return DRWAV_FALSE;
		}

		if (!drwav__guid_equal(wave, drwavGUID_W64_WAVE))
		{
			return DRWAV_FALSE;
		}
	}

	/* The next bytes should be the "fmt " chunk. */
	if (!drwav__read_fmt(pWav->onRead, pWav->onSeek, pWav->pUserData,
	                     pWav->container, &cursor, &fmt))
	{
		return DRWAV_FALSE; /* Failed to read the "fmt " chunk. */
	}

	/* Basic validation. */
	if (fmt.sampleRate == 0 || fmt.channels == 0 || fmt.bitsPerSample == 0 ||
	    fmt.blockAlign == 0)
	{
		return DRWAV_FALSE; /* Invalid channel count. Probably an invalid WAV
		                       file. */
	}

	/* Translate the internal format. */
	translatedFormatTag = fmt.formatTag;
	if (translatedFormatTag == DR_WAVE_FORMAT_EXTENSIBLE)
	{
		translatedFormatTag = drwav__bytes_to_u16(fmt.subFormat + 0);
	}

	sampleCountFromFactChunk = 0;

	/*
	We need to enumerate over each chunk for two reasons:
	  1) The "data" chunk may not be the next one
	  2) We may want to report each chunk back to the client

	In order to correctly report each chunk back to the client we will need to
	keep looping until the end of the file.
	*/
	foundDataChunk = DRWAV_FALSE;
	dataChunkSize = 0;

	/* The next chunk we care about is the "data" chunk. This is not necessarily
	 * the next chunk so we'll need to loop. */
	for (;;)
	{
		drwav_chunk_header header;
		drwav_result result = drwav__read_chunk_header(
		    pWav->onRead, pWav->pUserData, pWav->container, &cursor, &header);
		if (result != DRWAV_SUCCESS)
		{
			if (!foundDataChunk)
			{
				return DRWAV_FALSE;
			}
			else
			{
				break; /* Probably at the end of the file. Get out of the loop.
				        */
			}
		}

		/* Tell the client about this chunk. */
		if (!sequential && onChunk != NULL)
		{
			drwav_uint64_t callbackBytesRead =
			    onChunk(pChunkUserData, pWav->onRead, pWav->onSeek,
			            pWav->pUserData, &header);

			/*
			dr_wav may need to read the contents of the chunk, so we now need to
			seek back to the position before we called the callback.
			*/
			if (callbackBytesRead > 0)
			{
				if (!drwav__seek_from_start(pWav->onSeek, cursor,
				                            pWav->pUserData))
				{
					return DRWAV_FALSE;
				}
			}
		}

		if (!foundDataChunk)
		{
			pWav->dataChunkDataPos = cursor;
		}

		chunkSize = header.sizeInBytes;
		if (pWav->container == drwav_container_riff)
		{
			if (drwav__fourcc_equal(header.id.fourcc, "data"))
			{
				foundDataChunk = DRWAV_TRUE;
				dataChunkSize = chunkSize;
			}
		}
		else
		{
			if (drwav__guid_equal(header.id.guid, drwavGUID_W64_DATA))
			{
				foundDataChunk = DRWAV_TRUE;
				dataChunkSize = chunkSize;
			}
		}

		/*
		If at this point we have found the data chunk and we're running in
		sequential mode, we need to break out of this loop. The reason for this
		is that we would otherwise require a backwards seek which sequential
		mode forbids.
		*/
		if (foundDataChunk && sequential)
		{
			break;
		}

		/* Optional. Get the total sample count from the FACT chunk. This is
		 * useful for compressed formats. */
		if (pWav->container == drwav_container_riff)
		{
			if (drwav__fourcc_equal(header.id.fourcc, "fact"))
			{
				drwav_uint32_t sampleCount;
				if (drwav__on_read(pWav->onRead, pWav->pUserData, &sampleCount,
				                   4, &cursor) != 4)
				{
					return DRWAV_FALSE;
				}
				chunkSize -= 4;

				if (!foundDataChunk)
				{
					pWav->dataChunkDataPos = cursor;
				}

				/*
				The sample count in the "fact" chunk is either unreliable, or
				I'm not understanding it properly. For now I am only enabling
				this for Microsoft ADPCM formats.
				*/
				if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM)
				{
					sampleCountFromFactChunk = sampleCount;
				}
				else
				{
					sampleCountFromFactChunk = 0;
				}
			}
		}
		else
		{
			if (drwav__guid_equal(header.id.guid, drwavGUID_W64_FACT))
			{
				if (drwav__on_read(pWav->onRead, pWav->pUserData,
				                   &sampleCountFromFactChunk, 8, &cursor) != 8)
				{
					return DRWAV_FALSE;
				}
				chunkSize -= 8;

				if (!foundDataChunk)
				{
					pWav->dataChunkDataPos = cursor;
				}
			}
		}

		/* "smpl" chunk. */
		if (pWav->container == drwav_container_riff)
		{
			if (drwav__fourcc_equal(header.id.fourcc, "smpl"))
			{
				unsigned char
				    smplHeaderData[36]; /* 36 = size of the smpl header section,
				                           not including the loop data. */
				if (chunkSize >= sizeof(smplHeaderData))
				{
					drwav_uint64_t bytesJustRead = drwav__on_read(
					    pWav->onRead, pWav->pUserData, smplHeaderData,
					    sizeof(smplHeaderData), &cursor);
					chunkSize -= bytesJustRead;

					if (bytesJustRead == sizeof(smplHeaderData))
					{
						drwav_uint32_t iLoop;

						pWav->smpl.manufacturer =
						    drwav__bytes_to_u32(smplHeaderData + 0);
						pWav->smpl.product =
						    drwav__bytes_to_u32(smplHeaderData + 4);
						pWav->smpl.samplePeriod =
						    drwav__bytes_to_u32(smplHeaderData + 8);
						pWav->smpl.midiUnityNotes =
						    drwav__bytes_to_u32(smplHeaderData + 12);
						pWav->smpl.midiPitchFraction =
						    drwav__bytes_to_u32(smplHeaderData + 16);
						pWav->smpl.smpteFormat =
						    drwav__bytes_to_u32(smplHeaderData + 20);
						pWav->smpl.smpteOffset =
						    drwav__bytes_to_u32(smplHeaderData + 24);
						pWav->smpl.numSampleLoops =
						    drwav__bytes_to_u32(smplHeaderData + 28);
						pWav->smpl.samplerData =
						    drwav__bytes_to_u32(smplHeaderData + 32);

						for (iLoop = 0; iLoop < pWav->smpl.numSampleLoops &&
						                iLoop < drwav_countof(pWav->smpl.loops);
						     ++iLoop)
						{
							unsigned char
							    smplLoopData[24]; /* 24 = size of a loop section
							                         in the smpl chunk. */
							bytesJustRead = drwav__on_read(
							    pWav->onRead, pWav->pUserData, smplLoopData,
							    sizeof(smplLoopData), &cursor);
							chunkSize -= bytesJustRead;

							if (bytesJustRead == sizeof(smplLoopData))
							{
								pWav->smpl.loops[iLoop].cuePointId =
								    drwav__bytes_to_u32(smplLoopData + 0);
								pWav->smpl.loops[iLoop].type =
								    drwav__bytes_to_u32(smplLoopData + 4);
								pWav->smpl.loops[iLoop].start =
								    drwav__bytes_to_u32(smplLoopData + 8);
								pWav->smpl.loops[iLoop].end =
								    drwav__bytes_to_u32(smplLoopData + 12);
								pWav->smpl.loops[iLoop].fraction =
								    drwav__bytes_to_u32(smplLoopData + 16);
								pWav->smpl.loops[iLoop].playCount =
								    drwav__bytes_to_u32(smplLoopData + 20);
							}
							else
							{
								break; /* Break from the smpl loop for loop. */
							}
						}
					}
				}
				else
				{
					/* Looks like invalid data. Ignore the chunk. */
				}
			}
		}
		else
		{
			if (drwav__guid_equal(header.id.guid, drwavGUID_W64_SMPL))
			{
				/*
				This path will be hit when a W64 WAV file contains a smpl chunk.
				I don't have a sample file to test this path, so a contribution
				is welcome to add support for this.
				*/
			}
		}

		/* Make sure we seek past the padding. */
		chunkSize += header.paddingSize;
		if (!drwav__seek_forward(pWav->onSeek, chunkSize, pWav->pUserData))
		{
			break;
		}
		cursor += chunkSize;

		if (!foundDataChunk)
		{
			pWav->dataChunkDataPos = cursor;
		}
	}

	/* If we haven't found a data chunk, return an error. */
	if (!foundDataChunk)
	{
		return DRWAV_FALSE;
	}

	/* We may have moved passed the data chunk. If so we need to move back. If
	 * running in sequential mode we can assume we are already sitting on the
	 * data chunk. */
	if (!sequential)
	{
		if (!drwav__seek_from_start(pWav->onSeek, pWav->dataChunkDataPos,
		                            pWav->pUserData))
		{
			return DRWAV_FALSE;
		}
		cursor = pWav->dataChunkDataPos;
	}

	/* At this point we should be sitting on the first byte of the raw audio
	 * data. */

	pWav->fmt = fmt;
	pWav->sampleRate = fmt.sampleRate;
	pWav->channels = fmt.channels;
	pWav->bitsPerSample = fmt.bitsPerSample;
	pWav->bytesRemaining = dataChunkSize;
	pWav->translatedFormatTag = translatedFormatTag;
	pWav->dataChunkDataSize = dataChunkSize;

	if (sampleCountFromFactChunk != 0)
	{
		pWav->totalPCMFrameCount = sampleCountFromFactChunk;
	}
	else
	{
		pWav->totalPCMFrameCount =
		    dataChunkSize / drwav_get_bytes_per_pcm_frame(pWav);

		if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM)
		{
			drwav_uint64_t totalBlockHeaderSizeInBytes;
			drwav_uint64_t blockCount = dataChunkSize / fmt.blockAlign;

			/* Make sure any trailing partial block is accounted for. */
			if ((blockCount * fmt.blockAlign) < dataChunkSize)
			{
				blockCount += 1;
			}

			/* We decode two samples per byte. There will be blockCount headers
			 * in the data chunk. This is enough to know how to calculate the
			 * total PCM frame count. */
			totalBlockHeaderSizeInBytes = blockCount * (6 * fmt.channels);
			pWav->totalPCMFrameCount =
			    ((dataChunkSize - totalBlockHeaderSizeInBytes) * 2) /
			    fmt.channels;
		}
		if (pWav->translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM)
		{
			drwav_uint64_t totalBlockHeaderSizeInBytes;
			drwav_uint64_t blockCount = dataChunkSize / fmt.blockAlign;

			/* Make sure any trailing partial block is accounted for. */
			if ((blockCount * fmt.blockAlign) < dataChunkSize)
			{
				blockCount += 1;
			}

			/* We decode two samples per byte. There will be blockCount headers
			 * in the data chunk. This is enough to know how to calculate the
			 * total PCM frame count. */
			totalBlockHeaderSizeInBytes = blockCount * (4 * fmt.channels);
			pWav->totalPCMFrameCount =
			    ((dataChunkSize - totalBlockHeaderSizeInBytes) * 2) /
			    fmt.channels;

			/* The header includes a decoded sample for each channel which acts
			 * as the initial predictor sample. */
			pWav->totalPCMFrameCount += blockCount;
		}
	}

	/* Some formats only support a certain number of channels. */
	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM ||
	    pWav->translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM)
	{
		if (pWav->channels > 2)
		{
			return DRWAV_FALSE;
		}
	}

#ifdef DR_WAV_LIBSNDFILE_COMPAT
	/*
	I use libsndfile as a benchmark for testing, however in the version I'm
	using (from the Windows installer on the libsndfile website), it appears the
	total sample count libsndfile uses for MS-ADPCM is incorrect. It would seem
	they are computing the total sample count from the number of blocks, however
	this results in the inclusion of extra silent samples at the end of the last
	block. The correct way to know the total sample count is to inspect the
	"fact" chunk, which should always be present for compressed formats, and
	should always include the sample count. This little block of code below is
	only used to emulate the libsndfile logic so I can properly run my
	correctness tests against libsndfile, and is disabled by default.
	*/
	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_ADPCM)
	{
		drwav_uint64_t blockCount = dataChunkSize / fmt.blockAlign;
		pWav->totalPCMFrameCount =
		    (((blockCount * (fmt.blockAlign - (6 * pWav->channels))) * 2)) /
		    fmt.channels; /* x2 because two samples per byte. */
	}
	if (pWav->translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM)
	{
		drwav_uint64_t blockCount = dataChunkSize / fmt.blockAlign;
		pWav->totalPCMFrameCount =
		    (((blockCount * (fmt.blockAlign - (4 * pWav->channels))) * 2) +
		     (blockCount * pWav->channels)) /
		    fmt.channels;
	}
#endif

	return DRWAV_TRUE;
}

drwav_result drwav_uninit(drwav *pWav)
{
	drwav_result result = DRWAV_SUCCESS;

	if (pWav == NULL)
	{
		return DRWAV_INVALID_ARGS;
	}

	/*
	If the drwav object was opened in write mode we'll need to finalize a few
	things:
	  - Make sure the "data" chunk is aligned to 16-bits for RIFF containers, or
	64 bits for W64 containers.
	  - Set the size of the "data" chunk.
	*/
	if (pWav->onWrite != NULL)
	{
		drwav_uint32_t paddingSize = 0;

		/* Padding. Do not adjust pWav->dataChunkDataSize - this should not
		 * include the padding. */
		if (pWav->container == drwav_container_riff)
		{
			paddingSize =
			    drwav__chunk_padding_size_riff(pWav->dataChunkDataSize);
		}
		else
		{
			paddingSize =
			    drwav__chunk_padding_size_w64(pWav->dataChunkDataSize);
		}

		if (paddingSize > 0)
		{
			drwav_uint64_t paddingData = 0;
			pWav->onWrite(pWav->pUserData, &paddingData, paddingSize);
		}

		/*
		Chunk sizes. When using sequential mode, these will have been filled in
		at initialization time. We only need to do this when using
		non-sequential mode.
		*/
		if (pWav->onSeek && !pWav->isSequentialWrite)
		{
			if (pWav->container == drwav_container_riff)
			{
				/* The "RIFF" chunk size. */
				if (pWav->onSeek(pWav->pUserData, 4, drwav_seek_origin_start))
				{
					drwav_uint32_t riffChunkSize =
					    drwav__riff_chunk_size_riff(pWav->dataChunkDataSize);
					pWav->onWrite(pWav->pUserData, &riffChunkSize, 4);
				}

				/* the "data" chunk size. */
				if (pWav->onSeek(pWav->pUserData,
				                 (int)pWav->dataChunkDataPos + 4,
				                 drwav_seek_origin_start))
				{
					drwav_uint32_t dataChunkSize =
					    drwav__data_chunk_size_riff(pWav->dataChunkDataSize);
					pWav->onWrite(pWav->pUserData, &dataChunkSize, 4);
				}
			}
			else
			{
				/* The "RIFF" chunk size. */
				if (pWav->onSeek(pWav->pUserData, 16, drwav_seek_origin_start))
				{
					drwav_uint64_t riffChunkSize =
					    drwav__riff_chunk_size_w64(pWav->dataChunkDataSize);
					pWav->onWrite(pWav->pUserData, &riffChunkSize, 8);
				}

				/* The "data" chunk size. */
				if (pWav->onSeek(pWav->pUserData,
				                 (int)pWav->dataChunkDataPos + 16,
				                 drwav_seek_origin_start))
				{
					drwav_uint64_t dataChunkSize =
					    drwav__data_chunk_size_w64(pWav->dataChunkDataSize);
					pWav->onWrite(pWav->pUserData, &dataChunkSize, 8);
				}
			}
		}

		/* Validation for sequential mode. */
		if (pWav->isSequentialWrite)
		{
			if (pWav->dataChunkDataSize != pWav->dataChunkDataSizeTargetWrite)
			{
				result = DRWAV_INVALID_FILE;
			}
		}
	}

#ifndef DR_WAV_NO_STDIO
	/*
	If we opened the file with drwav_open_file() we will want to close the file
	handle. We can know whether or not drwav_open_file() was used by looking at
	the onRead and onSeek callbacks.
	*/
	if (pWav->onRead == drwav__on_read_stdio ||
	    pWav->onWrite == drwav__on_write_stdio)
	{
		fclose((FILE *)pWav->pUserData);
	}
#endif

	return result;
}

#ifndef DR_WAV_NO_STDIO

static drwav_bool32 drwav__on_seek_stdio(void *pUserData, int offset,
                                         drwav_seek_origin origin)
{
	return fseek((FILE *)pUserData, offset,
	             (origin == drwav_seek_origin_current) ? SEEK_CUR : SEEK_SET) ==
	       0;
}

drwav_bool32
drwav_init_file(drwav *pWav, const char *filename,
                const drwav_allocation_callbacks *pAllocationCallbacks)
{
	return drwav_init_file_ex(pWav, filename, NULL, NULL, 0,
	                          pAllocationCallbacks);
}

drwav_bool32 drwav_init_file__internal_FILE(
    drwav *pWav, FILE *pFile, drwav_chunk_proc onChunk, void *pChunkUserData,
    drwav_uint32_t flags,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (!drwav_preinit(pWav, drwav__on_read_stdio, drwav__on_seek_stdio,
	                   (void *)pFile, pAllocationCallbacks))
	{
		fclose(pFile);
		return DRWAV_FALSE;
	}

	return drwav_init__internal(pWav, onChunk, pChunkUserData, flags);
}

drwav_bool32
drwav_init_file_ex(drwav *pWav, const char *filename, drwav_chunk_proc onChunk,
                   void *pChunkUserData, drwav_uint32_t flags,
                   const drwav_allocation_callbacks *pAllocationCallbacks)
{
	FILE *pFile = drwav_fopen(filename, "rb");
	if (pFile == NULL)
	{
		return DRWAV_FALSE;
	}

	/* This takes ownership of the FILE* object. */
	return drwav_init_file__internal_FILE(pWav, pFile, onChunk, pChunkUserData,
	                                      flags, pAllocationCallbacks);
}

drwav_bool32
drwav_init_file_w(drwav *pWav, const wchar_t *filename,
                  const drwav_allocation_callbacks *pAllocationCallbacks)
{
	return drwav_init_file_ex_w(pWav, filename, NULL, NULL, 0,
	                            pAllocationCallbacks);
}

drwav_bool32
drwav_init_file_ex_w(drwav *pWav, const wchar_t *filename,
                     drwav_chunk_proc onChunk, void *pChunkUserData,
                     drwav_uint32_t flags,
                     const drwav_allocation_callbacks *pAllocationCallbacks)
{
	FILE *pFile = drwav_wfopen(filename, L"rb", pAllocationCallbacks);
	if (pFile == NULL)
	{
		return DRWAV_FALSE;
	}

	/* This takes ownership of the FILE* object. */
	return drwav_init_file__internal_FILE(pWav, pFile, onChunk, pChunkUserData,
	                                      flags, pAllocationCallbacks);
}

drwav_bool32 drwav_init_file_write__internal_FILE(
    drwav *pWav, FILE *pFile, const drwav_data_format *pFormat,
    drwav_uint64_t totalSampleCount, drwav_bool32 isSequential,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (!drwav_preinit_write(pWav, pFormat, isSequential, drwav__on_write_stdio,
	                         drwav__on_seek_stdio, (void *)pFile,
	                         pAllocationCallbacks))
	{
		fclose(pFile);
		return DRWAV_FALSE;
	}

	return drwav_init_write__internal(pWav, pFormat, totalSampleCount);
}

drwav_bool32 drwav_init_file_write__internal(
    drwav *pWav, const char *filename, const drwav_data_format *pFormat,
    drwav_uint64_t totalSampleCount, drwav_bool32 isSequential,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	FILE *pFile = drwav_fopen(filename, "wb");
	if (pFile == NULL)
	{
		return DRWAV_FALSE;
	}

	/* This takes ownership of the FILE* object. */
	return drwav_init_file_write__internal_FILE(pWav, pFile, pFormat,
	                                            totalSampleCount, isSequential,
	                                            pAllocationCallbacks);
}

drwav_bool32 drwav_init_file_write_w__internal(
    drwav *pWav, const wchar_t *filename, const drwav_data_format *pFormat,
    drwav_uint64_t totalSampleCount, drwav_bool32 isSequential,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	FILE *pFile = drwav_wfopen(filename, L"wb", pAllocationCallbacks);
	if (pFile == NULL)
	{
		return DRWAV_FALSE;
	}

	/* This takes ownership of the FILE* object. */
	return drwav_init_file_write__internal_FILE(pWav, pFile, pFormat,
	                                            totalSampleCount, isSequential,
	                                            pAllocationCallbacks);
}

drwav_bool32
drwav_init_file_write(drwav *pWav, const char *filename,
                      const drwav_data_format *pFormat,
                      const drwav_allocation_callbacks *pAllocationCallbacks)
{
	return drwav_init_file_write__internal(pWav, filename, pFormat, 0,
	                                       DRWAV_FALSE, pAllocationCallbacks);
}

drwav_bool32 drwav_init_file_write_sequential(
    drwav *pWav, const char *filename, const drwav_data_format *pFormat,
    drwav_uint64_t totalSampleCount,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	return drwav_init_file_write__internal(pWav, filename, pFormat,
	                                       totalSampleCount, DRWAV_TRUE,
	                                       pAllocationCallbacks);
}

drwav_bool32 drwav_init_file_write_sequential_pcm_frames(
    drwav *pWav, const char *filename, const drwav_data_format *pFormat,
    drwav_uint64_t totalPCMFrameCount,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (pFormat == NULL)
	{
		return DRWAV_FALSE;
	}

	return drwav_init_file_write_sequential(
	    pWav, filename, pFormat, totalPCMFrameCount * pFormat->channels,
	    pAllocationCallbacks);
}

drwav_bool32
drwav_init_file_write_w(drwav *pWav, const wchar_t *filename,
                        const drwav_data_format *pFormat,
                        const drwav_allocation_callbacks *pAllocationCallbacks)
{
	return drwav_init_file_write_w__internal(pWav, filename, pFormat, 0,
	                                         DRWAV_FALSE, pAllocationCallbacks);
}

drwav_bool32 drwav_init_file_write_sequential_w(
    drwav *pWav, const wchar_t *filename, const drwav_data_format *pFormat,
    drwav_uint64_t totalSampleCount,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	return drwav_init_file_write_w__internal(pWav, filename, pFormat,
	                                         totalSampleCount, DRWAV_TRUE,
	                                         pAllocationCallbacks);
}

drwav_bool32 drwav_init_file_write_sequential_pcm_frames_w(
    drwav *pWav, const wchar_t *filename, const drwav_data_format *pFormat,
    drwav_uint64_t totalPCMFrameCount,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (pFormat == NULL)
	{
		return DRWAV_FALSE;
	}

	return drwav_init_file_write_sequential_w(
	    pWav, filename, pFormat, totalPCMFrameCount * pFormat->channels,
	    pAllocationCallbacks);
}
#endif /* DR_WAV_NO_STDIO */

drwav_bool32
drwav_init_memory(drwav *pWav, const void *data, size_t dataSize,
                  const drwav_allocation_callbacks *pAllocationCallbacks)
{
	return drwav_init_memory_ex(pWav, data, dataSize, NULL, NULL, 0,
	                            pAllocationCallbacks);
}

drwav_bool32
drwav_init_memory_ex(drwav *pWav, const void *data, size_t dataSize,
                     drwav_chunk_proc onChunk, void *pChunkUserData,
                     drwav_uint32_t flags,
                     const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (data == NULL || dataSize == 0)
	{
		return DRWAV_FALSE;
	}

	if (!drwav_preinit(pWav, drwav__on_read_memory, drwav__on_seek_memory, pWav,
	                   pAllocationCallbacks))
	{
		return DRWAV_FALSE;
	}

	pWav->memoryStream.data = (const unsigned char *)data;
	pWav->memoryStream.dataSize = dataSize;
	pWav->memoryStream.currentReadPos = 0;

	return drwav_init__internal(pWav, onChunk, pChunkUserData, flags);
}

drwav_bool32 drwav_init_memory_write__internal(
    drwav *pWav, void **ppData, size_t *pDataSize,
    const drwav_data_format *pFormat, drwav_uint64_t totalSampleCount,
    drwav_bool32 isSequential,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (ppData == NULL || pDataSize == NULL)
	{
		return DRWAV_FALSE;
	}

	*ppData = NULL; /* Important because we're using realloc()! */
	*pDataSize = 0;

	if (!drwav_preinit_write(
	        pWav, pFormat, isSequential, drwav__on_write_memory,
	        drwav__on_seek_memory_write, pWav, pAllocationCallbacks))
	{
		return DRWAV_FALSE;
	}

	pWav->memoryStreamWrite.ppData = ppData;
	pWav->memoryStreamWrite.pDataSize = pDataSize;
	pWav->memoryStreamWrite.dataSize = 0;
	pWav->memoryStreamWrite.dataCapacity = 0;
	pWav->memoryStreamWrite.currentWritePos = 0;

	return drwav_init_write__internal(pWav, pFormat, totalSampleCount);
}

drwav_bool32
drwav_init_memory_write(drwav *pWav, void **ppData, size_t *pDataSize,
                        const drwav_data_format *pFormat,
                        const drwav_allocation_callbacks *pAllocationCallbacks)
{
	return drwav_init_memory_write__internal(
	    pWav, ppData, pDataSize, pFormat, 0, DRWAV_FALSE, pAllocationCallbacks);
}

drwav_bool32 drwav_init_memory_write_sequential(
    drwav *pWav, void **ppData, size_t *pDataSize,
    const drwav_data_format *pFormat, drwav_uint64_t totalSampleCount,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	return drwav_init_memory_write__internal(pWav, ppData, pDataSize, pFormat,
	                                         totalSampleCount, DRWAV_TRUE,
	                                         pAllocationCallbacks);
}

drwav_bool32 drwav_init_memory_write_sequential_pcm_frames(
    drwav *pWav, void **ppData, size_t *pDataSize,
    const drwav_data_format *pFormat, drwav_uint64_t totalPCMFrameCount,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (pFormat == NULL)
	{
		return DRWAV_FALSE;
	}

	return drwav_init_memory_write_sequential(
	    pWav, ppData, pDataSize, pFormat,
	    totalPCMFrameCount * pFormat->channels, pAllocationCallbacks);
}
