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
#include <shellapi.h>	// ShellExecute
//#include <stdio.h>		// FILE *
//#include <stdlib.h>		// malloc, free
#include "resource.h"
#include "filters.h"	// CoolEdit
#include "Cfaac.h"

// *********************************************************************************************

extern HINSTANCE hInst;

// *********************************************************************************************



/*
DWORD PackCfg(MY_ENC_CFG *cfg)
{
DWORD dwOptions=0;

	if(cfg->AutoCfg)
		dwOptions=1<<31;
	dwOptions|=(DWORD)cfg->EncCfg.mpegVersion<<30;
	dwOptions|=(DWORD)cfg->EncCfg.aacObjectType<<28;
	dwOptions|=(DWORD)cfg->EncCfg.allowMidside<<27;
	dwOptions|=(DWORD)cfg->EncCfg.useTns<<26;
	dwOptions|=(DWORD)cfg->EncCfg.useLfe<<25;
	dwOptions|=(DWORD)cfg->EncCfg.outputFormat<<24;
	if(cfg->UseQuality)
		dwOptions|=(((DWORD)cfg->EncCfg.quantqual>>1)&0xff)<<16; // [2,512]
	else
		dwOptions|=(((DWORD)cfg->EncCfg.bitRate>>1)&0xff)<<16; // [2,512]
	if(cfg->UseQuality)
		dwOptions|=1<<15;
	dwOptions|=((DWORD)cfg->EncCfg.bandWidth>>1)&&0x7fff; // [0,65536]

	return dwOptions;
}
// -----------------------------------------------------------------------------------------------

void UnpackCfg(MY_ENC_CFG *cfg, DWORD dwOptions)
{
	cfg->AutoCfg=dwOptions>>31;
	cfg->EncCfg.mpegVersion=(dwOptions>>30)&1;
	cfg->EncCfg.aacObjectType=(dwOptions>>28)&3;
	cfg->EncCfg.allowMidside=(dwOptions>>27)&1;
	cfg->EncCfg.useTns=(dwOptions>>26)&1;
	cfg->EncCfg.useLfe=(dwOptions>>25)&1;
	cfg->EncCfg.outputFormat=(dwOptions>>24)&1;
	cfg->EncCfg.bitRate=((dwOptions>>16)&0xff)<<1;
	cfg->UseQuality=(dwOptions>>15)&1;
	cfg->EncCfg.bandWidth=(dwOptions&0x7fff)<<1;
}*/
// -----------------------------------------------------------------------------------------------

#define INIT_CB(hWnd,nID,list,IdSelected) \
{ \
	for(int i=0; list[i]; i++) \
		SendMessage(GetDlgItem(hWnd, nID), CB_ADDSTRING, 0, (LPARAM)list[i]); \
	SendMessage(GetDlgItem(hWnd, nID), CB_SETCURSEL, IdSelected, 0); \
}
// -----------------------------------------------------------------------------------------------

#define DISABLE_LTP \
{ \
	if(IsDlgButtonChecked(hWndDlg,IDC_RADIO_MPEG2) && \
	   IsDlgButtonChecked(hWndDlg,IDC_RADIO_LTP)) \
	{ \
		CheckDlgButton(hWndDlg,IDC_RADIO_LTP,FALSE); \
		CheckDlgButton(hWndDlg,IDC_RADIO_MAIN,TRUE); \
	} \
    EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_LTP), FALSE); \
}
// -----------------------------------------------------------------------------------------------

//        EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_SSR), Enabled);
//        EnableWindow(GetDlgItem(hWndDlg, IDC_CHK_USELFE), Enabled);
#define DISABLE_CTRL(Enabled) \
{ \
	CheckDlgButton(hWndDlg,IDC_CHK_AUTOCFG, !Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_MPEG4), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_MPEG2), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_RAW), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_ADTS), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_CHK_ALLOWMIDSIDE), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_CHK_USETNS), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_CHK_USELFE), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_CB_QUALITY), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_CB_BITRATE), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_CB_BANDWIDTH), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_QUALITY), Enabled); \
	EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_BITRATE), Enabled); \
    EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_MAIN), Enabled); \
    EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_LOW), Enabled); \
    EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_LTP), Enabled); \
	if(IsDlgButtonChecked(hWndDlg,IDC_RADIO_MPEG4)) \
		EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_LTP), Enabled); \
	else \
		DISABLE_LTP \
}
// -----------------------------------------------------------------------------------------------

