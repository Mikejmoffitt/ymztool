#include "3rdparty/dr_wav/dr_wav_fileops.h"

FILE *drwav_fopen(const char *filePath, const char *openMode)
{
	FILE *pFile;
#if defined(_MSC_VER) && _MSC_VER >= 1400
	if (fopen_s(&pFile, filePath, openMode) != 0)
	{
		return NULL;
	}
#else
	pFile = fopen(filePath, openMode);
	if (pFile == NULL)
	{
		return NULL;
	}
#endif

	return pFile;
}

FILE *drwav_wfopen(const wchar_t *pFilePath, const wchar_t *pOpenMode,
                   const drwav_allocation_callbacks *pAllocationCallbacks)
{
	FILE *pFile;

#if defined(_WIN32)
	(void)pAllocationCallbacks;
#if defined(_MSC_VER) && _MSC_VER >= 1400
	if (_wfopen_s(&pFile, pFilePath, pOpenMode) != 0)
	{
		return NULL;
	}
#else
	pFile = _wfopen(pFilePath, pOpenMode);
	if (pFile == NULL)
	{
		return NULL;
	}
#endif
#else
	/*
	Use fopen() on anything other than Windows. Requires a conversion. This is
	annoying because fopen() is locale specific. The only real way I can think
	of to do this is with wcsrtombs(). Note that wcstombs() is apparently not
	thread-safe because it uses a static global mbstate_t object for maintaining
	state. I've checked this with -std=c89 and it works, but if somebody get's a
	compiler error I'll look into improving compatibility.
	*/
	{
		mbstate_t mbs;
		size_t lenMB;
		const wchar_t *pFilePathTemp = pFilePath;
		char *pFilePathMB = NULL;
		const wchar_t *pOpenModeMBTemp = pOpenMode;
		char pOpenModeMB[16];
		drwav_allocation_callbacks allocationCallbacks;

		allocationCallbacks =
		    drwav_copy_allocation_callbacks_or_defaults(pAllocationCallbacks);

		/* Get the length first. */
		DRWAV_ZERO_MEMORY(&mbs, sizeof(mbs));
		lenMB = wcsrtombs(NULL, &pFilePathTemp, 0, &mbs);
		if (lenMB == (size_t)-1)
		{
			return NULL;
		}

		pFilePathMB = (char *)drwav__malloc_from_callbacks(
		    lenMB + 1, &allocationCallbacks);
		if (pFilePathMB == NULL)
		{
			return NULL;
		}

		pFilePathTemp = pFilePath;
		DRWAV_ZERO_MEMORY(&mbs, sizeof(mbs));
		wcsrtombs(pFilePathMB, &pFilePathTemp, lenMB + 1, &mbs);

		DRWAV_ZERO_MEMORY(&mbs, sizeof(mbs));
		wcsrtombs(pOpenModeMB, &pOpenModeMBTemp, sizeof(pOpenModeMB), &mbs);

		pFile = fopen(pFilePathMB, pOpenModeMB);

		drwav__free_from_callbacks(pFilePathMB, &allocationCallbacks);
	}
#endif

	return pFile;
}

size_t drwav__on_read(drwav_read_proc onRead, void *pUserData, void *pBufferOut,
                      size_t bytesToRead, drwav_uint64_t *pCursor)
{
	size_t bytesRead;

	DRWAV_ASSERT(onRead != NULL);
	DRWAV_ASSERT(pCursor != NULL);

	bytesRead = onRead(pUserData, pBufferOut, bytesToRead);
	*pCursor += bytesRead;
	return bytesRead;
}

drwav_bool32 drwav__on_seek(drwav_seek_proc onSeek, void *pUserData, int offset,
                            drwav_seek_origin origin, drwav_uint64_t *pCursor)
{
	DRWAV_ASSERT(onSeek != NULL);
	DRWAV_ASSERT(pCursor != NULL);

	if (!onSeek(pUserData, offset, origin))
	{
		return DRWAV_FALSE;
	}

	if (origin == drwav_seek_origin_start)
	{
		*pCursor = offset;
	}
	else
	{
		*pCursor += offset;
	}

	return DRWAV_TRUE;
}

size_t drwav__on_read_stdio(void *pUserData, void *pBufferOut,
                            size_t bytesToRead)
{
	return fread(pBufferOut, 1, bytesToRead, (FILE *)pUserData);
}

size_t drwav__on_write_stdio(void *pUserData, const void *pData,
                             size_t bytesToWrite)
{
	return fwrite(pData, 1, bytesToWrite, (FILE *)pUserData);
}

