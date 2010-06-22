// aac_acm.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "codec.h"
#include <list>

struct messagetype
{
    const char* c;
    int i;
};

const messagetype g_types[] =
{
	"DRV_LOAD", DRV_LOAD,
	"DRV_FREE", DRV_FREE,
	"DRV_OPEN", DRV_OPEN,
	"DRV_CLOSE", DRV_CLOSE,
	"DRV_DISABLE", DRV_DISABLE,
	"DRV_ENABLE", DRV_ENABLE,
	"DRV_INSTALL", DRV_INSTALL,
	"DRV_REMOVE", DRV_REMOVE,
	"DRV_CONFIGURE", DRV_CONFIGURE,
	"DRV_QUERYCONFIGURE", DRV_QUERYCONFIGURE,
	"ACMDM_DRIVER_DETAILS", ACMDM_DRIVER_DETAILS,
	"ACMDM_DRIVER_ABOUT", ACMDM_DRIVER_ABOUT,
	"ACMDM_FORMATTAG_DETAILS", ACMDM_FORMATTAG_DETAILS,
	"ACMDM_FORMAT_DETAILS", ACMDM_FORMAT_DETAILS,
	"ACMDM_FORMAT_SUGGEST", ACMDM_FORMAT_SUGGEST,
	"ACMDM_STREAM_OPEN", ACMDM_STREAM_OPEN,
	"ACMDM_STREAM_CLOSE", ACMDM_STREAM_CLOSE,
	"ACMDM_STREAM_SIZE", ACMDM_STREAM_SIZE,
	"ACMDM_STREAM_CONVERT", ACMDM_STREAM_CONVERT,
	"ACMDM_STREAM_RESET", ACMDM_STREAM_RESET,
	"ACMDM_STREAM_PREPARE", ACMDM_STREAM_PREPARE,
	"ACMDM_STREAM_UNPREPARE", ACMDM_STREAM_UNPREPARE,
	"ACMDM_STREAM_UPDATE", ACMDM_STREAM_UPDATE,
};

#include <stdio.h>

static void Message(const char* fmt, ...)
{
#ifndef NDEBUG
	FILE* f=fopen("c:\\msg.log", "ab");
	va_list va;
	va_start(va, fmt);
	vfprintf(f, fmt, va);
	va_end(va);
	fclose(f);
#endif
}

//codec c1;
//codec c2;
//int iCodecs=0;

bool g_bAttached=false;
std::list<codec*> g_codec_objects;

BOOL APIENTRY DllMain( HANDLE hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved
					  )
{
    Message("DllMain(%d)\n");
    if(ul_reason_for_call==DLL_PROCESS_ATTACH)
		g_bAttached=true;
    if(ul_reason_for_call==DLL_PROCESS_DETACH)
    {
		for(std::list<codec*>::iterator it=g_codec_objects.begin();
		it!=g_codec_objects.end();
		it++)
			delete *it;
		g_bAttached=false;
    }
    return TRUE;
}


extern "C" LONG WINAPI DriverProc(DWORD dwDriverID, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2) 
{
    codec *cdx = (codec *)(UINT)dwDriverID;
    ICOPEN *icinfo = (ICOPEN *)lParam2;
	
    if(g_bAttached==false) 
		// it really happens!
		// and since our heap may be already destroyed, we don't dare to do anything
		return E_FAIL;
	
    for(int i=0; i<sizeof(g_types)/sizeof(g_types[0]); i++)
    {
		if(uiMessage==g_types[i].i)
		{
			Message("%x %s %x %x\n", dwDriverID, g_types[i].c, lParam1, lParam2);
			goto cont;
		}
    }
    Message("%x %x %x %x\n", dwDriverID, uiMessage, lParam1, lParam2);
cont:
    switch (uiMessage) 
    {
    /****************************************
	
	  standard driver messages
	  
	****************************************/
		
    case DRV_LOAD:
		return (LRESULT)1L;
		
    case DRV_FREE:
		return (LRESULT)1L;
		
    case DRV_OPEN:
		if (icinfo && icinfo->fccType != ICTYPE_AUDIO) return NULL;
		//	if(!iCodecs)
		//	    cdx=&c1;
		//	else
		//	    cdx=&c2;
		//	iCodecs++;
		cdx = new codec;
		g_codec_objects.push_back(cdx);
		if (icinfo) icinfo->dwError = cdx ? ICERR_OK : ICERR_MEMORY;
		Message(" ==> %x\n", cdx);
		return (LRESULT)(DWORD)(UINT) cdx;
		
    case DRV_CLOSE:
		g_codec_objects.remove(cdx);
		delete cdx;
		return (LRESULT)1L;
		
    case DRV_DISABLE:
    case DRV_ENABLE:
		return (LRESULT)1L;
		
    case DRV_INSTALL:
    case DRV_REMOVE:
		return (LRESULT)DRV_OK;
		
    case DRV_QUERYCONFIGURE:    
		return (LRESULT)0L; // does support drive configure with the about box
		
    case DRV_CONFIGURE:
		//	return cdx->about(lParam1,lParam2);
		MessageBox(0, "Configure", "qqq", MB_OK);
		return DRVCNF_OK;
		
    case ACMDM_DRIVER_DETAILS:
		return cdx->details((ACMDRIVERDETAILSW*)lParam1);
		
    case ACMDM_DRIVER_ABOUT:
		return cdx->about((DWORD)lParam1);
		
    case ACMDM_FORMATTAG_DETAILS:
		return cdx->formattag_details((ACMFORMATTAGDETAILSW*)lParam1, (DWORD)lParam2);
		
    case ACMDM_FORMAT_DETAILS:
		return cdx->format_details((ACMFORMATDETAILSW*)lParam1, (DWORD)lParam2);
		
    case ACMDM_FORMAT_SUGGEST:
		return cdx->format_suggest((ACMDRVFORMATSUGGEST*)lParam1);
		
    case ACMDM_STREAM_OPEN:
		return cdx->open((ACMDRVSTREAMINSTANCE*)lParam1);
		
    case ACMDM_STREAM_PREPARE:
		return cdx->prepare((ACMDRVSTREAMINSTANCE*)lParam1, (ACMDRVSTREAMHEADER*) lParam2);
		
    case ACMDM_STREAM_RESET:
		return cdx->reset((ACMDRVSTREAMINSTANCE*)lParam1);
		
    case ACMDM_STREAM_SIZE:
		return cdx->size((ACMDRVSTREAMINSTANCE*)lParam1, (ACMDRVSTREAMSIZE*)lParam2);
		
    case ACMDM_STREAM_UNPREPARE:
		return cdx->unprepare((ACMDRVSTREAMINSTANCE*)lParam1, (ACMDRVSTREAMHEADER*) lParam2);
		
    case ACMDM_STREAM_CONVERT:
		return cdx->convert((ACMDRVSTREAMINSTANCE*)lParam1, (ACMDRVSTREAMHEADER*) lParam2);
		
    case ACMDM_STREAM_CLOSE:
		return cdx->close((ACMDRVSTREAMINSTANCE*)lParam1);
		
    }
	
    if (uiMessage < DRV_USER)
		return DefDriverProc(dwDriverID, hDriver, uiMessage, lParam1, lParam2);
    else
        return MMSYSERR_NOTSUPPORTED;
}
