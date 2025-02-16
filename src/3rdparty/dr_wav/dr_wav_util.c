#include "3rdparty/dr_wav/dr_wav_util.h"
#include "3rdparty/dr_wav/dr_wav_types.h"

unsigned int drwav__chunk_padding_size_riff(drwav_uint64_t chunkSize)
{
	return (unsigned int)(chunkSize % 2);
}

unsigned int drwav__chunk_padding_size_w64(drwav_uint64_t chunkSize)
{
	return (unsigned int)(chunkSize % 8);
}

void *drwav__malloc_from_callbacks(
    size_t sz, const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (pAllocationCallbacks == NULL)
	{
		return NULL;
	}

	if (pAllocationCallbacks->onMalloc != NULL)
	{
		return pAllocationCallbacks->onMalloc(sz,
		                                      pAllocationCallbacks->pUserData);
	}

	/* Try using realloc(). */
	if (pAllocationCallbacks->onRealloc != NULL)
	{
		return pAllocationCallbacks->onRealloc(NULL, sz,
		                                       pAllocationCallbacks->pUserData);
	}

	return NULL;
}

void *drwav__realloc_from_callbacks(
    void *p, size_t szNew, size_t szOld,
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (pAllocationCallbacks == NULL)
	{
		return NULL;
	}

	if (pAllocationCallbacks->onRealloc != NULL)
	{
		return pAllocationCallbacks->onRealloc(p, szNew,
		                                       pAllocationCallbacks->pUserData);
	}

	/* Try emulating realloc() in terms of malloc()/free(). */
	if (pAllocationCallbacks->onMalloc != NULL &&
	    pAllocationCallbacks->onFree != NULL)
	{
		void *p2;

		p2 = pAllocationCallbacks->onMalloc(szNew,
		                                    pAllocationCallbacks->pUserData);
		if (p2 == NULL)
		{
			return NULL;
		}

		DRWAV_COPY_MEMORY(p2, p, szOld);
		pAllocationCallbacks->onFree(p, pAllocationCallbacks->pUserData);

		return p2;
	}

	return NULL;
}

void drwav__free_from_callbacks(
    void *p, const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (p == NULL || pAllocationCallbacks == NULL)
	{
		return;
	}

	if (pAllocationCallbacks->onFree != NULL)
	{
		pAllocationCallbacks->onFree(p, pAllocationCallbacks->pUserData);
	}
}

drwav_allocation_callbacks drwav_copy_allocation_callbacks_or_defaults(
    const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (pAllocationCallbacks != NULL)
	{
		/* Copy. */
		return *pAllocationCallbacks;
	}
	else
	{
		/* Defaults. */
		drwav_allocation_callbacks allocationCallbacks;
		allocationCallbacks.pUserData = NULL;
		allocationCallbacks.onMalloc = drwav__malloc_default;
		allocationCallbacks.onRealloc = drwav__realloc_default;
		allocationCallbacks.onFree = drwav__free_default;
		return allocationCallbacks;
	}
}

drwav_uint32_t drwav_get_bytes_per_pcm_frame(drwav *pWav)
{
	/*
	The bytes per frame is a bit ambiguous. It can be either be based on the
	bits per sample, or the block align. The way I'm doing it here is that if
	the bits per sample is a multiple of 8, use floor(bitsPerSample*channels/8),
	otherwise fall back to the block align.
	*/
	if ((pWav->bitsPerSample & 0x7) == 0)
	{
		/* Bits per sample is a multiple of 8. */
		return (pWav->bitsPerSample * pWav->fmt.channels) >> 3;
	}
	else
	{
		return pWav->fmt.blockAlign;
	}
}

drwav_uint32_t drwav__riff_chunk_size_riff(drwav_uint64_t dataChunkSize)
{
	drwav_uint32_t dataSubchunkPaddingSize =
	    drwav__chunk_padding_size_riff(dataChunkSize);

	if (dataChunkSize <= (0xFFFFFFFFUL - 36 - dataSubchunkPaddingSize))
	{
		return 36 + (drwav_uint32_t)(dataChunkSize + dataSubchunkPaddingSize);
	}
	else
	{
		return 0xFFFFFFFF;
	}
}

drwav_uint32_t drwav__data_chunk_size_riff(drwav_uint64_t dataChunkSize)
{
	if (dataChunkSize <= 0xFFFFFFFFUL)
	{
		return (drwav_uint32_t)dataChunkSize;
	}
	else
	{
		return 0xFFFFFFFFUL;
	}
}

drwav_uint64_t drwav__riff_chunk_size_w64(drwav_uint64_t dataChunkSize)
{
	drwav_uint64_t dataSubchunkPaddingSize =
	    drwav__chunk_padding_size_w64(dataChunkSize);

	return 80 + 24 + dataChunkSize +
	       dataSubchunkPaddingSize; /* +24 because W64 includes the size of the
	                                   GUID and size fields. */
}

drwav_uint64_t drwav__data_chunk_size_w64(drwav_uint64_t dataChunkSize)
{
	return 24 + dataChunkSize; /* +24 because W64 includes the size of the GUID
	                              and size fields. */
}

drwav_uint64_t drwav_target_write_size_bytes(drwav_data_format const *format,
                                             drwav_uint64_t totalSampleCount)
{
	drwav_uint64_t targetDataSizeBytes =
	    (totalSampleCount * format->channels * format->bitsPerSample / 8);
	drwav_uint64_t riffChunkSizeBytes;
	drwav_uint64_t fileSizeBytes;

	if (format->container == drwav_container_riff)
	{
		riffChunkSizeBytes = drwav__riff_chunk_size_riff(targetDataSizeBytes);
		fileSizeBytes =
		    (8 + riffChunkSizeBytes); /* +8 because WAV doesn't include the size
		                                 of the ChunkID and ChunkSize fields. */
	}
	else
	{
		riffChunkSizeBytes = drwav__riff_chunk_size_w64(targetDataSizeBytes);
		fileSizeBytes = riffChunkSizeBytes;
	}

	return fileSizeBytes;
}

size_t drwav_read_raw(drwav *pWav, size_t bytesToRead, void *pBufferOut)
{
	size_t bytesRead;

	if (pWav == NULL || bytesToRead == 0 || pBufferOut == NULL)
	{
		return 0;
	}

	if (bytesToRead > pWav->bytesRemaining)
	{
		bytesToRead = (size_t)pWav->bytesRemaining;
	}

	bytesRead = pWav->onRead(pWav->pUserData, pBufferOut, bytesToRead);

	pWav->bytesRemaining -= bytesRead;
	return bytesRead;
}

void drwav_free(void *p, const drwav_allocation_callbacks *pAllocationCallbacks)
{
	if (pAllocationCallbacks != NULL)
	{
		drwav__free_from_callbacks(p, pAllocationCallbacks);
	}
	else
	{
		drwav__free_default(p, NULL);
	}
}
