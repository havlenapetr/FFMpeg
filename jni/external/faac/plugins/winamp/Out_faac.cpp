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
#include <shlobj.h>
#include <stdio.h>		// FILE *
#include <shellapi.h>	// ShellExecute
#include "resource.h"
#include "out.h"
/*#include <mp4.h>
#include <faac.h>
#include "CRegistry.h"
#include "defines.h"
*/
#include "defines.h"
#include "Cfaac.h"



void Config(HWND);
void About(HWND);
void Init();
void Quit();
int Open(int, int, int, int, int);
void Close();
int Write(char*, int);
int CanWrite();
int IsPlaying();
int Pause(int);
void SetVolume(int);
void SetPan(int);
void Flush(int);
int GetOutputTime();
int GetWrittenTime();



Cfaac			*Cpcmaac;
char			OutDir[MAX_PATH]="";

HINSTANCE		hInstance=NULL;
static			HBITMAP hBmBrowse=NULL;
char			config_AACoutdir[MAX_PATH]="";

static int		srate, numchan, bps;
volatile int	writtentime, w_offset;
static int		last_pause=0;


Out_Module out = {
	OUT_VER,
	APP_NAME " " APP_VER,
	NULL,
    NULL, // hmainwindow
    NULL, // hdllinstance
    Config,
    About,
    Init,
    Quit,
    Open,
    Close,
    Write,
    CanWrite,
    IsPlaying,
    Pause,
    SetVolume,
    SetPan,
    Flush,
    GetOutputTime,
    GetWrittenTime
};



// *********************************************************************************************



Out_Module *winampGetOutModule()
{
	return &out;
}
// *********************************************************************************************

BOOL WINAPI DllMain (HINSTANCE hInst, DWORD ulReason, LPVOID lpReserved)
{
	switch(ulReason)
	{
	case DLL_PROCESS_ATTACH:
		hInstance=hInst;
		DisableThreadLibraryCalls((struct HINSTANCE__ *)hInst);
		if(!hBmBrowse)
			hBmBrowse=(HBITMAP)LoadImage(hInst,MAKEINTRESOURCE(IDB_BROWSE),IMAGE_BITMAP,0,0,/*LR_CREATEDIBSECTION|*/LR_LOADTRANSPARENT|LR_LOADMAP3DCOLORS);
		
		/*	Code from LibMain inserted here.  Return TRUE to keep the
			DLL loaded or return FALSE to fail loading the DLL.

			You may have to modify the code in your original LibMain to
			account for the fact that it may be called more than once.
			You will get one DLL_PROCESS_ATTACH for each process that
			loads the DLL. This is different from LibMain which gets
			called only once when the DLL is loaded. The only time this
			is critical is when you are using shared data sections.
			If you are using shared data sections for statically
			allocated data, you will need to be careful to initialize it
			only once. Check your code carefully.

			Certain one-time initializations may now need to be done for
			each process that attaches. You may also not need code from
			your original LibMain because the operating system may now
			be doing it for you.
		*/
		break;
		
	case DLL_THREAD_ATTACH:
		/*	Called each time a thread is created in a process that has
			already loaded (attached to) this DLL. Does not get called
			for each thread that exists in the process before it loaded
			the DLL.
	
			Do thread-specific initialization here.
		*/
		break;
		
	case DLL_THREAD_DETACH:
		/*	Same as above, but called when a thread in the process
			exits.
		
			Do thread-specific cleanup here.
		*/
		break;
		
	case DLL_PROCESS_DETACH:
		hInstance=NULL;
		if(hBmBrowse)
		{
            DeleteObject(hBmBrowse);
            hBmBrowse=NULL;
		}
		/*	Code from _WEP inserted here.  This code may (like the
			LibMain) not be necessary.  Check to make certain that the
			operating system is not doing it for you.
		*/
		break;
	}
	
	/*	The return value is only used for DLL_PROCESS_ATTACH; all other
		conditions are ignored.
	*/
	return TRUE;   // successful DLL_PROCESS_ATTACH
}

// *********************************************************************************************
//										Interface
// *********************************************************************************************

static BOOL CALLBACK DialogMsgProcAbout(HWND hWndDlg, UINT Message, WPARAM wParam, LPARAM lParam)
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
//					"\tFAAD2 v" FAAD2_VERSION "\n"
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

void About(HWND hWndDlg)
{
	DialogBox(out.hDllInstance, MAKEINTRESOURCE(IDD_ABOUT), hWndDlg, DialogMsgProcAbout);

/*char buf[256];
  unsigned long samplesInput, maxBytesOutput;
  faacEncHandle hEncoder =
    faacEncOpen(44100, 2, &samplesInput, &maxBytesOutput);
  faacEncConfigurationPtr myFormat =
    faacEncGetCurrentConfiguration(hEncoder);

	sprintf(buf,
			APP_NAME " %s by Antonio Foranna\n\n"
			"This plugin uses FAAC encoder engine v%s\n\n"
			"Compiled on %s\n",
			 APP_VER,
			 myFormat->name,
			 __DATE__
			 );
	faacEncClose(hEncoder);
	MessageBox(hWndDlg, buf, "About", MB_OK);*/
}
// *********************************************************************************************

