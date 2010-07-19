#ifndef _CODEC_H
#define _CODEC_H

#include "msacmdrv.h"
#include "faac.h"
#include "faad.h"
class codec
{
    static const int m_iCompressedFormatTag;
    static const int m_iManufacturerID;
    static const int m_iProductID;
	
    static void fill_pcm_format(WAVEFORMATEX* pwfx, int rate, int bits, int channels);
    static void fill_compressed_format(WAVEFORMATEX* pwfx, int rate, int bits, int channels, int bitrate);
    int something;
    class stream
    {
    public:
		virtual ~stream() {}
		virtual HRESULT reset() =0;
		virtual HRESULT size(ACMDRVSTREAMSIZE*) =0;
		virtual HRESULT convert(ACMDRVSTREAMHEADER*) =0;
    };
    class encoder: public stream
    {
		WAVEFORMATEX m_sFormat;
		unsigned long m_iInputSamples;
		unsigned long m_iMaxOutputBytes;
		unsigned long m_iOutputBytesPerSec;
		faacEncHandle m_pHandle;
    public:
		encoder(WAVEFORMATEX* pF, WAVEFORMATEX* pFDest);
		~encoder() {}
		virtual HRESULT reset();
		virtual HRESULT size(ACMDRVSTREAMSIZE*);
		virtual HRESULT convert(ACMDRVSTREAMHEADER*);
    };
    class decoder: public stream
    {
		WAVEFORMATEX m_sFormat;
		faacDecHandle m_pHandle;
		bool m_bInitialized;
		unsigned char* m_pCache;
		int m_iCacheSize;
    public:
		decoder(WAVEFORMATEX* pF);
		~decoder() { delete[] m_pCache; }
		virtual HRESULT reset();
		virtual HRESULT size(ACMDRVSTREAMSIZE*);
		virtual HRESULT convert(ACMDRVSTREAMHEADER*);
    };
	public:
		codec();
		~codec();
		HRESULT formattag_details(ACMFORMATTAGDETAILSW* lParam1, DWORD lParam2);
		HRESULT format_details(ACMFORMATDETAILSW* lParam1, DWORD lParam2);
		HRESULT format_suggest(ACMDRVFORMATSUGGEST*);
		HRESULT details(ACMDRIVERDETAILSW*);
		HRESULT about(DWORD);
		HRESULT open(ACMDRVSTREAMINSTANCE*);
		HRESULT prepare(ACMDRVSTREAMINSTANCE*, ACMDRVSTREAMHEADER*);
		HRESULT reset(ACMDRVSTREAMINSTANCE*);
		HRESULT size(ACMDRVSTREAMINSTANCE*, ACMDRVSTREAMSIZE*);
		HRESULT unprepare(ACMDRVSTREAMINSTANCE*, ACMDRVSTREAMHEADER*);
		HRESULT convert(ACMDRVSTREAMINSTANCE*, ACMDRVSTREAMHEADER*);
		HRESULT close(ACMDRVSTREAMINSTANCE*);
};

#endif


