#include "codec.h"
#include <assert.h>

// we must register these by emailing to mmreg@microsoft.com
const int codec::m_iCompressedFormatTag = 0x1234;
const int codec::m_iManufacturerID = MM_GADGETLABS;
const int codec::m_iProductID = 7;

const wchar_t g_sCodecName[]=L"MPEG-2/4 AAC audio codec";
const wchar_t g_sLongCodecName[]=L"Very special audio codec";
const wchar_t g_sFormatName[]=L"MPEG-2/4 AAC";

codec::codec()
{
}

codec::~codec()
{
}

HRESULT codec::formattag_details(ACMFORMATTAGDETAILSW* lParam1, DWORD lParam2)
{
    bool bCompressedFormat;
    switch(lParam2)
    {
    case ACM_FORMATTAGDETAILSF_INDEX:
		if(lParam1->dwFormatTagIndex>=2)
			return ACMERR_NOTPOSSIBLE;
		bCompressedFormat=(lParam1->dwFormatTagIndex==1);
		break;
    case ACM_FORMATTAGDETAILSF_FORMATTAG:
		if(lParam1->dwFormatTag==1)
			bCompressedFormat=false;
		else if(lParam1->dwFormatTag==m_iCompressedFormatTag)
			bCompressedFormat=true;
		else
			return ACMERR_NOTPOSSIBLE;
    case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
		bCompressedFormat=true;
		break;
    default:
		return ACMERR_NOTPOSSIBLE;
    }
    lParam1->cbStruct=sizeof(ACMFORMATTAGDETAILSW);    
    lParam1->cbFormatSize=bCompressedFormat ? 20 : 16;    
    lParam1->fdwSupport=ACMDRIVERDETAILS_SUPPORTF_CODEC;
    if(bCompressedFormat)
		lParam1->cStandardFormats=8; // 44 & 48 khz 16 bit, mono & stereo, 64 & 128 kbps
    else
		lParam1->cStandardFormats=8; // 44 & 48 khz 8 & 16 bit, mono & stereo
    if(bCompressedFormat)
    {
		wcscpy(lParam1->szFormatTag, g_sFormatName);
		lParam1->dwFormatTag=m_iCompressedFormatTag;
    }
    else
    {
		wcscpy(lParam1->szFormatTag, L"PCM");
		lParam1->dwFormatTag=1;
    }
    return MMSYSERR_NOERROR;
}

void codec::fill_pcm_format(WAVEFORMATEX* pwfx, int rate, int bits, int channels)
{
    pwfx->wFormatTag=1;
    pwfx->nSamplesPerSec=rate;
    pwfx->wBitsPerSample=bits;
    pwfx->nChannels=channels;
    pwfx->nAvgBytesPerSec=rate * bits * channels / 8;
    pwfx->nBlockAlign=channels * bits / 8;
    pwfx->cbSize=0;
}

#define MAIN 0
#define LOW  1
#define SSR  2
#define LTP  3

static unsigned int aacSamplingRates[16] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
		16000, 12000, 11025, 8000, 7350, 0, 0, 0
};

void codec::fill_compressed_format(WAVEFORMATEX* pwfx, int rate, int bits, int channels, int bitrate)
{
    pwfx->wFormatTag=m_iCompressedFormatTag;
    pwfx->nSamplesPerSec=rate;
    pwfx->wBitsPerSample=bits;
    pwfx->nChannels=channels;
    pwfx->nAvgBytesPerSec=bitrate / 8;
    pwfx->nBlockAlign=1024;
    pwfx->cbSize=2;
	
    unsigned char* ext=(unsigned char*)(&pwfx[1]);
    int profile = MAIN;
    int samplerate_index=-1;
    for(int i=0; i<16; i++)
		if(aacSamplingRates[i]==rate)
		{
			samplerate_index=i;
			break;
		}
		if(samplerate_index<0)
			return;
		ext[0] = ((profile + 1) << 3) | ((samplerate_index & 0xe) >> 1);
		ext[1] = ((samplerate_index & 0x1) << 7) | (channels << 3);
		// 960 byte frames not used
}

