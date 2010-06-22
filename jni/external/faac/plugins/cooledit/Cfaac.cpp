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

#include "Cfaac.h"



// *********************************************************************************************



#define FREE_ARRAY(ptr) \
{ \
	if(ptr) \
		free(ptr); \
	ptr=0; \
}

// *********************************************************************************************

Cfaac::Cfaac(HANDLE hOut)
{
	if(hOut)
	{
		hOutput=hOut;
		return;
	}

    if(!(hOutput=GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE|GMEM_ZEROINIT,sizeof(MYOUTPUT))))
		MessageBox(0, "Memory allocation error: hOutput", APP_NAME " plugin", MB_OK|MB_ICONSTOP); \
/*
MYOUTPUT *mo;

	if(!(mo=(MYOUTPUT *)GlobalLock(hOutput)))
		MessageBox(0, "GlobalLock(hOutput)", APP_NAME " plugin", MB_OK|MB_ICONSTOP); \

	GlobalUnlock(hOutput);*/
}
// -----------------------------------------------------------------------------------------------

Cfaac::~Cfaac()
{
	if(!hOutput)
		return;

MYOUTPUT *mo;

	GLOBALLOCK(mo,hOutput,MYOUTPUT,return);
	
	if(mo->WrittenSamples)
	{
	int	BytesWritten;
		if(mo->bytes_into_buffer>0)
			memset(mo->bufIn+mo->bytes_into_buffer, 0, (mo->samplesInput*(mo->BitsPerSample>>3))-mo->bytes_into_buffer);
		do
		{
			if((BytesWritten=processData(hOutput,mo->bufIn,mo->bytes_into_buffer))<0)
				MessageBox(0, "~Cfaac: processData", APP_NAME " plugin", MB_OK|MB_ICONSTOP);
			mo->bytes_into_buffer=0;
		}while(BytesWritten>0);
	}

	if(mo->aacFile)
	{
		fclose(mo->aacFile);
		mo->aacFile=0;
	}
	else
	{
		MP4Close(mo->MP4File);
		mo->MP4File=0;
	}
	
	if(mo->hEncoder)
		faacEncClose(mo->hEncoder);
	
	FREE_ARRAY(mo->bitbuf)
	FREE_ARRAY(mo->buf32bit)
	FREE_ARRAY(mo->bufIn)
	
	GlobalUnlock(hOutput);
	GlobalFree(hOutput);
}

// *********************************************************************************************
//									Utilities
// *********************************************************************************************

#define SWAP32(x) (((x & 0xff) << 24) | ((x & 0xff00) << 8) \
	| ((x & 0xff0000) >> 8) | ((x & 0xff000000) >> 24))
#define SWAP16(x) (((x & 0xff) << 8) | ((x & 0xff00) >> 8))

void Cfaac::To32bit(int32_t *buf, BYTE *bufi, int size, BYTE samplebytes, BYTE bigendian)
{
int i;

	switch(samplebytes)
	{
	case 1:
		// this is endian clean
		for (i = 0; i < size; i++)
			buf[i] = (bufi[i] - 128) * 65536;
		break;
		
	case 2:
#ifdef WORDS_BIGENDIAN
		if (!bigendian)
#else
			if (bigendian)
#endif
			{
				// swap bytes
				for (i = 0; i < size; i++)
				{
					int16_t s = ((int16_t *)bufi)[i];
					
					s = SWAP16(s);
					
					buf[i] = ((u_int32_t)s) << 8;
				}
			}
			else
			{
				// no swap
				for (i = 0; i < size; i++)
				{
					int s = ((int16_t *)bufi)[i];
					
					buf[i] = s << 8;
				}
			}
			break;
			
	case 3:
		if (!bigendian)
		{
			for (i = 0; i < size; i++)
			{
				int s = bufi[3 * i] | (bufi[3 * i + 1] << 8) | (bufi[3 * i + 2] << 16);
				
				// fix sign
				if (s & 0x800000)
					s |= 0xff000000;
				
				buf[i] = s;
			}
		}
		else // big endian input
		{
			for (i = 0; i < size; i++)
			{
				int s = (bufi[3 * i] << 16) | (bufi[3 * i + 1] << 8) | bufi[3 * i + 2];
				
				// fix sign
				if (s & 0x800000)
					s |= 0xff000000;
				
				buf[i] = s;
			}
		}
		break;
		
	case 4:		
#ifdef WORDS_BIGENDIAN
		if (!bigendian)
#else
			if (bigendian)
#endif
			{
				// swap bytes
				for (i = 0; i < size; i++)
				{
					int s = bufi[i];
					
					buf[i] = SWAP32(s);
				}
			}
			else
				memcpy(buf,bufi,size*sizeof(u_int32_t));
		/*
		int exponent, mantissa;
		float *bufo=(float *)buf;
			
			for (i = 0; i < size; i++)
			{
				exponent=bufi[(i<<2)+3]<<1;
				if(bufi[i*4+2] & 0x80)
					exponent|=0x01;
				exponent-=126;
				mantissa=(DWORD)bufi[(i<<2)+2]<<16;
				mantissa|=(DWORD)bufi[(i<<2)+1]<<8;
				mantissa|=bufi[(i<<2)];
				bufo[i]=(float)ldexp(mantissa,exponent);
			}*/
			break;
	}
}

