/*
 * FAAC - Freeware Advanced Audio Coder
 * Copyright (C) 2001 Menno Bakker
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: psych.h,v 1.13 2003/11/16 05:02:52 stux Exp $
 */

#ifndef PSYCH_H
#define PSYCH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#include "coder.h"
#include "channels.h"
#include "fft.h"

typedef struct {
	int size;
	int sizeS;

	/* Previous input samples */
	double *prevSamples;
	double *prevSamplesS;

	int block_type;

        void *data;
} PsyInfo;

typedef struct {
	double sampleRate;

	/* Hann window */
	double *hannWindow;
	double *hannWindowS;

        void *data;
} GlobalPsyInfo;

typedef struct 
{
void (*PsyInit) (GlobalPsyInfo *gpsyInfo, PsyInfo *psyInfo,
		unsigned int numChannels, unsigned int sampleRate,
		int *cb_width_long, int num_cb_long,
		int *cb_width_short, int num_cb_short);
void (*PsyEnd) (GlobalPsyInfo *gpsyInfo, PsyInfo *psyInfo,
		unsigned int numChannels);
void (*PsyCalculate) (ChannelInfo *channelInfo, GlobalPsyInfo *gpsyInfo,
		PsyInfo *psyInfo, int *cb_width_long, int num_cb_long,
		int *cb_width_short, int num_cb_short,
		unsigned int numChannels);
void (*PsyBufferUpdate) ( FFT_Tables *fft_tables, GlobalPsyInfo * gpsyInfo, PsyInfo * psyInfo,
		double *newSamples, unsigned int bandwidth,
		int *cb_width_short, int num_cb_short);
void (*BlockSwitch) (CoderInfo *coderInfo, PsyInfo *psyInfo,
		unsigned int numChannels);
} psymodel_t;

extern psymodel_t psymodel2;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PSYCH_H */