HRESULT codec::format_details(ACMFORMATDETAILSW* lParam1, DWORD lParam2)
{
    lParam1->cbStruct=sizeof(ACMFORMATDETAILSW);
    lParam1->szFormat[0]=0;
    lParam1->fdwSupport=ACMDRIVERDETAILS_SUPPORTF_CODEC;
	
    if(lParam1->dwFormatTag==1)
    {
		if(lParam2==ACM_FORMATDETAILSF_INDEX)
		{
			switch(lParam1->dwFormatIndex)
			{
			case 0:
				fill_pcm_format(lParam1->pwfx, 44100, 16, 1);
				break;
			case 1:
				fill_pcm_format(lParam1->pwfx, 44100, 16, 2);
				break;
			case 2:
				fill_pcm_format(lParam1->pwfx, 44100, 8, 1);
				break;
			case 3:
				fill_pcm_format(lParam1->pwfx, 44100, 8, 2);
				break;
			case 4:
				fill_pcm_format(lParam1->pwfx, 48000, 16, 1);
				break;
			case 5:
				fill_pcm_format(lParam1->pwfx, 48000, 16, 2);
				break;
			case 6:
				fill_pcm_format(lParam1->pwfx, 48000, 8, 1);
				break;
			case 7:
				fill_pcm_format(lParam1->pwfx, 48000, 8, 2);
				break;
			default:
				return ACMERR_NOTPOSSIBLE;
			}
		}   
		else if(lParam2==ACM_FORMATDETAILSF_FORMAT)
		{
			if((lParam1->pwfx->nSamplesPerSec != 44100) && (lParam1->pwfx->nSamplesPerSec != 48000))
				return ACMERR_NOTPOSSIBLE;
		}
		else
			return ACMERR_NOTPOSSIBLE;
    }
    else if(lParam1->dwFormatTag==m_iCompressedFormatTag)
    {
		if(lParam2==ACM_FORMATDETAILSF_INDEX)
		{
			switch(lParam1->dwFormatIndex)
			{
			case 0:
				fill_compressed_format(lParam1->pwfx, 44100, 16, 1, 128000);
				break;
			case 1:
				fill_compressed_format(lParam1->pwfx, 44100, 16, 2, 128000);
				break;
			case 2:
				fill_compressed_format(lParam1->pwfx, 44100, 16, 1, 64000);
				break;
			case 3:
				fill_compressed_format(lParam1->pwfx, 44100, 16, 2, 64000);
				break;
			case 4:
				fill_compressed_format(lParam1->pwfx, 48000, 16, 1, 128000);
				break;
			case 5:
				fill_compressed_format(lParam1->pwfx, 48000, 16, 2, 128000);
				break;
			case 6:
				fill_compressed_format(lParam1->pwfx, 48000, 16, 1, 64000);
				break;
			case 7:
				fill_compressed_format(lParam1->pwfx, 48000, 16, 2, 64000);
				break;
			default:
				return ACMERR_NOTPOSSIBLE;
			}
		}
		else if(lParam2==ACM_FORMATDETAILSF_FORMAT)
		{
			if((lParam1->pwfx->nSamplesPerSec != 44100) && (lParam1->pwfx->nSamplesPerSec != 48000))
				return ACMERR_NOTPOSSIBLE;
		}
		else
			return ACMERR_NOTPOSSIBLE;
    }
    else
		return ACMERR_NOTPOSSIBLE;
    return MMSYSERR_NOERROR;
}

HRESULT codec::format_suggest(ACMDRVFORMATSUGGEST* pFormat)
{
    bool bEncode;
    if(pFormat->fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG)
    {
		if((pFormat->pwfxDst->wFormatTag == 1) && (pFormat->pwfxSrc->wFormatTag == m_iCompressedFormatTag))
			bEncode=false;
		else
			if((pFormat->pwfxDst->wFormatTag == m_iCompressedFormatTag) && (pFormat->pwfxSrc->wFormatTag == 1))
				bEncode=true;
			else
				return ACMERR_NOTPOSSIBLE;
    }
    else
    {
		if(pFormat->pwfxSrc->wFormatTag == m_iCompressedFormatTag)
		{
			pFormat->pwfxDst->wFormatTag=1; 
			bEncode=false;
		}
		else
			if(pFormat->pwfxSrc->wFormatTag == 1)
			{
				pFormat->pwfxDst->wFormatTag=m_iCompressedFormatTag; 
				bEncode=true;
			}
			else
				return ACMERR_NOTPOSSIBLE;
    }
    if(pFormat->fdwSuggest & ACM_FORMATSUGGESTF_NCHANNELS)
    {
		if(pFormat->pwfxDst->nChannels != pFormat->pwfxSrc->nChannels)
			return ACMERR_NOTPOSSIBLE;
    }
    int iChannels = pFormat->pwfxSrc->nChannels;
    if(pFormat->fdwSuggest & ACM_FORMATSUGGESTF_NSAMPLESPERSEC)
    {
		if(pFormat->pwfxDst->nSamplesPerSec != pFormat->pwfxSrc->nSamplesPerSec)
			return ACMERR_NOTPOSSIBLE;
    }
    int iSamplesPerSec = pFormat->pwfxSrc->nSamplesPerSec;
	
    if(pFormat->fdwSuggest & ACM_FORMATSUGGESTF_WBITSPERSAMPLE)
    {
		if(pFormat->pwfxDst->wBitsPerSample != pFormat->pwfxSrc->wBitsPerSample)
			return ACMERR_NOTPOSSIBLE;
    }
    int iBitsPerSample = pFormat->pwfxSrc->wBitsPerSample;
	
    if(bEncode)
		fill_compressed_format(pFormat->pwfxDst, iSamplesPerSec, iBitsPerSample, iChannels, 128000);
    else
		fill_pcm_format(pFormat->pwfxDst, iSamplesPerSec, iBitsPerSample, iChannels);
	
    return MMSYSERR_NOERROR;
}