drwav_bool32 drwav__read_fmt(drwav_read_proc onRead, drwav_seek_proc onSeek,
                             void *pUserData, drwav_container container,
                             drwav_uint64_t *pRunningBytesReadOut,
                             drwav_fmt *fmtOut)
{
	drwav_chunk_header header;
	unsigned char fmt[16];

	if (drwav__read_chunk_header(onRead, pUserData, container,
	                             pRunningBytesReadOut,
	                             &header) != DRWAV_SUCCESS)
	{
		return DRWAV_FALSE;
	}

	/* Skip non-fmt chunks. */
	while ((container == drwav_container_riff &&
	        !drwav__fourcc_equal(header.id.fourcc, "fmt ")) ||
	       (container == drwav_container_w64 &&
	        !drwav__guid_equal(header.id.guid, drwavGUID_W64_FMT)))
	{
		if (!drwav__seek_forward(
		        onSeek, header.sizeInBytes + header.paddingSize, pUserData))
		{
			return DRWAV_FALSE;
		}
		*pRunningBytesReadOut += header.sizeInBytes + header.paddingSize;

		/* Try the next header. */
		if (drwav__read_chunk_header(onRead, pUserData, container,
		                             pRunningBytesReadOut,
		                             &header) != DRWAV_SUCCESS)
		{
			return DRWAV_FALSE;
		}
	}

	/* Validation. */
	if (container == drwav_container_riff)
	{
		if (!drwav__fourcc_equal(header.id.fourcc, "fmt "))
		{
			return DRWAV_FALSE;
		}
	}
	else
	{
		if (!drwav__guid_equal(header.id.guid, drwavGUID_W64_FMT))
		{
			return DRWAV_FALSE;
		}
	}

	if (onRead(pUserData, fmt, sizeof(fmt)) != sizeof(fmt))
	{
		return DRWAV_FALSE;
	}
	*pRunningBytesReadOut += sizeof(fmt);

	fmtOut->formatTag = drwav__bytes_to_u16(fmt + 0);
	fmtOut->channels = drwav__bytes_to_u16(fmt + 2);
	fmtOut->sampleRate = drwav__bytes_to_u32(fmt + 4);
	fmtOut->avgBytesPerSec = drwav__bytes_to_u32(fmt + 8);
	fmtOut->blockAlign = drwav__bytes_to_u16(fmt + 12);
	fmtOut->bitsPerSample = drwav__bytes_to_u16(fmt + 14);

	fmtOut->extendedSize = 0;
	fmtOut->validBitsPerSample = 0;
	fmtOut->channelMask = 0;
	memset(fmtOut->subFormat, 0, sizeof(fmtOut->subFormat));

	if (header.sizeInBytes > 16)
	{
		unsigned char fmt_cbSize[2];
		int bytesReadSoFar = 0;

		if (onRead(pUserData, fmt_cbSize, sizeof(fmt_cbSize)) !=
		    sizeof(fmt_cbSize))
		{
			return DRWAV_FALSE; /* Expecting more data. */
		}
		*pRunningBytesReadOut += sizeof(fmt_cbSize);

		bytesReadSoFar = 18;

		fmtOut->extendedSize = drwav__bytes_to_u16(fmt_cbSize);
		if (fmtOut->extendedSize > 0)
		{
			/* Simple validation. */
			if (fmtOut->formatTag == DR_WAVE_FORMAT_EXTENSIBLE)
			{
				if (fmtOut->extendedSize != 22)
				{
					return DRWAV_FALSE;
				}
			}

			if (fmtOut->formatTag == DR_WAVE_FORMAT_EXTENSIBLE)
			{
				unsigned char fmtext[22];
				if (onRead(pUserData, fmtext, fmtOut->extendedSize) !=
				    fmtOut->extendedSize)
				{
					return DRWAV_FALSE; /* Expecting more data. */
				}

				fmtOut->validBitsPerSample = drwav__bytes_to_u16(fmtext + 0);
				fmtOut->channelMask = drwav__bytes_to_u32(fmtext + 2);
				drwav__bytes_to_guid(fmtext + 6, fmtOut->subFormat);
			}
			else
			{
				if (!onSeek(pUserData, fmtOut->extendedSize,
				            drwav_seek_origin_current))
				{
					return DRWAV_FALSE;
				}
			}
			*pRunningBytesReadOut += fmtOut->extendedSize;

			bytesReadSoFar += fmtOut->extendedSize;
		}

		/* Seek past any leftover bytes. For w64 the leftover will be defined
		 * based on the chunk size. */
		if (!onSeek(pUserData, (int)(header.sizeInBytes - bytesReadSoFar),
		            drwav_seek_origin_current))
		{
			return DRWAV_FALSE;
		}
		*pRunningBytesReadOut += (header.sizeInBytes - bytesReadSoFar);
	}

	if (header.paddingSize > 0)
	{
		if (!onSeek(pUserData, header.paddingSize, drwav_seek_origin_current))
		{
			return DRWAV_FALSE;
		}
		*pRunningBytesReadOut += header.paddingSize;
	}

	return DRWAV_TRUE;
}

