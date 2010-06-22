/*
FAAC - codec plugin for Cooledit
Copyright (C) 2002-2004 Antonio Foranna

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation.
	
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
		
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
			
The author can be contacted at:
ntnfrn_email-temp@yahoo.it
*/

#include <windows.h>
#include <stdio.h>		// FILE *
#include "resource.h"
#include "filters.h"	// CoolEdit
#include "CRegistry.h"
#include "Defines.h"	// my defines
#include "Cfaad.h"

// *********************************************************************************************

extern HINSTANCE hInst;

// -----------------------------------------------------------------------------------------------

extern BOOL DialogMsgProcAbout(HWND hWndDlg, UINT Message, WPARAM wParam, LPARAM lParam);

// *********************************************************************************************

static const char* mpeg4AudioNames[]=
{
	"Raw PCM",
	"AAC Main",
	"AAC LC (Low Complexity)",
	"AAC SSR",
	"AAC LTP (Long Term Prediction)",
	"AAC HE (High Efficiency)",
	"AAC Scalable",
	"TwinVQ",
	"CELP",
	"HVXC",
	"Reserved",
	"Reserved",
	"TTSI",
	"Main synthetic",
	"Wavetable synthesis",
	"General MIDI",
	"Algorithmic Synthesis and Audio FX",
// defined in MPEG-4 version 2
	"ER AAC LC (Low Complexity)",
	"Reserved",
	"ER AAC LTP (Long Term Prediction)",
	"ER AAC Scalable",
	"ER TwinVQ",
	"ER BSAC",
	"ER AAC LD (Low Delay)",
	"ER CELP",
	"ER HVXC",
	"ER HILN",
	"ER Parametric",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved"
};

// *********************************************************************************************

#define INIT_CB(hWnd,nID,list,IdSelected) \
{ \
	for(int i=0; list[i]; i++) \
		SendMessage(GetDlgItem(hWnd, nID), CB_ADDSTRING, 0, (LPARAM)list[i]); \
	SendMessage(GetDlgItem(hWnd, nID), CB_SETCURSEL, IdSelected, 0); \
}
// -----------------------------------------------------------------------------------------------

//	EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_SSR), Enabled);
#define DISABLE_CTRL(Enabled) \
{ \
	EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_MAIN), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_LOW), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_LTP), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_HE), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_CHK_DOWNMATRIX), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_CHK_OLDADTS), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_CB_SAMPLERATE), Enabled); \
}
// -----------------------------------------------------------------------------------------------

static MY_DEC_CFG *CfgDecoder;