HRESULT codec::details(ACMDRIVERDETAILSW* pDetails)
{
    memset(pDetails, 0, sizeof(ACMDRIVERDETAILSW));
    pDetails->cbStruct=sizeof(ACMDRIVERDETAILSW);
    pDetails->fccType=ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
    pDetails->fccComp=ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
    pDetails->wMid=m_iManufacturerID;
    pDetails->wPid=m_iProductID;
    pDetails->vdwACM=0x3320000;
    pDetails->vdwDriver=0x1000000;
    pDetails->fdwSupport=ACMDRIVERDETAILS_SUPPORTF_CODEC;
    pDetails->cFormatTags=2;
    pDetails->cFilterTags=0;
    wcscpy(pDetails->szShortName, g_sCodecName);
    wcscpy(pDetails->szLongName, g_sLongCodecName);
    return MMSYSERR_NOERROR;
}

HRESULT codec::about(DWORD h)
{
    if(h==(DWORD)-1)
		return MMSYSERR_NOERROR;
    MessageBoxW((HWND)h, g_sLongCodecName, L"About", MB_OK);
    return MMSYSERR_NOERROR;
}

HRESULT codec::open(ACMDRVSTREAMINSTANCE* pStream)
{
    if(pStream->pwfxDst->nChannels != pStream->pwfxSrc->nChannels)
		return ACMERR_NOTPOSSIBLE;
    if(pStream->pwfxDst->nSamplesPerSec != pStream->pwfxSrc->nSamplesPerSec)
		return ACMERR_NOTPOSSIBLE;
    bool bDecode = (pStream->pwfxDst->wFormatTag == 1);
    if(bDecode)
    {
		if(pStream->pwfxDst->wBitsPerSample!=16)
			return ACMERR_NOTPOSSIBLE;
		if(pStream->pwfxSrc->wFormatTag!=m_iCompressedFormatTag)
			return ACMERR_NOTPOSSIBLE;
    }
    else
    {
		//	if(pStream->pwfxSrc->wBitsPerSample!=16)
		//	    return ACMERR_NOTPOSSIBLE;
		if(pStream->pwfxDst->wFormatTag!=m_iCompressedFormatTag)
			return ACMERR_NOTPOSSIBLE;
		if(pStream->pwfxSrc->wFormatTag!=1)
			return ACMERR_NOTPOSSIBLE;
    }
	
    if(pStream->fdwOpen & ACM_STREAMOPENF_QUERY)
		return MMSYSERR_NOERROR;
	
    if(bDecode)
		pStream->dwDriver=(DWORD)new decoder(pStream->pwfxSrc);
    else
		pStream->dwDriver=(DWORD)new encoder(pStream->pwfxSrc, pStream->pwfxDst);
    return MMSYSERR_NOERROR;
}

HRESULT codec::prepare(ACMDRVSTREAMINSTANCE* pStream, ACMDRVSTREAMHEADER* pHeader)
{
    return MMSYSERR_NOTSUPPORTED;
}

HRESULT codec::reset(ACMDRVSTREAMINSTANCE* pStream)
{
    stream* pstr=(stream*)pStream->dwDriver;
    return pstr->reset();
}

HRESULT codec::size(ACMDRVSTREAMINSTANCE* pStream, ACMDRVSTREAMSIZE* pSize)
{
    stream* pstr=(stream*)pStream->dwDriver;
    return pstr->size(pSize);
}