BOOL DialogMsgProcAbout(HWND hWndDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
	case WM_INITDIALOG:
		{
		  char buf[512];
		  unsigned long samplesInput, maxBytesOutput;

		  faacEncHandle hEncoder =
		    faacEncOpen(44100, 2, &samplesInput, &maxBytesOutput);
		  faacEncConfigurationPtr myFormat =
		    faacEncGetCurrentConfiguration(hEncoder);

			sprintf(buf,
					APP_NAME " plugin " APP_VER " by Antonio Foranna\n\n"
					"Engines used:\n"
					"\tlibfaac v%s\n"
					"\tFAAD2 v" FAAD2_VERSION "\n"
					"\t" PACKAGE " v" VERSION "\n\n"
					"This code is given with FAAC package and does not contain executables.\n"
					"This program is free software and can be distributed/modifyed under the terms of the GNU General Public License.\n"
					"This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.\n\n"
					"Compiled on %s\n",
				(myFormat->version == FAAC_CFG_VERSION) ? myFormat->name : " bad version",
					__DATE__
					);
			SetDlgItemText(hWndDlg, IDC_L_ABOUT, buf);
			faacEncClose(hEncoder);
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hWndDlg, TRUE);
			break;
        case IDCANCEL:
			// Ignore data values entered into the controls and dismiss the dialog window returning FALSE
			EndDialog(hWndDlg, FALSE);
			break;
		case IDC_AUDIOCODING:
			ShellExecute(hWndDlg, NULL, "http://www.audiocoding.com", NULL, NULL, SW_SHOW);
			break;
		case IDC_MPEG4IP:
			ShellExecute(hWndDlg, NULL, "http://www.mpeg4ip.net", NULL, NULL, SW_SHOW);
			break;
		case IDC_EMAIL:
			ShellExecute(hWndDlg, NULL, "mailto:ntnfrn_email-temp@yahoo.it", NULL, NULL, SW_SHOW);
			break;
		}
		break;
	default: 
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------------