drwav_result drwav__read_chunk_header(drwav_read_proc onRead, void *pUserData,
                                      drwav_container container,
                                      drwav_uint64_t *pRunningBytesReadOut,
                                      drwav_chunk_header *pHeaderOut)
{
	if (container == drwav_container_riff)
	{
		unsigned char sizeInBytes[4];

		if (onRead(pUserData, pHeaderOut->id.fourcc, 4) != 4)
		{
			return DRWAV_EOF;
		}

		if (onRead(pUserData, sizeInBytes, 4) != 4)
		{
			return DRWAV_INVALID_FILE;
		}

		pHeaderOut->sizeInBytes = drwav__bytes_to_u32(sizeInBytes);
		pHeaderOut->paddingSize =
		    drwav__chunk_padding_size_riff(pHeaderOut->sizeInBytes);
		*pRunningBytesReadOut += 8;
	}
	else
	{
		unsigned char sizeInBytes[8];

		if (onRead(pUserData, pHeaderOut->id.guid, 16) != 16)
		{
			return DRWAV_EOF;
		}

		if (onRead(pUserData, sizeInBytes, 8) != 8)
		{
			return DRWAV_INVALID_FILE;
		}

		pHeaderOut->sizeInBytes = drwav__bytes_to_u64(sizeInBytes) -
		                          24; /* <-- Subtract 24 because w64 includes
		                                 the size of the header. */
		pHeaderOut->paddingSize =
		    drwav__chunk_padding_size_w64(pHeaderOut->sizeInBytes);
		*pRunningBytesReadOut += 24;
	}

	return DRWAV_SUCCESS;
}

drwav_bool32 drwav__seek_forward(drwav_seek_proc onSeek, drwav_uint64_t offset,
                                 void *pUserData)
{
	drwav_uint64_t bytesRemainingToSeek = offset;
	while (bytesRemainingToSeek > 0)
	{
		if (bytesRemainingToSeek > 0x7FFFFFFF)
		{
			if (!onSeek(pUserData, 0x7FFFFFFF, drwav_seek_origin_current))
			{
				return DRWAV_FALSE;
			}
			bytesRemainingToSeek -= 0x7FFFFFFF;
		}
		else
		{
			if (!onSeek(pUserData, (int)bytesRemainingToSeek,
			            drwav_seek_origin_current))
			{
				return DRWAV_FALSE;
			}
			bytesRemainingToSeek = 0;
		}
	}

	return DRWAV_TRUE;
}

drwav_bool32 drwav__seek_from_start(drwav_seek_proc onSeek,
                                    drwav_uint64_t offset, void *pUserData)
{
	if (offset <= 0x7FFFFFFF)
	{
		return onSeek(pUserData, (int)offset, drwav_seek_origin_start);
	}

	/* Larger than 32-bit seek. */
	if (!onSeek(pUserData, 0x7FFFFFFF, drwav_seek_origin_start))
	{
		return DRWAV_FALSE;
	}
	offset -= 0x7FFFFFFF;

	for (;;)
	{
		if (offset <= 0x7FFFFFFF)
		{
			return onSeek(pUserData, (int)offset, drwav_seek_origin_current);
		}

		if (!onSeek(pUserData, 0x7FFFFFFF, drwav_seek_origin_current))
		{
			return DRWAV_FALSE;
		}
		offset -= 0x7FFFFFFF;
	}

	/* Should never get here. */
	/*return DRWAV_TRUE; */
}

size_t drwav__on_read_memory(void *pUserData, void *pBufferOut,
                             size_t bytesToRead)
{
	drwav *pWav = (drwav *)pUserData;
	size_t bytesRemaining;

	DRWAV_ASSERT(pWav != NULL);
	DRWAV_ASSERT(pWav->memoryStream.dataSize >=
	             pWav->memoryStream.currentReadPos);

	bytesRemaining =
	    pWav->memoryStream.dataSize - pWav->memoryStream.currentReadPos;
	if (bytesToRead > bytesRemaining)
	{
		bytesToRead = bytesRemaining;
	}

	if (bytesToRead > 0)
	{
		DRWAV_COPY_MEMORY(pBufferOut,
		                  pWav->memoryStream.data +
		                      pWav->memoryStream.currentReadPos,
		                  bytesToRead);
		pWav->memoryStream.currentReadPos += bytesToRead;
	}

	return bytesToRead;
}

