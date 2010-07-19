/*
FAAC - codec plugin for Cooledit
Copyright (C) 2004 Antonio Foranna

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

#ifndef _Cfaac_H
#define _Cfaac_H

// *********************************************************************************************

#include <mp4.h>		// int32_t, ...
#include <faad.h>		// FAAD2 version
#ifdef MAIN
	#undef MAIN
#endif
#ifdef SSR
	#undef SSR
#endif
#ifdef LTP
	#undef LTP
#endif
#include <faac.h>
#include <win32_ver.h>	// mpeg4ip version
#include "CRegistry.h"
#include "Defines.h"	// my defines

// *********************************************************************************************

#ifdef	ADTS
#undef	ADTS
#define ADTS 1
#endif

// *********************************************************************************************

typedef struct mec
{
bool					AutoCfg,
						UseQuality,
						SaveMP4;
faacEncConfiguration	EncCfg;
} MY_ENC_CFG;
// -----------------------------------------------------------------------------------------------

typedef struct output_tag  // any special vars associated with output file
{
// MP4
MP4FileHandle 	MP4File;
MP4TrackId		MP4track;
MP4Duration		TotalSamples,
				WrittenSamples,
				encoded_samples;
DWORD			frameSize,
				ofs;

// AAC
FILE			*aacFile;

// GLOBAL
long			Samprate;
WORD			BitsPerSample;
WORD			Channels;
DWORD			srcSize;
//char			*dst_name;		// name of compressed file

faacEncHandle	hEncoder;
int32_t			*buf32bit;
BYTE			*bufIn;
unsigned char	*bitbuf;
long			bytes_into_buffer;
DWORD			maxBytesOutput;
long			samplesInput,
				samplesInputSize;
bool			WriteMP4;
} MYOUTPUT;



// *********************************************************************************************



class Cfaac
{
private:
	virtual void DisplayError(char *ProcName, char *str);
	virtual HANDLE ERROR_Init(char *str) { DisplayError("Init", str); return NULL; }
	virtual int ERROR_processData(char *str) { DisplayError("processData", str); return -1; }
	virtual void showInfo(MYOUTPUT *mi) {}
	virtual void showProgress(MYOUTPUT *mi) {}
	virtual void To32bit(int32_t *buf, BYTE *bufi, int size, BYTE samplebytes, BYTE bigendian);

public:
    Cfaac(HANDLE hOutput=NULL);
    virtual ~Cfaac();

	static void getFaacCfg(MY_ENC_CFG *cfg);
	static void setFaacCfg(MY_ENC_CFG *cfg);
    virtual HANDLE Init(LPSTR lpstrFilename,long lSamprate,WORD wBitsPerSample,WORD wChannels,long FileSize);
    virtual int processData(HANDLE hOutput, BYTE *bufIn, DWORD len);
	virtual int processDataBufferized(HANDLE hOutput, BYTE *bufIn, long lBytes);
/*
// AAC
	bool            BlockSeeking;

// GLOBAL
	long            newpos_ms;
	BOOL            IsSeekable;
	MYINPUT			*mi;
*/
	HANDLE			hOutput;
};

#endif