BOOL DIALOGMsgProc(HWND hWndDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
	case WM_INITDIALOG:
		{
		char buf[50];
		char *Quality[]={"Default","10","20","30","40","50","60","70","80","90","100","110","120","130","140","150","200","300","400","500",0};
		char *BitRate[]={"Auto","8","18","20","24","32","40","48","56","64","96","112","128","160","192","224","256","320","384",0};
		char *BandWidth[]={"Auto","Full","4000","8000","11025","16000","22050","24000","32000","44100","48000",0};
		MY_ENC_CFG cfg;

			Cfaac::getFaacCfg(&cfg);

			INIT_CB(hWndDlg,IDC_CB_QUALITY,Quality,0);
			INIT_CB(hWndDlg,IDC_CB_BITRATE,BitRate,0);
			INIT_CB(hWndDlg,IDC_CB_BANDWIDTH,BandWidth,0);

			if(cfg.EncCfg.mpegVersion==MPEG4)
				CheckDlgButton(hWndDlg,IDC_RADIO_MPEG4,TRUE);
			else
				CheckDlgButton(hWndDlg,IDC_RADIO_MPEG2,TRUE);

			switch(cfg.EncCfg.aacObjectType)
			{
			case MAIN:
				CheckDlgButton(hWndDlg,IDC_RADIO_MAIN,TRUE);
				break;
			case LOW:
				CheckDlgButton(hWndDlg,IDC_RADIO_LOW,TRUE);
				break;
			case SSR:
				CheckDlgButton(hWndDlg,IDC_RADIO_SSR,TRUE);
				break;
			case LTP:
				CheckDlgButton(hWndDlg,IDC_RADIO_LTP,TRUE);
				DISABLE_LTP
				break;
			}

			switch(cfg.EncCfg.outputFormat)
			{
			case RAW:
				CheckDlgButton(hWndDlg,IDC_RADIO_RAW,TRUE);
				break;
			case ADTS:
				CheckDlgButton(hWndDlg,IDC_RADIO_ADTS,TRUE);
				break;
			}

			CheckDlgButton(hWndDlg, IDC_CHK_ALLOWMIDSIDE, cfg.EncCfg.allowMidside);
			CheckDlgButton(hWndDlg, IDC_CHK_USETNS, cfg.EncCfg.useTns);
			CheckDlgButton(hWndDlg, IDC_CHK_USELFE, cfg.EncCfg.useLfe);

			if(cfg.UseQuality)
				CheckDlgButton(hWndDlg,IDC_RADIO_QUALITY,TRUE);
			else
				CheckDlgButton(hWndDlg,IDC_RADIO_BITRATE,TRUE);

			switch(cfg.EncCfg.quantqual)
			{
			case 100:
				SendMessage(GetDlgItem(hWndDlg, IDC_CB_QUALITY), CB_SETCURSEL, 0, 0);
				break;
			default:
				if(cfg.EncCfg.quantqual<10)
					cfg.EncCfg.quantqual=10;
				if(cfg.EncCfg.quantqual>500)
					cfg.EncCfg.quantqual=500;
				sprintf(buf,"%lu",cfg.EncCfg.quantqual);
				SetDlgItemText(hWndDlg, IDC_CB_QUALITY, buf);
				break;
			}
			switch(cfg.EncCfg.bitRate)
			{
			case 0:
				SendMessage(GetDlgItem(hWndDlg, IDC_CB_BITRATE), CB_SETCURSEL, 0, 0);
				break;
			default:
				sprintf(buf,"%lu",cfg.EncCfg.bitRate);
				SetDlgItemText(hWndDlg, IDC_CB_BITRATE, buf);
				break;
			}
			switch(cfg.EncCfg.bandWidth)
			{
			case 0:
				SendMessage(GetDlgItem(hWndDlg, IDC_CB_BANDWIDTH), CB_SETCURSEL, 0, 0);
				break;
			case 0xffffffff:
				SendMessage(GetDlgItem(hWndDlg, IDC_CB_BANDWIDTH), CB_SETCURSEL, 1, 0);
				break;
			default:
				sprintf(buf,"%lu",cfg.EncCfg.bandWidth);
				SetDlgItemText(hWndDlg, IDC_CB_BANDWIDTH, buf);
				break;
			}

			CheckDlgButton(hWndDlg, IDC_CHK_WRITEMP4, cfg.SaveMP4);

			CheckDlgButton(hWndDlg,IDC_CHK_AUTOCFG, cfg.AutoCfg);
			DISABLE_CTRL(!cfg.AutoCfg);
		}
		break; // End of WM_INITDIALOG                                 

	case WM_CLOSE:
		// Closing the Dialog behaves the same as Cancel               
		PostMessage(hWndDlg, WM_COMMAND, IDCANCEL, 0);
		break; // End of WM_CLOSE                                      

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_CHK_AUTOCFG:
			{
			char Enabled=!IsDlgButtonChecked(hWndDlg,IDC_CHK_AUTOCFG);
				DISABLE_CTRL(Enabled);
			}
			break;

		case IDOK:
			{
			char buf[50];
			MY_ENC_CFG cfg;

				cfg.AutoCfg=IsDlgButtonChecked(hWndDlg,IDC_CHK_AUTOCFG) ? TRUE : FALSE;
				cfg.EncCfg.mpegVersion=IsDlgButtonChecked(hWndDlg,IDC_RADIO_MPEG4) ? MPEG4 : MPEG2;
				if(IsDlgButtonChecked(hWndDlg,IDC_RADIO_MAIN))
					cfg.EncCfg.aacObjectType=MAIN;
				if(IsDlgButtonChecked(hWndDlg,IDC_RADIO_LOW))
					cfg.EncCfg.aacObjectType=LOW;
				if(IsDlgButtonChecked(hWndDlg,IDC_RADIO_SSR))
					cfg.EncCfg.aacObjectType=SSR;
				if(IsDlgButtonChecked(hWndDlg,IDC_RADIO_LTP))
					cfg.EncCfg.aacObjectType=LTP;
				cfg.EncCfg.allowMidside=IsDlgButtonChecked(hWndDlg, IDC_CHK_ALLOWMIDSIDE);
				cfg.EncCfg.useTns=IsDlgButtonChecked(hWndDlg, IDC_CHK_USETNS);
				cfg.EncCfg.useLfe=IsDlgButtonChecked(hWndDlg, IDC_CHK_USELFE);
				
				GetDlgItemText(hWndDlg, IDC_CB_BITRATE, buf, 50);
				switch(*buf)
				{
				case 'A': // Auto
					cfg.EncCfg.bitRate=0;
					break;
				default:
					cfg.EncCfg.bitRate=GetDlgItemInt(hWndDlg, IDC_CB_BITRATE, 0, FALSE);
				}
				GetDlgItemText(hWndDlg, IDC_CB_BANDWIDTH, buf, 50);
				switch(*buf)
				{
				case 'A': // Auto
					cfg.EncCfg.bandWidth=0;
					break;
				case 'F': // Full
					cfg.EncCfg.bandWidth=0xffffffff;
					break;
				default:
					cfg.EncCfg.bandWidth=GetDlgItemInt(hWndDlg, IDC_CB_BANDWIDTH, 0, FALSE);
				}
				cfg.UseQuality=IsDlgButtonChecked(hWndDlg,IDC_RADIO_QUALITY) ? TRUE : FALSE;
				GetDlgItemText(hWndDlg, IDC_CB_QUALITY, buf, 50);
				switch(*buf)
				{
				case 'D': // Default
					cfg.EncCfg.quantqual=100;
					break;
				default:
					cfg.EncCfg.quantqual=GetDlgItemInt(hWndDlg, IDC_CB_QUALITY, 0, FALSE);
				}
				cfg.EncCfg.outputFormat=IsDlgButtonChecked(hWndDlg,IDC_RADIO_RAW) ? RAW : ADTS;

				cfg.SaveMP4=IsDlgButtonChecked(hWndDlg, IDC_CHK_WRITEMP4) ? TRUE : FALSE;

				Cfaac::setFaacCfg(&cfg);

				EndDialog(hWndDlg, 1);
			}
			break;

        case IDCANCEL:
			// Ignore data values entered into the controls        
			// and dismiss the dialog window returning FALSE
			EndDialog(hWndDlg, FALSE);
			break;

		case IDC_BTN_ABOUT:
			DialogBox((HINSTANCE)hInst,(LPCSTR)MAKEINTRESOURCE(IDD_ABOUT), (HWND)hWndDlg, (DLGPROC)DialogMsgProcAbout);
			break;
			
		case IDC_RADIO_MPEG4:
			EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_LTP), !IsDlgButtonChecked(hWndDlg,IDC_CHK_AUTOCFG));
			break;
			
		case IDC_RADIO_MPEG2:
			EnableWindow(GetDlgItem(hWndDlg, IDC_RADIO_LTP), FALSE);
			DISABLE_LTP
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