drwav_bool32 drwav__on_seek_memory(void *pUserData, int offset,
                                   drwav_seek_origin origin)
{
	drwav *pWav = (drwav *)pUserData;
	DRWAV_ASSERT(pWav != NULL);

	if (origin == drwav_seek_origin_current)
	{
		if (offset > 0)
		{
			if (pWav->memoryStream.currentReadPos + offset >
			    pWav->memoryStream.dataSize)
			{
				return DRWAV_FALSE; /* Trying to seek too far forward. */
			}
		}
		else
		{
			if (pWav->memoryStream.currentReadPos < (size_t)-offset)
			{
				return DRWAV_FALSE; /* Trying to seek too far backwards. */
			}
		}

		/* This will never underflow thanks to the clamps above. */
		pWav->memoryStream.currentReadPos += offset;
	}
	else
	{
		if ((drwav_uint32_t)offset <= pWav->memoryStream.dataSize)
		{
			pWav->memoryStream.currentReadPos = offset;
		}
		else
		{
			return DRWAV_FALSE; /* Trying to seek too far forward. */
		}
	}

	return DRWAV_TRUE;
}

size_t drwav__on_write_memory(void *pUserData, const void *pDataIn,
                              size_t bytesToWrite)
{
	drwav *pWav = (drwav *)pUserData;
	size_t bytesRemaining;

	DRWAV_ASSERT(pWav != NULL);
	DRWAV_ASSERT(pWav->memoryStreamWrite.dataCapacity >=
	             pWav->memoryStreamWrite.currentWritePos);

	bytesRemaining = pWav->memoryStreamWrite.dataCapacity -
	                 pWav->memoryStreamWrite.currentWritePos;
	if (bytesRemaining < bytesToWrite)
	{
		/* Need to reallocate. */
		void *pNewData;
		size_t newDataCapacity = (pWav->memoryStreamWrite.dataCapacity == 0)
		                             ? 256
		                             : pWav->memoryStreamWrite.dataCapacity * 2;

		/* If doubling wasn't enough, just make it the minimum required size to
		 * write the data. */
		if ((newDataCapacity - pWav->memoryStreamWrite.currentWritePos) <
		    bytesToWrite)
		{
			newDataCapacity =
			    pWav->memoryStreamWrite.currentWritePos + bytesToWrite;
		}

		pNewData = drwav__realloc_from_callbacks(
		    *pWav->memoryStreamWrite.ppData, newDataCapacity,
		    pWav->memoryStreamWrite.dataCapacity, &pWav->allocationCallbacks);
		if (pNewData == NULL)
		{
			return 0;
		}

		*pWav->memoryStreamWrite.ppData = pNewData;
		pWav->memoryStreamWrite.dataCapacity = newDataCapacity;
	}

	DRWAV_COPY_MEMORY(((drwav_uint8_t *)(*pWav->memoryStreamWrite.ppData)) +
	                      pWav->memoryStreamWrite.currentWritePos,
	                  pDataIn, bytesToWrite);

	pWav->memoryStreamWrite.currentWritePos += bytesToWrite;
	if (pWav->memoryStreamWrite.dataSize <
	    pWav->memoryStreamWrite.currentWritePos)
	{
		pWav->memoryStreamWrite.dataSize =
		    pWav->memoryStreamWrite.currentWritePos;
	}

	*pWav->memoryStreamWrite.pDataSize = pWav->memoryStreamWrite.dataSize;

	return bytesToWrite;
}

drwav_bool32 drwav__on_seek_memory_write(void *pUserData, int offset,
                                         drwav_seek_origin origin)
{
	drwav *pWav = (drwav *)pUserData;
	DRWAV_ASSERT(pWav != NULL);

	if (origin == drwav_seek_origin_current)
	{
		if (offset > 0)
		{
			if (pWav->memoryStreamWrite.currentWritePos + offset >
			    pWav->memoryStreamWrite.dataSize)
			{
				offset = (int)(pWav->memoryStreamWrite.dataSize -
				               pWav->memoryStreamWrite
				                   .currentWritePos); /* Trying to seek too far
				                                         forward. */
			}
		}
		else
		{
			if (pWav->memoryStreamWrite.currentWritePos < (size_t)-offset)
			{
				offset = -(int)pWav->memoryStreamWrite
				              .currentWritePos; /* Trying to seek too far
				                                   backwards. */
			}
		}

		/* This will never underflow thanks to the clamps above. */
		pWav->memoryStreamWrite.currentWritePos += offset;
	}
	else
	{
		if ((drwav_uint32_t)offset <= pWav->memoryStreamWrite.dataSize)
		{
			pWav->memoryStreamWrite.currentWritePos = offset;
		}
		else
		{
			pWav->memoryStreamWrite.currentWritePos =
			    pWav->memoryStreamWrite
			        .dataSize; /* Trying to seek too far forward. */
		}
	}

	return DRWAV_TRUE;
}
