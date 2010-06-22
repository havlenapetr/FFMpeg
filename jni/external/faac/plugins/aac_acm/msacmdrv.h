#ifndef _MSACMDRV_H
#define _MSACMDRV_H

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <vfw.h>
#include <msacm.h>

#define ACMDM_DRIVER_DETAILS            (ACMDM_BASE + 10)
#define ACMDM_DRIVER_ABOUT	        (ACMDM_BASE + 11)
#define ACMDM_FORMATTAG_DETAILS         (ACMDM_BASE + 25)
#define ACMDM_FORMAT_DETAILS            (ACMDM_BASE + 26)
#define ACMDM_FORMAT_SUGGEST            (ACMDM_BASE + 27)

#define ACMDM_STREAM_OPEN               (ACMDM_BASE + 76)
#define ACMDM_STREAM_CLOSE              (ACMDM_BASE + 77)
#define ACMDM_STREAM_SIZE               (ACMDM_BASE + 78)
#define ACMDM_STREAM_CONVERT            (ACMDM_BASE + 79)
#define ACMDM_STREAM_RESET              (ACMDM_BASE + 80)
#define ACMDM_STREAM_PREPARE            (ACMDM_BASE + 81)
#define ACMDM_STREAM_UNPREPARE          (ACMDM_BASE + 82)
#define ACMDM_STREAM_UPDATE             (ACMDM_BASE + 83)

typedef struct _ACMDRVOPENDESCA
{
  DWORD  cbStruct;
  FOURCC fccType;
  FOURCC fccComp;
  DWORD  dwVersion;
  DWORD  dwFlags;
  DWORD  dwError;
  LPCSTR pszSectionName;
  LPCSTR pszAliasName;
  DWORD  dnDevNode;
} ACMDRVOPENDESCA, *PACMDRVOPENDESCA;

typedef struct _ACMDRVOPENDESCW
{
  DWORD   cbStruct;
  FOURCC  fccType;
  FOURCC  fccComp;
  DWORD   dwVersion;
  DWORD   dwFlags;
  DWORD   dwError;
  LPCWSTR pszSectionName;
  LPCWSTR pszAliasName;
  DWORD   dnDevNode;
} ACMDRVOPENDESCW, *PACMDRVOPENDESCW;

typedef struct _ACMDRVOPENDESC16
{
  DWORD  cbStruct;
  FOURCC fccType;
  FOURCC fccComp;
  DWORD  dwVersion;
  DWORD  dwFlags;
  DWORD  dwError;
  LPCSTR pszSectionName;
  LPCSTR pszAliasName;
  DWORD  dnDevNode;
} ACMDRVOPENDESC16, *NPACMDRVOPENDESC16, *LPACMDRVOPENDESC16;
/*
typedef struct _ACMDRVSTREAMINSTANCE16
{
  DWORD            cbStruct;
  LPWAVEFORMATEX   pwfxSrc;
  LPWAVEFORMATEX   pwfxDst;
  LPWAVEFILTER     pwfltr;
  DWORD            dwCallback;
  DWORD            dwInstance;
  DWORD            fdwOpen;
  DWORD            fdwDriver;
  DWORD            dwDriver;
  HACMSTREAM16     has;
} ACMDRVSTREAMINSTANCE16, *NPACMDRVSTREAMINSTANCE16, *LPACMDRVSTREAMINSTANCE16;
*/
typedef struct _ACMDRVSTREAMINSTANCE
{
  DWORD           cbStruct;
  PWAVEFORMATEX   pwfxSrc;
  PWAVEFORMATEX   pwfxDst;
  PWAVEFILTER     pwfltr;
  DWORD           dwCallback;
  DWORD           dwInstance;
  DWORD           fdwOpen;
  DWORD           fdwDriver;
  DWORD           dwDriver;
  HACMSTREAM    has;
} ACMDRVSTREAMINSTANCE, *PACMDRVSTREAMINSTANCE;


typedef struct _ACMDRVSTREAMHEADER16 *LPACMDRVSTREAMHEADER16;
typedef struct _ACMDRVSTREAMHEADER16 {
  DWORD  cbStruct;
  DWORD  fdwStatus;
  DWORD  dwUser;
  LPBYTE pbSrc;
  DWORD  cbSrcLength;
  DWORD  cbSrcLengthUsed;
  DWORD  dwSrcUser;
  LPBYTE pbDst;
  DWORD  cbDstLength;
  DWORD  cbDstLengthUsed;
  DWORD  dwDstUser;

  DWORD fdwConvert;
  LPACMDRVSTREAMHEADER16 *padshNext;
  DWORD fdwDriver;
  DWORD dwDriver;

  /* Internal fields for ACM */
  DWORD  fdwPrepared;
  DWORD  dwPrepared;
  LPBYTE pbPreparedSrc;
  DWORD  cbPreparedSrcLength;
  LPBYTE pbPreparedDst;
  DWORD  cbPreparedDstLength;
} ACMDRVSTREAMHEADER16, *NPACMDRVSTREAMHEADER16;

typedef struct _ACMDRVSTREAMHEADER *PACMDRVSTREAMHEADER;
typedef struct _ACMDRVSTREAMHEADER {
  DWORD  cbStruct;
  DWORD  fdwStatus;
  DWORD  dwUser;
  LPBYTE pbSrc;
  DWORD  cbSrcLength;
  DWORD  cbSrcLengthUsed;
  DWORD  dwSrcUser;
  LPBYTE pbDst;
  DWORD  cbDstLength;
  DWORD  cbDstLengthUsed;
  DWORD  dwDstUser;

  DWORD fdwConvert;
  PACMDRVSTREAMHEADER *padshNext;
  DWORD fdwDriver;
  DWORD dwDriver;

  /* Internal fields for ACM */
  DWORD  fdwPrepared;
  DWORD  dwPrepared;
  LPBYTE pbPreparedSrc;
  DWORD  cbPreparedSrcLength;
  LPBYTE pbPreparedDst;
  DWORD  cbPreparedDstLength;
} ACMDRVSTREAMHEADER;

typedef struct _ACMDRVSTREAMSIZE
{
  DWORD cbStruct;
  DWORD fdwSize;
  DWORD cbSrcLength;
  DWORD cbDstLength;
} ACMDRVSTREAMSIZE16, *NPACMDRVSTREAMSIZE16, *LPACMDRVSTREAMSIZE16,
  ACMDRVSTREAMSIZE, *PACMDRVSTREAMSIZE;

typedef struct _ACMDRVFORMATSUGGEST16
{
  DWORD            cbStruct;
  DWORD            fdwSuggest;
  LPWAVEFORMATEX   pwfxSrc;
  DWORD            cbwfxSrc;
  LPWAVEFORMATEX   pwfxDst;
  DWORD            cbwfxDst;
} ACMDRVFORMATSUGGEST16, *NPACMDRVFORMATSUGGEST, *LPACMDRVFORMATSUGGEST;

typedef struct _ACMDRVFORMATSUGGEST
{
  DWORD           cbStruct;
  DWORD           fdwSuggest;
  PWAVEFORMATEX   pwfxSrc;
  DWORD           cbwfxSrc;
  PWAVEFORMATEX   pwfxDst;
  DWORD           cbwfxDst;
} ACMDRVFORMATSUGGEST, *PACMDRVFORMATSUGGEST;

#endif