#define ERROR_FGO(msg) \
{ \
	if(msg) \
	{ \
	char buf[100]; \
		sprintf(buf,"FilterGetOptions: %s", msg); \
		MessageBox(0, buf, APP_NAME " plugin", MB_OK|MB_ICONSTOP); \
	} \
	return 0; \
}
// -----------------------------------------------------------------------------------------------

DWORD FAR PASCAL FilterGetOptions(HWND hWnd, HINSTANCE hInst, long lSamprate, WORD wChannels, WORD wBitsPerSample, DWORD dwOptions)
{
long		retVal;
/*CRegistry	reg;
BOOL		OpenDialog=FALSE;

	if(!reg.openCreateReg(HKEY_LOCAL_MACHINE,REGISTRY_PROGRAM_NAME  "\\FAAD"))
		ERROR_FGO("Can't open registry!")
	else
		if(OpenDialog=reg.getSetRegBool("OpenDialog",FALSE))
			reg.setRegBool("OpenDialog",FALSE);

	if(OpenDialog)
	{
	MY_DEC_CFG	*Cfg;
		if(!(Cfg=(MY_DEC_CFG *)malloc(sizeof(MY_DEC_CFG))))
			ERROR_FGO("Memory allocation error");
		ReadCfgDec(Cfg);
		Cfg->DecCfg.defSampleRate=lSamprate;
		switch(wBitsPerSample)
		{
		case 16:
			Cfg->DecCfg.outputFormat=FAAD_FMT_16BIT;
			break;
		case 24:
			Cfg->DecCfg.outputFormat=FAAD_FMT_24BIT;
			break;
		case 32:
			Cfg->DecCfg.outputFormat=FAAD_FMT_32BIT;
			break;
		default:
			ERROR_FGO("Invalid Bps");
		}
		Cfg->Channels=(BYTE)wChannels;
		retVal=DialogBoxParam((HINSTANCE)hInst,(LPCSTR)MAKEINTRESOURCE(IDD_DECODER), (HWND)hWnd, (DLGPROC)DialogMsgProcDecoder, (DWORD)Cfg);
		WriteCfgDec(Cfg);
		FREE(Cfg);
	}
	else*/
		retVal=DialogBoxParam((HINSTANCE)hInst,(LPCSTR)MAKEINTRESOURCE(IDD_ENCODER), (HWND)hWnd, (DLGPROC)DIALOGMsgProc, dwOptions);

	if(retVal==-1)
		ERROR_FGO("DialogBoxParam");

	return retVal;
}
// *********************************************************************************************