// *********************************************************************************************
//									Main functions
// *********************************************************************************************

void Cfaac::DisplayError(char *ProcName, char *str)
{
char buf[100]="";

	if(str && *str)
	{
		if(ProcName && *ProcName)
			sprintf(buf,"%s: ", ProcName);
		strcat(buf,str);
		MessageBox(0, buf, APP_NAME " plugin", MB_OK|MB_ICONSTOP);
	}

MYOUTPUT *mo;
	GLOBALLOCK(mo,hOutput,MYOUTPUT,return);
	mo->bytes_into_buffer=-1;
	GlobalUnlock(hOutput);
	GlobalUnlock(hOutput);
}
// *********************************************************************************************

void Cfaac::getFaacCfg(MY_ENC_CFG *cfg)
{ 
CRegistry reg;

	if(reg.openCreateReg(HKEY_LOCAL_MACHINE,REGISTRY_PROGRAM_NAME "\\FAAC"))
	{
		cfg->AutoCfg=reg.getSetRegBool("Auto",true);
		cfg->SaveMP4=reg.getSetRegBool("Write MP4",false);
		cfg->EncCfg.mpegVersion=reg.getSetRegDword("MPEG version",MPEG4); 
		cfg->EncCfg.aacObjectType=reg.getSetRegDword("Profile",LOW); 
		cfg->EncCfg.allowMidside=reg.getSetRegDword("MidSide",true); 
		cfg->EncCfg.useTns=reg.getSetRegDword("TNS",true); 
		cfg->EncCfg.useLfe=reg.getSetRegDword("LFE",false);
		cfg->UseQuality=reg.getSetRegBool("Use quality",false);
		cfg->EncCfg.quantqual=reg.getSetRegDword("Quality",100); 
		cfg->EncCfg.bitRate=reg.getSetRegDword("BitRate",0); 
		cfg->EncCfg.bandWidth=reg.getSetRegDword("BandWidth",0); 
		cfg->EncCfg.outputFormat=reg.getSetRegDword("Header",ADTS); 
	}
	else
		MessageBox(0,"Can't open registry!",0,MB_OK|MB_ICONSTOP);
}
// -----------------------------------------------------------------------------------------------

void Cfaac::setFaacCfg(MY_ENC_CFG *cfg)
{ 
CRegistry reg;

	if(reg.openCreateReg(HKEY_LOCAL_MACHINE,REGISTRY_PROGRAM_NAME "\\FAAC"))
	{
		reg.setRegBool("Auto",cfg->AutoCfg); 
		reg.setRegBool("Write MP4",cfg->SaveMP4); 
		reg.setRegDword("MPEG version",cfg->EncCfg.mpegVersion); 
		reg.setRegDword("Profile",cfg->EncCfg.aacObjectType); 
		reg.setRegDword("MidSide",cfg->EncCfg.allowMidside); 
		reg.setRegDword("TNS",cfg->EncCfg.useTns); 
		reg.setRegDword("LFE",cfg->EncCfg.useLfe); 
		reg.setRegBool("Use quality",cfg->UseQuality); 
		reg.setRegDword("Quality",cfg->EncCfg.quantqual); 
		reg.setRegDword("BitRate",cfg->EncCfg.bitRate); 
		reg.setRegDword("BandWidth",cfg->EncCfg.bandWidth); 
		reg.setRegDword("Header",cfg->EncCfg.outputFormat); 
	}
	else
		MessageBox(0,"Can't open registry!",0,MB_OK|MB_ICONSTOP);
}
// *********************************************************************************************