BOOL DialogMsgProcDecoder(HWND hWndDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
	case WM_INITDIALOG:
		{
			if(!lParam)
			{
				MessageBox(hWndDlg,"Pointer==NULL",0,MB_OK|MB_ICONSTOP);
				EndDialog(hWndDlg, 0);
				return TRUE;
			}

		char buf[50];
		char *SampleRate[]={"6000","8000","16000","22050","32000","44100","48000","64000","88200","96000","192000",0};
			CfgDecoder=(MY_DEC_CFG *)lParam;

			INIT_CB(hWndDlg,IDC_CB_SAMPLERATE,SampleRate,5);
			sprintf(buf,"%lu",CfgDecoder->DecCfg.defSampleRate);
			SetDlgItemText(hWndDlg, IDC_CB_SAMPLERATE, buf);

			switch(CfgDecoder->DecCfg.defObjectType)
			{
			case MAIN:
				CheckDlgButton(hWndDlg,IDC_RADIO_MAIN,TRUE);
				break;
			case LC:
				CheckDlgButton(hWndDlg,IDC_RADIO_LOW,TRUE);
				break;
			case SSR:
				CheckDlgButton(hWndDlg,IDC_RADIO_SSR,TRUE);
				break;
			case LTP:
				CheckDlgButton(hWndDlg,IDC_RADIO_LTP,TRUE);
				break;
			case HE_AAC:
				CheckDlgButton(hWndDlg,IDC_RADIO_HE,TRUE);
				break;
			}

			CheckDlgButton(hWndDlg,IDC_CHK_DOWNMATRIX, CfgDecoder->DefaultCfg);
			CheckDlgButton(hWndDlg,IDC_CHK_OLDADTS, CfgDecoder->DefaultCfg);

			CheckDlgButton(hWndDlg,IDC_CHK_DEFAULTCFG, CfgDecoder->DefaultCfg);
			DISABLE_CTRL(!CfgDecoder->DefaultCfg);
		}
		break; // End of WM_INITDIALOG                                 

	case WM_CLOSE:
		// Closing the Dialog behaves the same as Cancel               
		PostMessage(hWndDlg, WM_COMMAND, IDCANCEL, 0);
		break; // End of WM_CLOSE                                      

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_CHK_DEFAULTCFG:
			{
			char Enabled=!IsDlgButtonChecked(hWndDlg,IDC_CHK_DEFAULTCFG);
				DISABLE_CTRL(Enabled);
			}
			break;

		case IDOK:
			{
				if(IsDlgButtonChecked(hWndDlg,IDC_RADIO_MAIN))
					CfgDecoder->DecCfg.defObjectType=MAIN;
				if(IsDlgButtonChecked(hWndDlg,IDC_RADIO_LOW))
					CfgDecoder->DecCfg.defObjectType=LC;
				if(IsDlgButtonChecked(hWndDlg,IDC_RADIO_SSR))
					CfgDecoder->DecCfg.defObjectType=SSR;
				if(IsDlgButtonChecked(hWndDlg,IDC_RADIO_LTP))
					CfgDecoder->DecCfg.defObjectType=LTP;
				if(IsDlgButtonChecked(hWndDlg,IDC_RADIO_HE))
					CfgDecoder->DecCfg.defObjectType=HE_AAC;

				CfgDecoder->DecCfg.defSampleRate=GetDlgItemInt(hWndDlg, IDC_CB_SAMPLERATE, 0, FALSE);
				CfgDecoder->DefaultCfg=IsDlgButtonChecked(hWndDlg,IDC_CHK_DEFAULTCFG) ? TRUE : FALSE;
				CfgDecoder->DecCfg.downMatrix=IsDlgButtonChecked(hWndDlg,IDC_CHK_DOWNMATRIX) ? TRUE : FALSE;
				CfgDecoder->DecCfg.useOldADTSFormat=IsDlgButtonChecked(hWndDlg,IDC_CHK_OLDADTS) ? TRUE : FALSE;
				Cfaad::WriteCfgDec(CfgDecoder);

				EndDialog(hWndDlg, (DWORD)CfgDecoder);
			}
			break;

        case IDCANCEL:
			// Ignore data values entered into the controls        
			// and dismiss the dialog window returning FALSE
			EndDialog(hWndDlg, (DWORD)FALSE);
			break;

		case IDC_BTN_ABOUT:
				DialogBox((HINSTANCE)hInst,(LPCSTR)MAKEINTRESOURCE(IDD_ABOUT), (HWND)hWndDlg, (DLGPROC)DialogMsgProcAbout);
			break;
		}
		break; // End of WM_COMMAND
	default: 
		return FALSE;
	}
 
	return TRUE;
}

// *********************************************************************************************
// *********************************************************************************************
// *********************************************************************************************

BOOL FAR PASCAL FilterUnderstandsFormat(LPSTR filename)
{
WORD len;

	if((len=lstrlen(filename))>4 && 
		(!strcmpi(filename+len-4,".aac") ||
		!strcmpi(filename+len-4,".mp4")))
		return TRUE;
	return FALSE;
}
// *********************************************************************************************

long FAR PASCAL FilterGetFileSize(HANDLE hInput)
{
	if(!hInput)
		return 0;

DWORD	dst_size;
MYINPUT	*mi;

	GLOBALLOCK(mi,hInput,MYINPUT,return 0);
	dst_size=mi->dst_size;
	
	GlobalUnlock(hInput);

	return dst_size;
}
// *********************************************************************************************