static int CALLBACK WINAPI BrowseCallbackProc( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED)
	{
		SetWindowText(hwnd,"Select Directory");
		SendMessage(hwnd,BFFM_SETSELECTION,(WPARAM)1,(LPARAM)config_AACoutdir);
	}
	return 0;
}
// -----------------------------------------------------------------------------------------------

void ReadCfgEnc()
{ 
CRegistry reg;

	if(reg.openCreateReg(HKEY_LOCAL_MACHINE,REGISTRY_PROGRAM_NAME))
		reg.getSetRegStr("OutDir","",OutDir,MAX_PATH); 
	else
		MessageBox(0,"Can't open registry!",0,MB_OK|MB_ICONSTOP);
}
// -----------------------------------------------------------------------------------------------

void WriteCfgEnc()
{ 
CRegistry reg;

	if(reg.openCreateReg(HKEY_LOCAL_MACHINE,REGISTRY_PROGRAM_NAME))
		reg.setRegStr("OutDir",OutDir); 
	else
		MessageBox(0,"Can't open registry!",0,MB_OK|MB_ICONSTOP);
}
// -----------------------------------------------------------------------------------------------

#define INIT_CB(hWnd,nID,list,IdSelected) \
{ \
	for(int i=0; list[i]; i++) \
		SendMessage(GetDlgItem(hWnd, nID), CB_ADDSTRING, 0, (LPARAM)list[i]); \
	SendMessage(GetDlgItem(hWnd, nID), CB_SETCURSEL, IdSelected, 0); \
}

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

static BOOL CALLBACK DIALOGMsgProc(HWND hWndDlg, UINT Message, WPARAM wParam, LPARAM lParam)
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
			
			ReadCfgEnc();
			Cfaac::getFaacCfg(&cfg);
			
			INIT_CB(hWndDlg,IDC_CB_QUALITY,Quality,0);
			INIT_CB(hWndDlg,IDC_CB_BITRATE,BitRate,0);
			INIT_CB(hWndDlg,IDC_CB_BANDWIDTH,BandWidth,0);
			
			SendMessage(GetDlgItem(hWndDlg, IDC_BTN_BROWSE), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hBmBrowse);
			if(!*OutDir)
				GetCurrentDirectory(MAX_PATH,OutDir);
			SetDlgItemText(hWndDlg, IDC_E_BROWSE, OutDir);
			
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
		PostMessage(hWndDlg, WM_COMMAND, IDCANCEL, 0L);
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
			HANDLE hCfg=(HANDLE)lParam;
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
					cfg.EncCfg.bitRate=1000*GetDlgItemInt(hWndDlg, IDC_CB_BITRATE, 0, FALSE);
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
				GetDlgItemText(hWndDlg, IDC_E_BROWSE, OutDir, MAX_PATH);
				
				cfg.SaveMP4=IsDlgButtonChecked(hWndDlg, IDC_CHK_WRITEMP4) ? TRUE : FALSE;

				WriteCfgEnc();
				Cfaac::setFaacCfg(&cfg);
				EndDialog(hWndDlg, (DWORD)hCfg);
			}
			break;
			
        case IDCANCEL:
			// Ignore data values entered into the controls        
			// and dismiss the dialog window returning FALSE
			EndDialog(hWndDlg, FALSE);
			break;
			
		case IDC_BTN_BROWSE:
			{
			char name[MAX_PATH];
			BROWSEINFO bi;
			ITEMIDLIST *idlist;
				bi.hwndOwner = hWndDlg;
				bi.pidlRoot = 0;
				bi.pszDisplayName = name;
				bi.lpszTitle = "Select a directory for AAC-MPEG4 file output:";
				bi.ulFlags = BIF_RETURNONLYFSDIRS;
				bi.lpfn = BrowseCallbackProc;
				bi.lParam = 0;
				
				GetDlgItemText(hWndDlg, IDC_E_BROWSE, config_AACoutdir, MAX_PATH);
				idlist = SHBrowseForFolder( &bi );
				if(idlist)
				{
					SHGetPathFromIDList( idlist, config_AACoutdir);
					SetDlgItemText(hWndDlg, IDC_E_BROWSE, config_AACoutdir);
				}
			}
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
} // End of DIALOGSMsgProc                                      
// *********************************************************************************************

void Config(HWND hWnd)
{
	DialogBox(out.hDllInstance, MAKEINTRESOURCE(IDD_ENCODER), hWnd, DIALOGMsgProc);
//	dwOptions=DialogBoxParam((HINSTANCE)out.hDllInstance,(LPCSTR)MAKEINTRESOURCE(IDD_ENCODER), (HWND)hWnd, (DLGPROC)DIALOGMsgProc, dwOptions);
}

// *********************************************************************************************
//									Utilities
// *********************************************************************************************