HANDLE Cfaac::Init(LPSTR lpstrFilename,long lSamprate,WORD wBitsPerSample,WORD wChannels,long FileSize)
{
MYOUTPUT	*mo;
MY_ENC_CFG	cfg;
DWORD		samplesInput,
			maxBytesOutput;

//	if(wBitsPerSample!=8 && wBitsPerSample!=16) // 32 bit audio from cooledit is in unsupported format
//		return 0;
	if(wChannels>48)	// FAAC supports max 48 tracks!
		return NULL;

	GLOBALLOCK(mo,hOutput,MYOUTPUT,return NULL);

	// open the encoder library
	if(!(mo->hEncoder=faacEncOpen(lSamprate, wChannels, &samplesInput, &maxBytesOutput)))
		return ERROR_Init("Can't open library");

	if(!(mo->bitbuf=(unsigned char *)malloc(maxBytesOutput*sizeof(unsigned char))))
		return ERROR_Init("Memory allocation error: output buffer");

	if(!(mo->bufIn=(BYTE *)malloc(samplesInput*sizeof(int32_t))))
		return ERROR_Init("Memory allocation error: input buffer");

	if(!(mo->buf32bit=(int32_t *)malloc(samplesInput*sizeof(int32_t))))
		return ERROR_Init("Memory allocation error: 32 bit buffer");


	getFaacCfg(&cfg);

	if(cfg.SaveMP4)
		if(!strcmpi(lpstrFilename+lstrlen(lpstrFilename)-4,".aac"))
			strcpy(lpstrFilename+lstrlen(lpstrFilename)-4,".mp4");
		else
			if(strcmpi(lpstrFilename+lstrlen(lpstrFilename)-4,".mp4"))
				strcat(lpstrFilename,".mp4");
	mo->WriteMP4=!strcmpi(lpstrFilename+lstrlen(lpstrFilename)-4,".mp4");

	if(cfg.AutoCfg)
	{
	faacEncConfigurationPtr myFormat=&cfg.EncCfg;
	faacEncConfigurationPtr CurFormat=faacEncGetCurrentConfiguration(mo->hEncoder);
		if(mo->WriteMP4)
			CurFormat->outputFormat=RAW;
		CurFormat->useLfe=wChannels>=6 ? 1 : 0;
		if(!faacEncSetConfiguration(mo->hEncoder, CurFormat))
			return ERROR_Init("Unsupported parameters!");
	}
	else
	{
	faacEncConfigurationPtr myFormat=&cfg.EncCfg;
	faacEncConfigurationPtr CurFormat=faacEncGetCurrentConfiguration(mo->hEncoder);

		if(cfg.UseQuality)
		{
			CurFormat->quantqual=myFormat->quantqual;
			CurFormat->bitRate=myFormat->bitRate;
		}
		else
			if(!CurFormat->bitRate)
				CurFormat->bitRate=myFormat->bitRate;
			else
				CurFormat->bitRate*=1000;

		switch(CurFormat->bandWidth)
		{
		case 0:
			break;
		case 0xffffffff:
			CurFormat->bandWidth=lSamprate/2;
			break;
		default:
			CurFormat->bandWidth=myFormat->bandWidth;
			break;
		}
/*
		switch(wBitsPerSample)
		{
		case 16:
			CurFormat->inputFormat=FAAC_INPUT_16BIT;
			break;
		case 24:
			CurFormat->inputFormat=FAAC_INPUT_24BIT;
			break;
		case 32:
			CurFormat->inputFormat=FAAC_INPUT_32BIT;
			break;
		default:
			CurFormat->inputFormat=FAAC_INPUT_NULL;
			break;
		}
*/
		CurFormat->mpegVersion=myFormat->mpegVersion;
		CurFormat->outputFormat=mo->WriteMP4 ? 0 : myFormat->outputFormat;
		CurFormat->mpegVersion=myFormat->mpegVersion;
		CurFormat->aacObjectType=myFormat->aacObjectType;
		CurFormat->allowMidside=myFormat->allowMidside;
		CurFormat->useTns=myFormat->useTns;
		CurFormat->useLfe=wChannels>=6 ? 1 : 0;

		if(!faacEncSetConfiguration(mo->hEncoder, CurFormat))
			return ERROR_Init("Unsupported parameters!");
	}

//	mo->src_size=lSize;
//	mi->dst_name=strdup(lpstrFilename);
	mo->Samprate=lSamprate;
	mo->BitsPerSample=wBitsPerSample;
	mo->Channels=wChannels;
	mo->samplesInput=samplesInput;
	mo->samplesInputSize=samplesInput*(mo->BitsPerSample>>3);

	mo->maxBytesOutput=maxBytesOutput;

    if(mo->WriteMP4) // Create MP4 file --------------------------------------------------------------------------
	{
    BYTE *ASC=0;
    DWORD ASCLength=0;

        if((mo->MP4File=MP4Create(lpstrFilename, 0, 0, 0))==MP4_INVALID_FILE_HANDLE)
			return ERROR_Init("Can't create file");
        MP4SetTimeScale(mo->MP4File, 90000);
        mo->MP4track=MP4AddAudioTrack(mo->MP4File, lSamprate, MP4_INVALID_DURATION, MP4_MPEG4_AUDIO_TYPE);
        MP4SetAudioProfileLevel(mo->MP4File, 0x0F);
        faacEncGetDecoderSpecificInfo(mo->hEncoder, &ASC, &ASCLength);
        MP4SetTrackESConfiguration(mo->MP4File, mo->MP4track, (unsigned __int8 *)ASC, ASCLength);
		mo->frameSize=samplesInput/wChannels;
		mo->ofs=mo->frameSize;
    }
	else // Create AAC file -----------------------------------------------------------------------------
	{
		// open the aac output file 
		if(!(mo->aacFile=fopen(lpstrFilename, "wb")))
			return ERROR_Init("Can't create file");

		// use bufferized stream
		setvbuf(mo->aacFile,NULL,_IOFBF,32767);
	}

	showInfo(mo);

	GlobalUnlock(hOutput);
    return hOutput;
}
// *********************************************************************************************