DWORD FAR PASCAL FilterOptionsString(HANDLE hInput, LPSTR szString)
{
	if(!hInput)
	{
		lstrcpy(szString,"");
		return 0;
	}

MYINPUT	*mi;

	GLOBALLOCK(mi,hInput,MYINPUT,return 0);

	sprintf(szString,"MPEG%d - %lu bps\n", mi->file_info.version ? 4 : 2, mi->file_info.bitrate);
	
	if(mi->IsMP4)  // MP4 file --------------------------------------------------------------------
		lstrcat(szString,mpeg4AudioNames[mi->type]);
	else  // AAC file -----------------------------------------------------------------------------
	{
		switch(mi->file_info.headertype)
		{
		case RAW:
			sprintf(szString,"MPEG%d\nRaw\n", mi->file_info.version ? 4 : 2);
			lstrcat(szString,mpeg4AudioNames[mi->file_info.object_type]);
			GlobalUnlock(hInput);
			return 1;//0; // call FilterGetOptions()
		case ADIF:
			lstrcat(szString,"ADIF\n");
			break;
		case ADTS:
			lstrcat(szString,"ADTS\n");
			break;
		}
		
		lstrcat(szString,mpeg4AudioNames[mi->file_info.object_type]);
/*		switch(mi->file_info.object_type)
		{
		case MAIN:
			lstrcat(szString,"Main");
			break;
		case LC:
			lstrcat(szString,"LC (Low Complexity)");
			break;
		case SSR:
			lstrcat(szString,"SSR (unsupported)");
			break;
		case LTP:
			lstrcat(szString,"LTP (Long Term Prediction)");
			break;
		case HE_AAC:
			lstrcat(szString,"HE (High Efficiency)");
			break;
		}*/
	}
	
	GlobalUnlock(hInput);
	return 1; // don't call FilterGetOptions()
}
// *********************************************************************************************
/*
DWORD FAR PASCAL FilterOptions(HANDLE hInput)
{
//	FilterGetOptions() is called if this function and FilterSetOptions() are exported and FilterOptionsString() returns 0
//	FilterSetOptions() is called only if this function is exported and and it returns 0

	return 1;
}
// ---------------------------------------------------------------------------------------------

DWORD FAR PASCAL FilterSetOptions(HANDLE hInput, DWORD dwOptions, LONG lSamprate, WORD wChannels, WORD wBPS)
{
	return dwOptions;
}*/
// *********************************************************************************************

void FAR PASCAL CloseFilterInput(HANDLE hInput)
{
	if(!hInput)
		return;

/*	if(mi->file_info.headertype==RAW)
	{
	CRegistry	reg;

		if(reg.openCreateReg(HKEY_LOCAL_MACHINE,REGISTRY_PROGRAM_NAME  "\\FAAD"))
			reg.setRegBool("OpenDialog",FALSE);
		else
			MessageBox(0,"Can't open registry!",0,MB_OK|MB_ICONSTOP);
	}*/

Cfaad tmp(hInput);
}
// *********************************************************************************************

#define ERROR_OFI(msg) \
{ \
	if(msg) \
		MessageBox(0, msg, APP_NAME " plugin", MB_OK|MB_ICONSTOP); \
	if(hInput) \
	{ \
		GlobalUnlock(hInput); \
		CloseFilterInput(hInput); \
	} \
	return 0; \
}
// -----------------------------------------------------------------------------------------------

// return handle that will be passed in to close, and write routines
HANDLE FAR PASCAL OpenFilterInput(LPSTR lpstrFilename, long far *lSamprate, WORD far *wBitsPerSample, WORD far *wChannels, HWND hWnd, long far *lChunkSize)
{
HANDLE	hInput;
Cfaad	tmp;

	if(hInput=tmp.getInfos(lpstrFilename))
	{
	MYINPUT	*mi;
		GLOBALLOCK(mi,hInput,MYINPUT,return NULL);
	
		if(mi->file_info.headertype!=RAW || mi->IsMP4) // to show dialog asking for samplerate
			*lSamprate=mi->Samprate;
		*wBitsPerSample=mi->BitsPerSample;
		*wChannels=(WORD)mi->Channels;
		*lChunkSize=(*wBitsPerSample/8)*1024**wChannels*2;

		GlobalUnlock(hInput);
		tmp.hInput=NULL;
	}
	return hInput;
}
// *********************************************************************************************

DWORD FAR PASCAL ReadFilterInput(HANDLE hInput, unsigned char far *bufout, long lBytes)
{
	if(!hInput)
		return 0;

Cfaad tmp;

	return tmp.processData(hInput,bufout,lBytes);
}