HRESULT codec::unprepare(ACMDRVSTREAMINSTANCE* pStream, ACMDRVSTREAMHEADER* pHeader)
{
    return MMSYSERR_NOTSUPPORTED;
}

HRESULT codec::convert(ACMDRVSTREAMINSTANCE* pStream, ACMDRVSTREAMHEADER* pHeader)
{
    stream* pstr=(stream*)pStream->dwDriver;
    return pstr->convert(pHeader);
}

HRESULT codec::close(ACMDRVSTREAMINSTANCE* pStream)
{
    stream* pstr=(stream*)pStream->dwDriver;
    delete pstr;
    return MMSYSERR_NOERROR;
}

#include "faac.h"

codec::encoder::encoder(WAVEFORMATEX* pF, WAVEFORMATEX* pFDest) : m_sFormat(*pF)
{
    m_iOutputBytesPerSec=pFDest->nAvgBytesPerSec; 
	
    m_pHandle=faacEncOpen(pF->nSamplesPerSec, pF->nChannels, &m_iInputSamples, &m_iMaxOutputBytes);
	
    faacEncConfiguration conf;
    conf.mpegVersion = MPEG4;
    conf.aacObjectType = MAIN;
    conf.allowMidside = 1;
    conf.useLfe = 1;
    conf.useTns = 1;
    conf.bitRate = m_iOutputBytesPerSec/pFDest->nChannels; // bits/second per channel
    conf.bandWidth = 18000; //Hz
    conf.outputFormat = 1; // ADTS
    faacEncSetConfiguration(m_pHandle, &conf);    
}

HRESULT codec::encoder::reset()
{
    // fixme (?)
    return MMSYSERR_NOERROR;
}

HRESULT codec::encoder::size(ACMDRVSTREAMSIZE* pSize)
{
    double dwInputBitRate=m_sFormat.nSamplesPerSec * m_sFormat.wBitsPerSample * m_sFormat.nChannels;
    double dwOutputBitRate=m_iOutputBytesPerSec;// kbytes/s    
	
    if(pSize->fdwSize == ACM_STREAMSIZEF_SOURCE)
    {
		if(pSize->cbSrcLength<2*m_iInputSamples)
			pSize->cbSrcLength=2*m_iInputSamples;
		pSize->cbDstLength = pSize->cbSrcLength * 2 * dwOutputBitRate/dwInputBitRate;
		if(pSize->cbDstLength<m_iMaxOutputBytes)
			pSize->cbDstLength=m_iMaxOutputBytes;
		
    }
    else
    {
		if(pSize->cbDstLength<m_iMaxOutputBytes)
			pSize->cbDstLength=m_iMaxOutputBytes;
		pSize->cbSrcLength = pSize->cbDstLength * 2 * dwInputBitRate/dwOutputBitRate;;
		if(pSize->cbSrcLength<2*m_iInputSamples)
			pSize->cbSrcLength=2*m_iInputSamples;
    }
    
    return MMSYSERR_NOERROR;
}

#include <stdio.h>