static char *scanstr_back(char *str, char *toscan, char *defval)
{
char *s=str+strlen(str)-1;

	if (strlen(str) < 1) return defval;
	if (strlen(toscan) < 1) return defval;
	while (1)
	{
		char *t=toscan;
		while (*t)
			if (*t++ == *s) return s;
		t=CharPrev(str,s);
		if (t==s) return defval;
		s=t;
	}
}

void GetNewFileName(char *lpstrFilename)
{
char temp2[MAX_PATH];
char *t,*p;

	GetWindowText(out.hMainWindow,temp2,sizeof(temp2));
	t=temp2;
	
	t=scanstr_back(temp2,"-",NULL);
	if (t) t[-1]=0;
	
	if (temp2[0] && temp2[1] == '.')
	{
		char *p1,*p2;
		p1=lpstrFilename;
		p2=temp2;
		while (*p2) *p1++=*p2++;
		*p1=0;
		p1 = temp2+1;
		p2 = lpstrFilename;
		while (*p2)
		{
			*p1++ = *p2++;
		}
		*p1=0;
		temp2[0] = '0';
	}
	p=temp2;
	while (*p != '.' && *p) p++;
	if (*p == '.') 
	{
		*p = '-';
		p=CharNext(p);
	}
	while (*p)
	{
		if (*p == '.' || *p == '/' || *p == '\\' || *p == '*' || 
			*p == '?' || *p == ':' || *p == '+' || *p == '\"' || 
			*p == '\'' || *p == '|' || *p == '<' || *p == '>') *p = '_';
		p=CharNext(p);
	}
	
	p=config_AACoutdir;
	if (p[0]) while (p[1]) p++;
	
	if (!config_AACoutdir[0] || config_AACoutdir[0] == ' ')
		Config(out.hMainWindow);
	if (!config_AACoutdir[0])
		wsprintf(lpstrFilename,"%s.aac",temp2);
	else if (p[0]=='\\')
		wsprintf(lpstrFilename,"%s%s.aac",config_AACoutdir,temp2);
	else
		wsprintf(lpstrFilename,"%s\\%s.aac",config_AACoutdir,temp2);
}

// *********************************************************************************************
//									Main functions
// *********************************************************************************************

void Init()
{
}
// *********************************************************************************************

void Quit()
{
}
// *********************************************************************************************

#define ERROR_O(msg) \
{ \
	if(msg) \
		MessageBox(0, msg, "FAAC plugin", MB_OK); \
	Close(); \
	return -1; \
}

int Open(int lSamprate, int wChannels, int wBitsPerSample, int bufferlenms, int prebufferms)
{
MY_ENC_CFG		cfg;
char			lpstrFilename[MAX_PATH];

	w_offset = writtentime = 0;
	numchan = wChannels;
	srate = lSamprate;
	bps = wBitsPerSample;

	ReadCfgEnc();
	Cfaac::getFaacCfg(&cfg);

	strcpy(config_AACoutdir,OutDir);
	GetNewFileName(lpstrFilename);

	Cpcmaac=new Cfaac();
	if(!Cpcmaac->Init(lpstrFilename,lSamprate,wBitsPerSample,wChannels,-1))
		ERROR_O(0);

	return 0;
}
// *********************************************************************************************

void Close()
{
	if(Cpcmaac)
	{
		delete Cpcmaac;
		Cpcmaac=NULL;
	}
}
// *********************************************************************************************

int Write(char *wabuf, int len)
{
	writtentime+=len;

	if(Cpcmaac->processDataBufferized(Cpcmaac->hOutput,(BYTE *)wabuf,len)<0)
		return -1;

//	Sleep(10);
	return 0;
}
// *********************************************************************************************

int CanWrite()
{
	return last_pause ? 0 : 16*1024*1024;
//	return last_pause ? 0 : mo->samplesInput*(mo->wBitsPerSample>>3);
}
// *********************************************************************************************

int IsPlaying()
{
	return 0;
}
// *********************************************************************************************

int Pause(int pause)
{
	int t=last_pause;
	last_pause=pause;
	return t;
}
// *********************************************************************************************

void SetVolume(int volume)
{
}
// *********************************************************************************************

void SetPan(int pan)
{
}
// *********************************************************************************************

void Flush(int t)
{
int a;

	  w_offset=0;
	  a = t - GetWrittenTime();
	  w_offset=a;
}
// *********************************************************************************************

int GetOutputTime()
{
int t=srate*numchan,
	ms=writtentime,
	l;

	if(t)
	{
		l=ms%t;
		ms /= t;
		ms *= 1000;
		ms += (l*1000)/t;
		if (bps == 16) ms/=2;
	}
	return ms + w_offset;
}
// *********************************************************************************************
	
int GetWrittenTime()
{
int t=srate*numchan,
	ms=writtentime,
	l;

	if(t)
	{
		l=ms%t;
		ms /= t;
		ms *= 1000;
		ms += (l*1000)/t;
		if (bps == 16) ms/=2;
	}
	return ms + w_offset;
}
