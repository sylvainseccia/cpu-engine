#include "pch.h"

cpu_sound::cpu_sound()
{
	data = nullptr;
	pVoice = nullptr;
	Close();
}

cpu_sound::~cpu_sound()
{
	Close();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool cpu_sound::LoadWave(byte* sourceBuffer, int sourceSize)
{
	if ( sourceBuffer==nullptr || sourceSize<12 )
		return false;

	Close();

	// RIFF / WAVE
	if ( memcmp(sourceBuffer, "RIFF", 4)!=0 || memcmp(sourceBuffer+8, "WAVE", 4)!=0 )
		return false;

	DWORD riffSize = 0;
	memcpy(&riffSize, sourceBuffer+4, 4);
	if ( riffSize<4 || (size_t)riffSize>(size_t)sourceSize-8 )
		return false;

	const size_t riffEnd = 8 + (size_t)riffSize;

	// Search chunks
	WAVEFORMATEXTENSIBLE wave = {};
	bool hasFormat = false;
	const byte* audioData = nullptr;
	DWORD audioSize = 0;
	size_t pos = 12;
	while ( pos+8<=riffEnd )
	{
		const byte* chunk = sourceBuffer + pos;
		DWORD chunkSize = 0;
		memcpy(&chunkSize, chunk+4, 4);
		size_t chunkStart = pos + 8;

		// Check boundaries
		if ( (size_t)chunkSize>riffEnd-chunkStart )
			return false;

		if ( memcmp(chunk, "fmt ", 4)==0 && hasFormat==false )
		{
			if ( chunkSize<16 )
				return false;

			size_t copySize = std::min((size_t)chunkSize, sizeof(WAVEFORMATEXTENSIBLE));
			memcpy(&wave, sourceBuffer+chunkStart, copySize);
			hasFormat = true;

			// Extended format
			if ( wave.Format.wFormatTag==WAVE_FORMAT_EXTENSIBLE )
			{
				if ( chunkSize<40 || wave.Format.cbSize!=22 )
					return false;
			}
			else if ( wave.Format.wFormatTag==WAVE_FORMAT_PCM || wave.Format.wFormatTag==WAVE_FORMAT_IEEE_FLOAT )
				wave.Format.cbSize = 0;
			else
				return false;
		}
		else if ( memcmp(chunk, "data", 4)==0 && audioData==nullptr )
		{
			audioData = sourceBuffer+chunkStart;
			audioSize = chunkSize;
		}
		if ( hasFormat && audioData )
			break;

		// RIFF chunks are aligned to 2 bytes
		pos = chunkStart + (size_t)chunkSize;
		if ( chunkSize & 1 )
		{
			if ( pos>=riffEnd )
				return false;
			++pos;
		}
	}

	if ( hasFormat==false || audioData==nullptr || audioSize==0 )
		return false;

	// Validate audio format
	if ( wave.Format.nChannels==0 || wave.Format.nSamplesPerSec==0 || wave.Format.nBlockAlign==0 )
		return false;

	if ( audioSize%wave.Format.nBlockAlign!=0 )
		return false;

	// Keep basic format information
	format = wave.Format;

	// Create Voice
	HRESULT hr = cpuAudio.pAudio->CreateSourceVoice(&pVoice, &wave.Format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr);
	if ( FAILED(hr) )
		return false;

	// Copy audio data
	size = audioSize;
	data = new byte[size];
	memcpy(data, audioData, size);
	ZeroMemory(&buffer, sizeof(XAUDIO2_BUFFER));
	buffer.AudioBytes = size;
	buffer.pAudioData = data;
	loaded = true;
	return true;
}

bool cpu_sound::LoadRaw(byte* sourceBuffer, int sourceSize)
{
	if ( sourceBuffer==nullptr || sourceSize<=0 )
		return false;

	Close();

	// Format
	format.cbSize = sizeof(WAVEFORMATEX);
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.wBitsPerSample = 16;
	format.nChannels = (WORD)2;
	format.nSamplesPerSec = 44100;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	// Create Voice
	HRESULT hr = cpuAudio.pAudio->CreateSourceVoice(&pVoice, &format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr);
	if ( FAILED(hr) )
		return false;

	// Buffer
	size = sourceSize;
	data = new byte[size];
	memcpy(data, sourceBuffer, size);
	ZeroMemory(&buffer, sizeof(XAUDIO2_BUFFER));
	buffer.AudioBytes = size;
	buffer.pAudioData = data;
	//m_buffer.Flags = XAUDIO2_END_OF_STREAM;
	loaded = true;
	return true;
}

bool cpu_sound::LoadMp3(byte* sourceBuffer, int sourceSize)
{
	if ( sourceBuffer==nullptr || sourceSize<=0 )
		return false;

	// Reset
	Close();

	// Source
	DWORD mp3InputBufferSize = 0;
	mp3InputBufferSize = sourceSize;
	if ( mp3InputBufferSize==0 )
		return false;
	BYTE* mp3InputBuffer = nullptr;
	mp3InputBuffer = sourceBuffer;

	// Constants
	const DWORD mp3BlockSize = 522;

	// Define output format
	WAVEFORMATEX pcmFormat = {
		WAVE_FORMAT_PCM,					// WORD		wFormatTag;         format type
		2,									// WORD		nChannels;          number of channels (i.e. mono, stereo...)
		44100,								// DWORD	nSamplesPerSec;     sample rate
		4 * 44100,							// DWORD	nAvgBytesPerSec;    for buffer estimation
		4,									// WORD		nBlockAlign;        block size of data
		16,									// WORD		wBitsPerSample;     number of bits per sample of mono data
		0,									// WORD		cbSize;             the count in bytes of the size of
	};

	// Define input format
	MPEGLAYER3WAVEFORMAT mp3Format = {
		{
			WAVE_FORMAT_MPEGLAYER3,			// WORD		wFormatTag;         format type
			2,								// WORD		nChannels;          number of channels (i.e. mono, stereo...)
			44100,							// DWORD	nSamplesPerSec;     sample rate
			128 * (1024/8),					// DWORD	nAvgBytesPerSec;    not really used but must be one of 64, 96, 112, 128, 160kbps
			1,								// WORD		nBlockAlign;        block size of data
			0,								// WORD		wBitsPerSample;     number of bits per sample of mono data
			MPEGLAYER3_WFX_EXTRA_BYTES,		// WORD		cbSize;        
		},
		MPEGLAYER3_ID_MPEG,					// WORD		wID;
		MPEGLAYER3_FLAG_PADDING_OFF,		// DWORD	fdwFlags;
		mp3BlockSize,						// WORD		nBlockSize;
		1,									// WORD		nFramesPerBlock;
		1393,								// WORD		nCodecDelay;
	};

	// Extract and verify mp3 info: duration, type = mp3, sampleRate = 44100, channels = 2
	///////////////////////////////////////////////////////////////////////////////////////

	// Create SyncReader
	IWMSyncReader* pSyncReader;
	if ( WMCreateSyncReader(NULL, WMT_RIGHT_PLAYBACK, &pSyncReader)!=S_OK )
		return false;

	// Alloc With global and create IStream
	HGLOBAL mp3HGlobal = GlobalAlloc(GPTR, mp3InputBufferSize);
	void* mp3HGlobalBuffer = GlobalLock(mp3HGlobal);
	memcpy(mp3HGlobalBuffer, mp3InputBuffer, mp3InputBufferSize);
	GlobalUnlock(mp3HGlobal);
	IStream* pStream;
	if ( CreateStreamOnHGlobal(mp3HGlobal, FALSE, &pStream)!=S_OK )
	{
		GlobalFree(mp3HGlobal);
		pSyncReader->Release();
		return false;
	}

	// Open MP3 Stream
	if ( pSyncReader->OpenStream(pStream)!=S_OK )
	{
		pStream->Release();
		GlobalFree(mp3HGlobal);
		pSyncReader->Release();
		return false;
	}

	// Get HeaderInfo interface
	IWMHeaderInfo* pHeaderInfo;
	if ( pSyncReader->QueryInterface(&pHeaderInfo)!=S_OK )
	{
		pStream->Release();
		GlobalFree(mp3HGlobal);
		pSyncReader->Release();
		return false;
	}

	// Retrieve mp3 song duration in seconds
	QWORD durationInNano;
	WORD wmStreamNum = 0;
	WMT_ATTR_DATATYPE wmAttrDataType;
	WORD lengthDataType = sizeof(QWORD);
	if ( pHeaderInfo->GetAttributeByName(&wmStreamNum, L"Duration", &wmAttrDataType, (byte*)&durationInNano, &lengthDataType)!=S_OK )
	{
		pHeaderInfo->Release();
		pStream->Release();
		GlobalFree(mp3HGlobal);
		pSyncReader->Release();
		return false;
	}
	double durationInSecond = ((double)durationInNano)/10000000.0;
	//DWORD durationInSecondInt = (int)(durationInNano/10000000)+1;
	//DWORD durationInSecondInt = ((DWORD)durationInSecond)+1;
	//durationInSecondInt += pcmFormat.nBlockAlign - durationInSecondInt%pcmFormat.nBlockAlign;

	// Sequence of call to get the MediaType (WAVEFORMATEX for mp3 can then be extract from MediaType)
	IWMProfile* pProfile;
	if ( pSyncReader->QueryInterface(&pProfile)!=S_OK )
	{
		pHeaderInfo->Release();
		pStream->Release();
		GlobalFree(mp3HGlobal);
		pSyncReader->Release();
		return false;
	}
	IWMStreamConfig* pStreamConfig;
	if ( pProfile->GetStream(0, &pStreamConfig)!=S_OK )
	{
		pProfile->Release();
		pHeaderInfo->Release();
		pStream->Release();
		GlobalFree(mp3HGlobal);
		pSyncReader->Release();
		return false;
	}
	IWMMediaProps* pMediaProperties;
	if ( pStreamConfig->QueryInterface(&pMediaProperties)!=S_OK )
	{
		pStreamConfig->Release();
		pProfile->Release();
		pHeaderInfo->Release();
		pStream->Release();
		GlobalFree(mp3HGlobal);
		pSyncReader->Release();
		return false;
	}

	// Retrieve sizeof MediaType
	DWORD sizeMediaType;
	if ( pMediaProperties->GetMediaType(NULL, &sizeMediaType)!=S_OK )
	{
		pMediaProperties->Release();
		pStreamConfig->Release();
		pProfile->Release();
		pHeaderInfo->Release();
		pStream->Release();
		GlobalFree(mp3HGlobal);
		pSyncReader->Release();
		return false;
	}

	// Retrieve MediaType
	WM_MEDIA_TYPE* mediaType = (WM_MEDIA_TYPE*)LocalAlloc(LPTR, sizeMediaType);	
	if ( pMediaProperties->GetMediaType(mediaType, &sizeMediaType)!=S_OK )
	{
		pMediaProperties->Release();
		pStreamConfig->Release();
		pProfile->Release();
		pHeaderInfo->Release();
		pStream->Release();
		GlobalFree(mp3HGlobal);
		pSyncReader->Release();
		return false;
	}

	// Check that MediaType is audio
	if ( mediaType->majortype!=WMMEDIATYPE_Audio )
	{
		LocalFree(mediaType);
		pMediaProperties->Release();
		pStreamConfig->Release();
		pProfile->Release();
		pHeaderInfo->Release();
		pStream->Release();
		GlobalFree(mp3HGlobal);
		pSyncReader->Release();
		return false;
	}

	// Check that input is mp3
	WAVEFORMATEX* inputFormat = (WAVEFORMATEX*)mediaType->pbFormat;
	if ( inputFormat->wFormatTag!=WAVE_FORMAT_MPEGLAYER3 || inputFormat->nSamplesPerSec!=44100 || inputFormat->nChannels!=2 )
	{
		LocalFree(mediaType);
		pMediaProperties->Release();
		pStreamConfig->Release();
		pProfile->Release();
		pHeaderInfo->Release();
		pStream->Release();
		GlobalFree(mp3HGlobal);
		pSyncReader->Release();
		return false;
	}

	// Release COM interface
	LocalFree(mediaType);
	pMediaProperties->Release();
	pStreamConfig->Release();
	pProfile->Release();
	pHeaderInfo->Release();
	pSyncReader->Release();

	// Convert mp3 to pcm using acm driver
	///////////////////////////////////////

	// Get maximum FormatSize for all acm
	DWORD maxFormatSize = 0;
	if ( acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &maxFormatSize)!=S_OK )
	{
		pStream->Release();
		GlobalFree(mp3HGlobal);
		return false;
	}

	// Allocate PCM output sound buffer
	DWORD bufferLength = (DWORD)(durationInSecond * pcmFormat.nAvgBytesPerSec);
	bufferLength += pcmFormat.nBlockAlign - bufferLength%pcmFormat.nBlockAlign;

	byte* soundBuffer = new byte[bufferLength];

	// Open acm stream
	HACMSTREAM acmMp3stream = NULL;
	if ( acmStreamOpen(&acmMp3stream, NULL, (LPWAVEFORMATEX)&mp3Format, &pcmFormat, nullptr, 0, 0, 0)!=MMSYSERR_NOERROR )
	{
		delete [] soundBuffer;
		pStream->Release();
		GlobalFree(mp3HGlobal);
		return false;
	}

	// Determine output decompressed buffer size
	unsigned long rawbufsize = 0;
	if ( acmStreamSize(acmMp3stream, mp3BlockSize, &rawbufsize, ACM_STREAMSIZEF_SOURCE)!=S_OK || rawbufsize<=0 )
	{
		acmStreamClose(acmMp3stream, 0);
		delete [] soundBuffer;
		pStream->Release();
		GlobalFree(mp3HGlobal);
		return false;
	}

	// allocate our I/O buffers
	BYTE* mp3BlockBuffer = new BYTE[mp3BlockSize];
	LPBYTE rawbuf = (LPBYTE)LocalAlloc(LPTR, rawbufsize);

	// prepare the decoder
	ACMSTREAMHEADER mp3streamHead;
	ZeroMemory(&mp3streamHead, sizeof(ACMSTREAMHEADER));
	mp3streamHead.cbStruct = sizeof(ACMSTREAMHEADER);
	mp3streamHead.pbSrc = mp3BlockBuffer;
	mp3streamHead.cbSrcLength = mp3BlockSize;
	mp3streamHead.pbDst = rawbuf;
	mp3streamHead.cbDstLength = rawbufsize;
	if ( acmStreamPrepareHeader(acmMp3stream, &mp3streamHead, 0)!=S_OK )
	{
		delete [] mp3BlockBuffer;
		LocalFree(rawbuf);
		acmStreamClose(acmMp3stream, 0);
		delete [] soundBuffer;
		pStream->Release();
		GlobalFree(mp3HGlobal);
		return false;
	}
	BYTE* currentOutput = soundBuffer;
	DWORD totalDecompressedSize = 0;

	// Read
	ULARGE_INTEGER newPosition;
	LARGE_INTEGER seekValue;
	seekValue.LowPart = 0;
	seekValue.HighPart = 0;
	pStream->Seek(seekValue, STREAM_SEEK_SET, &newPosition);
	bool hasError = false;
	while ( true )
	{
		// suck in some MP3 data
		ULONG count;
		if ( pStream->Read(mp3BlockBuffer, mp3BlockSize, &count)!=S_OK )
		{
			hasError = true;
			break;
		}
		if ( count!=mp3BlockSize )
			break;

		// convert the data
		if ( acmStreamConvert(acmMp3stream, &mp3streamHead, ACM_STREAMCONVERTF_BLOCKALIGN)!=S_OK )
		{
			hasError = true;
			break;
		}

		// write the decoded PCM to disk
		memcpy(currentOutput, rawbuf, mp3streamHead.cbDstLengthUsed);
		totalDecompressedSize += mp3streamHead.cbDstLengthUsed;
		currentOutput += mp3streamHead.cbDstLengthUsed;
	}
	if ( hasError )
	{
		delete [] mp3BlockBuffer;
		LocalFree(rawbuf);
		acmStreamClose(acmMp3stream, 0);
		delete [] soundBuffer;
		pStream->Release();
		GlobalFree(mp3HGlobal);
		return false;
	}

	// Unprepare
	if ( acmStreamUnprepareHeader(acmMp3stream, &mp3streamHead, 0)!=S_OK )
	{
		delete [] mp3BlockBuffer;
		LocalFree(rawbuf);
		acmStreamClose(acmMp3stream, 0);
		delete [] soundBuffer;
		pStream->Release();
		GlobalFree(mp3HGlobal);
		return false;
	}
	delete [] mp3BlockBuffer;
	LocalFree(rawbuf);
	acmStreamClose(acmMp3stream, 0);

	// Release allocated memory
	pStream->Release();
	GlobalFree(mp3HGlobal);

	// Silence
	int remainingSize = bufferLength - (int)(ui64)(currentOutput-soundBuffer);
	memset(currentOutput, 0, remainingSize);

	// Silence
	int memRawOffset = 0;
	int memRawSize = bufferLength;

	// Load Raw
	bool result = LoadRaw(soundBuffer+memRawOffset, memRawSize-memRawOffset);
	delete [] soundBuffer;
	return result;
}

void cpu_sound::Close()
{
	if ( pVoice )
	{
		pVoice->DestroyVoice();
		pVoice = nullptr;
	}

	loaded = false;
	memset(&format, 0, sizeof(WAVEFORMATEX));
	memset(&buffer, 0, sizeof(XAUDIO2_BUFFER));
	CPU_DELPTRS(data);
	size = 0;
}