HRESULT codec::encoder::convert(ACMDRVSTREAMHEADER* pHeader)
{
#if 0
    short* pSrc, *pDst;
    pSrc=(short*)(pHeader->pbSrc);
    pDst=(short*)(pHeader->pbDst);
    int iSrc=0, iDst=0;
    int block_size=m_sFormat.nChannels * 64;
    int samples_to_process=pHeader->cbSrcLength / (2 * block_size);
	
    for(int j=0; j<samples_to_process; j++)
    {
		if(m_sFormat.nChannels==1)
		{
			pDst[0]=pSrc[0];
			for(int i=1; i<64; i++)
				pDst[i]=(int)pSrc[i]-(int)pSrc[i-1];
			pSrc+=block_size;
			pDst+=block_size;
		}
		else
		{
			pDst[0]=pSrc[0];
			pDst[1]=pSrc[1];
			for(int i=1; i<64; i++)
			{
				pDst[2*i]=(int)pSrc[2*i]-(int)pSrc[2*i-2];
				pDst[2*i+1]=(int)pSrc[2*i+1]-(int)pSrc[2*i-1];
			}
			pSrc+=block_size;
			pDst+=block_size;
		}
    }
    FILE* f=fopen("c:\\enc.log", "ab");
    fwrite(pHeader->pbSrc, 2 * block_size * samples_to_process, 1, f);
    fclose(f);
    f=fopen("c:\\enc2.log", "ab");
    fwrite(pHeader->pbDst, 2 * block_size * samples_to_process, 1, f);
    fclose(f);
    pHeader->cbDstLengthUsed=2 * block_size * samples_to_process;
    pHeader->cbSrcLengthUsed=2 * block_size * samples_to_process;
    return MMSYSERR_NOERROR;
#else
    short* buffer=0;
    int length=pHeader->cbSrcLength;
    if(m_sFormat.wBitsPerSample!=16)
    {
		buffer = new short[length];
		for(int i=0; i<length; i++)
		{
			short s=(short)(((unsigned char*)pHeader->pbSrc)[i]);
			s-=128;
			s*=256;
			buffer[i]=s;
		}
    }
    short* pointer = buffer ? buffer : (short*)(pHeader->pbSrc);
    pHeader->cbSrcLengthUsed=0;
    pHeader->cbDstLengthUsed=0;
    while(1)
    {
		if(length-pHeader->cbSrcLengthUsed<2*m_iInputSamples)
			break;
		if(pHeader->cbDstLength-pHeader->cbDstLengthUsed<m_iMaxOutputBytes)
			break;
		int result=faacEncEncode(m_pHandle, pointer+pHeader->cbSrcLengthUsed,
			m_iInputSamples, 
			(short*)(pHeader->pbDst+pHeader->cbDstLengthUsed),
			pHeader->cbDstLength-pHeader->cbDstLengthUsed);
		if(result<0)
		{
			reset();
			break;
		}
        pHeader->cbDstLengthUsed+=result;
        pHeader->cbSrcLengthUsed+=2*m_iInputSamples;
    }    
    if(buffer)
    {
		pHeader->cbSrcLengthUsed/=2;
		delete[] buffer;
    }
    return MMSYSERR_NOERROR;
#endif
}

codec::decoder::decoder(WAVEFORMATEX* pF) : m_sFormat(*pF), m_bInitialized(false), m_pCache(0)
{
    m_pHandle=faacDecOpen();
}

HRESULT codec::decoder::reset()
{
    faacDecClose(m_pHandle);
    m_pHandle=faacDecOpen();
    m_bInitialized=false;
    delete[] m_pCache;
    m_pCache=0;
    return MMSYSERR_NOERROR;
}

HRESULT codec::decoder::size(ACMDRVSTREAMSIZE* pSize)
{
    double dwInputBitRate=m_sFormat.nAvgBytesPerSec;
    double dwOutputBitRate=m_sFormat.nSamplesPerSec * m_sFormat.wBitsPerSample * m_sFormat.nChannels;
	
    if(pSize->fdwSize == ACM_STREAMSIZEF_SOURCE)
    {
		if(pSize->cbSrcLength<768*m_sFormat.nChannels)
			pSize->cbSrcLength=768*m_sFormat.nChannels;
		pSize->cbDstLength = pSize->cbSrcLength * 2 * dwOutputBitRate/dwInputBitRate;
		if(pSize->cbDstLength<4096)
			pSize->cbDstLength=4096;
    }
    else
    {
		if(pSize->cbDstLength<4096)
			pSize->cbDstLength=4096;
		pSize->cbSrcLength = pSize->cbDstLength * 2 * dwInputBitRate/dwOutputBitRate;;
		if(pSize->cbSrcLength<768*m_sFormat.nChannels)
			pSize->cbSrcLength=768*m_sFormat.nChannels;
    }
    
    return MMSYSERR_NOERROR;
}

