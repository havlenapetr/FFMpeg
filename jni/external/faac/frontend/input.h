/*
 * FAAC - Freeware Advanced Audio Coder
 * Copyright (C) 2002 Krzysztof Nikiel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: input.h,v 1.5 2003/08/17 19:38:15 menno Exp $
 */

#ifndef _INPUT_H
#define _INPUT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

#ifndef HAVE_INT32_T
typedef int int32_t;
#endif
#ifndef HAVE_INT16_T
typedef short int16_t;
#endif
#ifndef HAVE_U_INT32_T
typedef unsigned int u_int32_t;
#endif
#ifndef HAVE_U_INT16_T
typedef unsigned short u_int16_t;
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
  FILE *f;
  int channels;
  int samplebytes;
  int samplerate;
  int samples;
  int bigendian;
  int isfloat;
} pcmfile_t;

pcmfile_t *wav_open_read(const char *path, int rawchans);
size_t wav_read_float32(pcmfile_t *sndf, float *buf, size_t num, int *map);
size_t wav_read_int24(pcmfile_t *sndf, int32_t *buf, size_t num, int *map);
int wav_close(pcmfile_t *file);

#ifdef __cplusplus
}
#endif
#endif /* _INPUT_H */