int Cfaac::processData(HANDLE hOutput, BYTE *bufIn, DWORD len)
{
	if(!hOutput)
		return -1;

int bytesWritten=0;
int bytesEncoded;
MYOUTPUT far *mo;

	GLOBALLOCK(mo,hOutput,MYOUTPUT,return 0);

int32_t *buf=mo->buf32bit;

	if((int)len<mo->samplesInputSize)
	{
		mo->samplesInput=(len<<3)/mo->BitsPerSample;
		mo->samplesInputSize=mo->samplesInput*(mo->BitsPerSample>>3);
	}
	To32bit(buf,bufIn,mo->samplesInput,mo->BitsPerSample>>3,false);

	// call the actual encoding routine
	if((bytesEncoded=faacEncEncode(mo->hEncoder, (int32_t *)buf, mo->samplesInput, mo->bitbuf, mo->maxBytesOutput))<0)
		return ERROR_processData("faacEncEncode()");

	// write bitstream to aac file 
	if(mo->aacFile)
	{
		if(bytesEncoded>0)
		{
			if((bytesWritten=fwrite(mo->bitbuf, 1, bytesEncoded, mo->aacFile))!=bytesEncoded)
				return ERROR_processData("fwrite");
			mo->WrittenSamples=1; // needed into destructor
		}
	}
	else
	// write bitstream to mp4 file
	{
	MP4Duration dur,
				SamplesLeft;
		if(len>0)
		{
			mo->srcSize+=len;
			dur=mo->frameSize;
		}
		else
		{
			mo->TotalSamples=(mo->srcSize<<3)/(mo->BitsPerSample*mo->Channels);
			SamplesLeft=(mo->TotalSamples-mo->WrittenSamples)+mo->frameSize;
			dur=SamplesLeft>mo->frameSize ? mo->frameSize : SamplesLeft;
		}
		if(bytesEncoded>0)
		{
			if(!(bytesWritten=MP4WriteSample(mo->MP4File, mo->MP4track, (unsigned __int8 *)mo->bitbuf, (DWORD)bytesEncoded, dur, mo->ofs, true) ? bytesEncoded : -1))
				return ERROR_processData("MP4WriteSample");
			mo->ofs=0;
			mo->WrittenSamples+=dur;
		}
	}

	showProgress(mo);

	GlobalUnlock(hOutput);
	return bytesWritten;
}
// -----------------------------------------------------------------------------------------------

int Cfaac::processDataBufferized(HANDLE hOutput, BYTE *bufIn, long lBytes)
{
	if(!hOutput)
		return -1;

int	bytesWritten=0, tot=0;
MYOUTPUT far *mo;

	GLOBALLOCK(mo,hOutput,MYOUTPUT,return 0);

	if(mo->bytes_into_buffer>=0)
		do
		{
			if(mo->bytes_into_buffer+lBytes<mo->samplesInputSize)
			{
				memmove(mo->bufIn+mo->bytes_into_buffer, bufIn, lBytes);
				mo->bytes_into_buffer+=lBytes;
				lBytes=0;
			}
			else
			{
			int	shift=mo->samplesInputSize-mo->bytes_into_buffer;
				memmove(mo->bufIn+mo->bytes_into_buffer, bufIn, shift);
				mo->bytes_into_buffer+=shift;
				bufIn+=shift;
				lBytes-=shift;

				tot+=bytesWritten=processData(hOutput,mo->bufIn,mo->bytes_into_buffer);
				if(bytesWritten<0)
					return ERROR_processData(0);
				mo->bytes_into_buffer=0;
			}
		}while(lBytes);

	GlobalUnlock(hOutput);
	return tot;
}