// GetSuggestedSampleType() is called if OpenFilterOutput() returns NULL
void FAR PASCAL GetSuggestedSampleType(LONG *lplSamprate, WORD *lpwBitsPerSample, WORD *wChannels)
{
	*lplSamprate=0; // don't care
	*lpwBitsPerSample= *lpwBitsPerSample<=16 ? 0 : 16;
	*wChannels= *wChannels<49 ? 0 : 48;
}
// *********************************************************************************************

void FAR PASCAL CloseFilterOutput(HANDLE hOutput)
{
	if(!hOutput)
		return;

Cfaac tmp(hOutput); // this line frees memory
}              
// *********************************************************************************************

HANDLE FAR PASCAL OpenFilterOutput(LPSTR lpstrFilename,long lSamprate,WORD wBitsPerSample,WORD wChannels,long lSize, long far *lpChunkSize, DWORD dwOptions)
{
HANDLE	hOutput;
Cfaac	tmp;

	if(hOutput=tmp.Init(lpstrFilename,lSamprate,wBitsPerSample,wChannels,lSize))
	{
	MYOUTPUT *mo;
		GLOBALLOCK(mo,hOutput,MYOUTPUT,return NULL);
		*lpChunkSize=mo->samplesInput*(wBitsPerSample>>3); // size of samplesInput

		GlobalUnlock(hOutput);
		tmp.hOutput=NULL;
	}

	return hOutput;
}
// *********************************************************************************************

DWORD FAR PASCAL WriteFilterOutput(HANDLE hOutput, unsigned char far *bufIn, long lBytes)
{
	if(!hOutput)
		return 0;

Cfaac tmp;
DWORD bytesWritten;

	bytesWritten=tmp.processData(hOutput,bufIn,lBytes);
	return bytesWritten ? bytesWritten : 0x7fffffff; // bytesWritten<=0 stops CoolEdit
}