//static int iBytesProcessed=0;
HRESULT codec::decoder::convert(ACMDRVSTREAMHEADER* pHeader)
{
#if 0
    short *pSrc, *pDst;
    pSrc=(short*)pHeader->pbSrc;
    pDst=(short*)pHeader->pbDst;
    int iSrc=0, iDst=0;
    int block_size=m_sFormat.nChannels * 64;
    int samples_to_process=pHeader->cbSrcLength / (2 * block_size);
    for(int j=0; j<samples_to_process; j++)
    {
		if(m_sFormat.nChannels==1)
		{
			pDst[0]=pSrc[0];
			for(int i=1; i<64; i++)
				pDst[i]=(int)pSrc[i]+(int)pDst[i-1];
			pSrc+=block_size;
			pDst+=block_size;
		}
		else
		{
			pDst[0]=pSrc[0];
			pDst[1]=pSrc[1];
			for(int i=1; i<64; i++)
			{
				pDst[2*i]=(int)pSrc[2*i]+(int)pDst[2*i-2];
				pDst[2*i+1]=(int)pSrc[2*i+1]+(int)pDst[2*i-1];
			}
			pSrc+=block_size;
			pDst+=block_size;
		}
    }
    FILE* f=fopen("c:\\dec.log", "ab");
    fwrite(pHeader->pbDst, 2 * block_size * samples_to_process, 1, f);
    fclose(f);
    pHeader->cbDstLengthUsed=2 * block_size * samples_to_process;
    pHeader->cbSrcLengthUsed=2 * block_size * samples_to_process;
    return MMSYSERR_NOERROR;
#else
    // fixme: check fdwConvert contents
	
    const int iMinInputSize = 768*m_sFormat.nChannels;
    pHeader->cbSrcLengthUsed=0;
    pHeader->cbDstLengthUsed=0;
    int decoded_bytes=0;
    if(!m_bInitialized)
    {
		unsigned long samplerate;
		unsigned long channels;
		//assert(!m_pCache);
		// we don't really need this call
		// and it is done only because i am not sure if it's necessary for faac functionality
		pHeader->cbSrcLengthUsed=faacDecInit(m_pHandle, pHeader->pbSrc, &samplerate, &channels);
		m_bInitialized=true;
    }
    unsigned long bytesconsumed;
    unsigned long samples;    
    if(pHeader->cbDstLength<4096)
		goto finish;

    int iSrcDataLeft=pHeader->cbSrcLength;
	
    while(m_pCache)
    {
		int iFillSize = iMinInputSize - m_iCacheSize;
		if(iFillSize > iSrcDataLeft)
			iFillSize = iSrcDataLeft;
		memcpy(&m_pCache[m_iCacheSize], pHeader->pbSrc+pHeader->cbSrcLengthUsed, iFillSize);
		m_iCacheSize += iFillSize;
		iSrcDataLeft -= iFillSize;
		pHeader->cbSrcLengthUsed += iFillSize;
		if(m_iCacheSize < iMinInputSize)
			goto finish;
		int result=faacDecDecode(m_pHandle, m_pCache, &bytesconsumed, 
			(short*)(pHeader->pbDst+pHeader->cbDstLengthUsed), &samples); // no way to prevent output buffer overrun???
		if(result==FAAD_FATAL_ERROR)
		{
			reset();
			goto finish;
		}
		if(result==FAAD_OK)
			pHeader->cbDstLengthUsed+=sizeof(short)*samples;
		assert(bytesconsumed <= iMinInputSize);
		if(bytesconsumed < iMinInputSize)
			memmove(m_pCache, &m_pCache[bytesconsumed], iMinInputSize-bytesconsumed);
		m_iCacheSize-=bytesconsumed;
		if(m_iCacheSize==0)
		{
			delete[] m_pCache;
			m_pCache=0;
		}
    }

    if(iSrcDataLeft == 0)
		goto finish;
	
    if(iSrcDataLeft < iMinInputSize)
    {
		m_pCache = new unsigned char[iMinInputSize];
		memcpy(m_pCache, pHeader->pbSrc + pHeader->cbSrcLengthUsed, iSrcDataLeft);
		m_iCacheSize = iSrcDataLeft;
		pHeader->cbSrcLengthUsed = pHeader->cbSrcLength;
		goto finish;
    }
    
    while(iSrcDataLeft>=iMinInputSize)
    {
		if(pHeader->cbDstLength-pHeader->cbDstLengthUsed<4096)
			break;
		int result=faacDecDecode(m_pHandle, pHeader->pbSrc+pHeader->cbSrcLengthUsed, &bytesconsumed, 
			(short*)(pHeader->pbDst+pHeader->cbDstLengthUsed), &samples); // no way to prevent output buffer overrun???
		if(result==FAAD_FATAL_ERROR)
		{
			pHeader->cbSrcLengthUsed=pHeader->cbSrcLength;
			reset();
			goto finish;
		}
		if(result==FAAD_OK)
			pHeader->cbDstLengthUsed+=sizeof(short)*samples;
        pHeader->cbSrcLengthUsed+=bytesconsumed;
		iSrcDataLeft-=bytesconsumed;
    }    
finish:
    FILE* f=fopen("c:\\aac_acm.bin", "ab");
    fwrite(pHeader->pbSrc, pHeader->cbSrcLengthUsed, 1, f);
    fclose(f);
	//    iBytesProcessed+=pHeader->cbSrcLengthUsed;
    return MMSYSERR_NOERROR;
#endif
}

