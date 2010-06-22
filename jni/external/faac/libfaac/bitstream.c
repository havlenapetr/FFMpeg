/**********************************************************************

This software module was originally developed by
and edited by Texas Instruments in the course of
development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1997.
**********************************************************************/
/*
 * $Id: bitstream.c,v 1.27 2003/10/14 15:23:44 menno Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#include "coder.h"
#include "channels.h"
#include "huffman.h"
#include "bitstream.h"
#include "ltp.h"
#include "util.h"

static int CountBitstream(faacEncHandle hEncoder,
                          CoderInfo *coderInfo,
                          ChannelInfo *channelInfo,
                          BitStream *bitStream,
                          int numChannels);
static int WriteADTSHeader(faacEncHandle hEncoder,
                           BitStream *bitStream,
                           int writeFlag);
static int WriteCPE(CoderInfo *coderInfoL,
                    CoderInfo *coderInfoR,
                    ChannelInfo *channelInfo,
                    BitStream* bitStream,
                    int objectType,
                    int writeFlag);
static int WriteSCE(CoderInfo *coderInfo,
                    ChannelInfo *channelInfo,
                    BitStream *bitStream,
                    int objectType,
                    int writeFlag);
static int WriteLFE(CoderInfo *coderInfo,
                    ChannelInfo *channelInfo,
                    BitStream *bitStream,
                    int objectType,
                    int writeFlag);
static int WriteICSInfo(CoderInfo *coderInfo,
                        BitStream *bitStream,
                        int objectType,
                        int common_window,
                        int writeFlag);
static int WriteICS(CoderInfo *coderInfo,
                    BitStream *bitStream,
                    int commonWindow,
                    int objectType,
                    int writeFlag);
static int WriteLTPPredictorData(CoderInfo *coderInfo,
                                 BitStream *bitStream,
                                 int writeFlag);
static int WritePredictorData(CoderInfo *coderInfo,
                              BitStream *bitStream,
                              int writeFlag);
static int WritePulseData(CoderInfo *coderInfo,
                          BitStream *bitStream,
                          int writeFlag);
static int WriteTNSData(CoderInfo *coderInfo,
                        BitStream *bitStream,
                        int writeFlag);
static int WriteGainControlData(CoderInfo *coderInfo,
                                BitStream *bitStream,
                                int writeFlag);
static int WriteSpectralData(CoderInfo *coderInfo,
                             BitStream *bitStream,
                             int writeFlag);
static int WriteAACFillBits(BitStream* bitStream,
                            int numBits,
                            int writeFlag);
static int FindGroupingBits(CoderInfo *coderInfo);
static long BufferNumBit(BitStream *bitStream);
static int WriteByte(BitStream *bitStream,
                     unsigned long data,
                     int numBit);
static int ByteAlign(BitStream* bitStream,
                     int writeFlag);



static int WriteFAACStr(BitStream *bitStream, char *version, int write)
{
  int i;
  char str[200];
  int len, padbits, count;
  int bitcnt;

  sprintf(str, "libfaac %s", version);

  len = strlen(str) + 1;
  padbits = (8 - ((bitStream->numBit + 7) % 8)) % 8;
  count = len + 3;

  bitcnt = LEN_SE_ID + 4 + ((count < 15) ? 0 : 8) + count * 8;
  if (!write)
    return bitcnt;

  PutBit(bitStream, ID_FIL, LEN_SE_ID);
  if (count < 15)
  {
    PutBit(bitStream, count, 4);
  }
  else
  {
    PutBit(bitStream, 15, 4);
    PutBit(bitStream, count - 14, 8);
  }

  PutBit(bitStream, 0, padbits);
  PutBit(bitStream, 0, 8);
  PutBit(bitStream, 0, 8); // just in case
  for (i = 0; i < len; i++)
    PutBit(bitStream, str[i], 8);

  PutBit(bitStream, 0, 8 - padbits);

  return bitcnt;
}


int WriteBitstream(faacEncHandle hEncoder,
                   CoderInfo *coderInfo,
                   ChannelInfo *channelInfo,
                   BitStream *bitStream,
                   int numChannel)
{
    int channel;
    int bits = 0;
    int bitsLeftAfterFill, numFillBits;

    CountBitstream(hEncoder, coderInfo, channelInfo, bitStream, numChannel);

    if(hEncoder->config.outputFormat == 1){
        bits += WriteADTSHeader(hEncoder, bitStream, 1);
    }else{
        bits = 0; // compilier will remove it, byt anyone will see that current size of bitstream is 0
    }

    if (hEncoder->frameNum == 4)
      WriteFAACStr(bitStream, hEncoder->config.name, 1);

    for (channel = 0; channel < numChannel; channel++) {

        if (channelInfo[channel].present) {

            /* Write out a single_channel_element */
            if (!channelInfo[channel].cpe) {

                if (channelInfo[channel].lfe) {
                    /* Write out lfe */
                    bits += WriteLFE(&coderInfo[channel],
                        &channelInfo[channel],
                        bitStream,
                        hEncoder->config.aacObjectType,
                        1);
                } else {
                    /* Write out sce */
                    bits += WriteSCE(&coderInfo[channel],
                        &channelInfo[channel],
                        bitStream,
                        hEncoder->config.aacObjectType,
                        1);
                }

            } else {

                if (channelInfo[channel].ch_is_left) {
                    /* Write out cpe */
                    bits += WriteCPE(&coderInfo[channel],
                        &coderInfo[channelInfo[channel].paired_ch],
                        &channelInfo[channel],
                        bitStream,
                        hEncoder->config.aacObjectType,
                        1);
                }
            }
        }
    }

    /* Compute how many fill bits are needed to avoid overflowing bit reservoir */
    /* Save room for ID_END terminator */
    if (bits < (8 - LEN_SE_ID) ) {
        numFillBits = 8 - LEN_SE_ID - bits;
    } else {
        numFillBits = 0;
    }

    /* Write AAC fill_elements, smallest fill element is 7 bits. */
    /* Function may leave up to 6 bits left after fill, so tell it to fill a few extra */
    numFillBits += 6;
    bitsLeftAfterFill = WriteAACFillBits(bitStream, numFillBits, 1);
    bits += (numFillBits - bitsLeftAfterFill);

    /* Write ID_END terminator */
    bits += LEN_SE_ID;
    PutBit(bitStream, ID_END, LEN_SE_ID);

    /* Now byte align the bitstream */
    /*
     * This byte_alignment() is correct for both MPEG2 and MPEG4, although
     * in MPEG4 the byte_alignment() is officially done before the new frame
     * instead of at the end. But this is basically the same.
     */
    bits += ByteAlign(bitStream, 1);

    return bits;
}

static int CountBitstream(faacEncHandle hEncoder,
                          CoderInfo *coderInfo,
                          ChannelInfo *channelInfo,
                          BitStream *bitStream,
                          int numChannel)
{
    int channel;
    int bits = 0;
    int bitsLeftAfterFill, numFillBits;


    if(hEncoder->config.outputFormat == 1){
        bits += WriteADTSHeader(hEncoder, bitStream, 0);
    }else{
        bits = 0; // compilier will remove it, byt anyone will see that current size of bitstream is 0
    }

    if (hEncoder->frameNum == 4)
      bits += WriteFAACStr(bitStream, hEncoder->config.name, 0);

    for (channel = 0; channel < numChannel; channel++) {

        if (channelInfo[channel].present) {

            /* Write out a single_channel_element */
            if (!channelInfo[channel].cpe) {

                if (channelInfo[channel].lfe) {
                    /* Write out lfe */
                    bits += WriteLFE(&coderInfo[channel],
                        &channelInfo[channel],
                        bitStream,
                        hEncoder->config.aacObjectType,
                        0);
                } else {
                    /* Write out sce */
                    bits += WriteSCE(&coderInfo[channel],
                        &channelInfo[channel],
                        bitStream,
                        hEncoder->config.aacObjectType,
                        0);
                }

            } else {

                if (channelInfo[channel].ch_is_left) {
                    /* Write out cpe */
                    bits += WriteCPE(&coderInfo[channel],
                        &coderInfo[channelInfo[channel].paired_ch],
                        &channelInfo[channel],
                        bitStream,
                        hEncoder->config.aacObjectType,
                        0);
                }
            }
        }
    }

    /* Compute how many fill bits are needed to avoid overflowing bit reservoir */
    /* Save room for ID_END terminator */
    if (bits < (8 - LEN_SE_ID) ) {
        numFillBits = 8 - LEN_SE_ID - bits;
    } else {
        numFillBits = 0;
    }

    /* Write AAC fill_elements, smallest fill element is 7 bits. */
    /* Function may leave up to 6 bits left after fill, so tell it to fill a few extra */
    numFillBits += 6;
    bitsLeftAfterFill = WriteAACFillBits(bitStream, numFillBits, 0);
    bits += (numFillBits - bitsLeftAfterFill);

    /* Write ID_END terminator */
    bits += LEN_SE_ID;

    /* Now byte align the bitstream */
    bits += ByteAlign(bitStream, 0);

    hEncoder->usedBytes = bit2byte(bits);

    return bits;
}

static int WriteADTSHeader(faacEncHandle hEncoder,
                           BitStream *bitStream,
                           int writeFlag)
{
    int bits = 56;

    if (writeFlag) {
        /* Fixed ADTS header */
        PutBit(bitStream, 0xFFFF, 12); /* 12 bit Syncword */
        PutBit(bitStream, hEncoder->config.mpegVersion, 1); /* ID == 0 for MPEG4 AAC, 1 for MPEG2 AAC */
        PutBit(bitStream, 0, 2); /* layer == 0 */
        PutBit(bitStream, 1, 1); /* protection absent */
        PutBit(bitStream, hEncoder->config.aacObjectType, 2); /* profile */
        PutBit(bitStream, hEncoder->sampleRateIdx, 4); /* sampling rate */
        PutBit(bitStream, 0, 1); /* private bit */
        PutBit(bitStream, hEncoder->numChannels, 3); /* ch. config (must be > 0) */
                                                     /* simply using numChannels only works for
                                                        6 channels or less, else a channel
                                                        configuration should be written */
        PutBit(bitStream, 0, 1); /* original/copy */
        PutBit(bitStream, 0, 1); /* home */

#if 0 // Removed in corrigendum 14496-3:2002
        if (hEncoder->config.mpegVersion == 0)
            PutBit(bitStream, 0, 2); /* emphasis */
#endif

        /* Variable ADTS header */
        PutBit(bitStream, 0, 1); /* copyr. id. bit */
        PutBit(bitStream, 0, 1); /* copyr. id. start */
        PutBit(bitStream, hEncoder->usedBytes, 13);
        PutBit(bitStream, 0x7FF, 11); /* buffer fullness (0x7FF for VBR) */
        PutBit(bitStream, 0, 2); /* raw data blocks (0+1=1) */

    }

    /*
     * MPEG2 says byte_aligment() here, but ADTS always is multiple of 8 bits
     * MPEG4 has no byte_alignment() here
     */
    /*
    if (hEncoder->config.mpegVersion == 1)
        bits += ByteAlign(bitStream, writeFlag);
    */

#if 0 // Removed in corrigendum 14496-3:2002
    if (hEncoder->config.mpegVersion == 0)
        bits += 2; /* emphasis */
#endif

    return bits;
}

static int WriteCPE(CoderInfo *coderInfoL,
                    CoderInfo *coderInfoR,
                    ChannelInfo *channelInfo,
                    BitStream* bitStream,
                    int objectType,
                    int writeFlag)
{
    int bits = 0;

    if (writeFlag) {
        /* write ID_CPE, single_element_channel() identifier */
        PutBit(bitStream, ID_CPE, LEN_SE_ID);

        /* write the element_identifier_tag */
        PutBit(bitStream, channelInfo->tag, LEN_TAG);

        /* common_window? */
        PutBit(bitStream, channelInfo->common_window, LEN_COM_WIN);
    }

    bits += LEN_SE_ID;
    bits += LEN_TAG;
    bits += LEN_COM_WIN;

    /* if common_window, write ics_info */
    if (channelInfo->common_window) {
        int numWindows, maxSfb;

        bits += WriteICSInfo(coderInfoL, bitStream, objectType, channelInfo->common_window, writeFlag);
        numWindows = coderInfoL->num_window_groups;
        maxSfb = coderInfoL->max_sfb;

        if (writeFlag) {
            PutBit(bitStream, channelInfo->msInfo.is_present, LEN_MASK_PRES);
            if (channelInfo->msInfo.is_present == 1) {
                int g;
                int b;
                for (g=0;g<numWindows;g++) {
                    for (b=0;b<maxSfb;b++) {
                        PutBit(bitStream, channelInfo->msInfo.ms_used[g*maxSfb+b], LEN_MASK);
                    }
                }
            }
        }
        bits += LEN_MASK_PRES;
        if (channelInfo->msInfo.is_present == 1)
            bits += (numWindows*maxSfb*LEN_MASK);
    }

    /* Write individual_channel_stream elements */
    bits += WriteICS(coderInfoL, bitStream, channelInfo->common_window, objectType, writeFlag);
    bits += WriteICS(coderInfoR, bitStream, channelInfo->common_window, objectType, writeFlag);

    return bits;
}

static int WriteSCE(CoderInfo *coderInfo,
                    ChannelInfo *channelInfo,
                    BitStream *bitStream,
                    int objectType,
                    int writeFlag)
{
    int bits = 0;

    if (writeFlag) {
        /* write Single Element Channel (SCE) identifier */
        PutBit(bitStream, ID_SCE, LEN_SE_ID);

        /* write the element identifier tag */
        PutBit(bitStream, channelInfo->tag, LEN_TAG);
    }

    bits += LEN_SE_ID;
    bits += LEN_TAG;

    /* Write an Individual Channel Stream element */
    bits += WriteICS(coderInfo, bitStream, 0, objectType, writeFlag);

    return bits;
}

static int WriteLFE(CoderInfo *coderInfo,
                    ChannelInfo *channelInfo,
                    BitStream *bitStream,
                    int objectType,
                    int writeFlag)
{
    int bits = 0;

    if (writeFlag) {
        /* write ID_LFE, lfe_element_channel() identifier */
        PutBit(bitStream, ID_LFE, LEN_SE_ID);

        /* write the element_identifier_tag */
        PutBit(bitStream, channelInfo->tag, LEN_TAG);
    }

    bits += LEN_SE_ID;
    bits += LEN_TAG;

    /* Write an individual_channel_stream element */
    bits += WriteICS(coderInfo, bitStream, 0, objectType, writeFlag);

    return bits;
}

static int WriteICSInfo(CoderInfo *coderInfo,
                        BitStream *bitStream,
                        int objectType,
                        int common_window,
                        int writeFlag)
{
    int grouping_bits;
    int bits = 0;

    if (writeFlag) {
        /* write out ics_info() information */
        PutBit(bitStream, 0, LEN_ICS_RESERV);  /* reserved Bit*/

        /* Write out window sequence */
        PutBit(bitStream, coderInfo->block_type, LEN_WIN_SEQ);  /* block type */

        /* Write out window shape */
        PutBit(bitStream, coderInfo->window_shape, LEN_WIN_SH);  /* window shape */
    }

    bits += LEN_ICS_RESERV;
    bits += LEN_WIN_SEQ;
    bits += LEN_WIN_SH;

    /* For short windows, write out max_sfb and scale_factor_grouping */
    if (coderInfo->block_type == ONLY_SHORT_WINDOW){
        if (writeFlag) {
            PutBit(bitStream, coderInfo->max_sfb, LEN_MAX_SFBS);
            grouping_bits = FindGroupingBits(coderInfo);
            PutBit(bitStream, grouping_bits, MAX_SHORT_WINDOWS - 1);  /* the grouping bits */
        }
        bits += LEN_MAX_SFBS;
        bits += MAX_SHORT_WINDOWS - 1;
    } else { /* Otherwise, write out max_sfb and predictor data */
        if (writeFlag) {
            PutBit(bitStream, coderInfo->max_sfb, LEN_MAX_SFBL);
        }
        bits += LEN_MAX_SFBL;

        if (objectType == LTP)
        {
            bits++;
            if(writeFlag)
                PutBit(bitStream, coderInfo->ltpInfo.global_pred_flag, 1); /* Prediction Global used */

            bits += WriteLTPPredictorData(coderInfo, bitStream, writeFlag);
            if (common_window)
                bits += WriteLTPPredictorData(coderInfo, bitStream, writeFlag);
        } else {
            bits++;
            if (writeFlag)
                PutBit(bitStream, coderInfo->pred_global_flag, LEN_PRED_PRES);  /* predictor_data_present */

            bits += WritePredictorData(coderInfo, bitStream, writeFlag);
        }
    }

    return bits;
}

static int WriteICS(CoderInfo *coderInfo,
                    BitStream *bitStream,
                    int commonWindow,
                    int objectType,
                    int writeFlag)
{
    /* this function writes out an individual_channel_stream to the bitstream and */
    /* returns the number of bits written to the bitstream */
    int bits = 0;

    /* Write the 8-bit global_gain */
    if (writeFlag)
        PutBit(bitStream, coderInfo->global_gain, LEN_GLOB_GAIN);
    bits += LEN_GLOB_GAIN;

    /* Write ics information */
    if (!commonWindow) {
        bits += WriteICSInfo(coderInfo, bitStream, objectType, commonWindow, writeFlag);
    }

    bits += SortBookNumbers(coderInfo, bitStream, writeFlag);
    bits += WriteScalefactors(coderInfo, bitStream, writeFlag);
    bits += WritePulseData(coderInfo, bitStream, writeFlag);
    bits += WriteTNSData(coderInfo, bitStream, writeFlag);
    bits += WriteGainControlData(coderInfo, bitStream, writeFlag);
    bits += WriteSpectralData(coderInfo, bitStream, writeFlag);

    /* Return number of bits */
    return bits;
}

static int WriteLTPPredictorData(CoderInfo *coderInfo, BitStream *bitStream, int writeFlag)
{
    int i, last_band;
    int bits;
    LtpInfo *ltpInfo = &coderInfo->ltpInfo;

    bits = 0;

    if (ltpInfo->global_pred_flag)
    {

        if(writeFlag)
            PutBit(bitStream, 1, 1); /* LTP used */
        bits++;

        switch(coderInfo->block_type)
        {
        case ONLY_LONG_WINDOW:
        case LONG_SHORT_WINDOW:
        case SHORT_LONG_WINDOW:
            bits += LEN_LTP_LAG;
            bits += LEN_LTP_COEF;
            if(writeFlag)
            {
                PutBit(bitStream, ltpInfo->delay[0], LEN_LTP_LAG);
                PutBit(bitStream, ltpInfo->weight_idx,  LEN_LTP_COEF);
            }

            last_band = ((coderInfo->nr_of_sfb < MAX_LT_PRED_LONG_SFB) ?
                coderInfo->nr_of_sfb : MAX_LT_PRED_LONG_SFB);
//            last_band = coderInfo->nr_of_sfb;

            bits += last_band;
            if(writeFlag)
                for (i = 0; i < last_band; i++)
                    PutBit(bitStream, ltpInfo->sfb_prediction_used[i], LEN_LTP_LONG_USED);
            break;

        default:
            break;
        }
    }

    return (bits);
}

static int WritePredictorData(CoderInfo *coderInfo,
                              BitStream *bitStream,
                              int writeFlag)
{
    int bits = 0;

    /* Write global predictor data present */
    short predictorDataPresent = coderInfo->pred_global_flag;
    int numBands = min(coderInfo->max_pred_sfb, coderInfo->nr_of_sfb);

    if (writeFlag) {
        if (predictorDataPresent) {
            int b;
            if (coderInfo->reset_group_number == -1) {
                PutBit(bitStream, 0, LEN_PRED_RST); /* No prediction reset */
            } else {
                PutBit(bitStream, 1, LEN_PRED_RST);
                PutBit(bitStream, (unsigned long)coderInfo->reset_group_number,
                    LEN_PRED_RSTGRP);
            }

            for (b=0;b<numBands;b++) {
                PutBit(bitStream, coderInfo->pred_sfb_flag[b], LEN_PRED_ENAB);
            }
        }
    }
    bits += (predictorDataPresent) ?
        (LEN_PRED_RST +
        ((coderInfo->reset_group_number)!=-1)*LEN_PRED_RSTGRP +
        numBands*LEN_PRED_ENAB) : 0;

    return bits;
}

static int WritePulseData(CoderInfo *coderInfo,
                          BitStream *bitStream,
                          int writeFlag)
{
    int bits = 0;

    if (writeFlag) {
        PutBit(bitStream, 0, LEN_PULSE_PRES);  /* no pulse_data_present */
    }

    bits += LEN_PULSE_PRES;

    return bits;
}

static int WriteTNSData(CoderInfo *coderInfo,
                        BitStream *bitStream,
                        int writeFlag)
{
    int bits = 0;
    int numWindows;
    int len_tns_nfilt;
    int len_tns_length;
    int len_tns_order;
    int filtNumber;
    int resInBits;
    int bitsToTransmit;
    unsigned long unsignedIndex;
    int w;

    TnsInfo* tnsInfoPtr = &coderInfo->tnsInfo;

    if (writeFlag) {
        PutBit(bitStream,tnsInfoPtr->tnsDataPresent,LEN_TNS_PRES);
    }
    bits += LEN_TNS_PRES;

    /* If TNS is not present, bail */
    if (!tnsInfoPtr->tnsDataPresent) {
        return bits;
    }

    /* Set window-dependent TNS parameters */
    if (coderInfo->block_type == ONLY_SHORT_WINDOW) {
        numWindows = MAX_SHORT_WINDOWS;
        len_tns_nfilt = LEN_TNS_NFILTS;
        len_tns_length = LEN_TNS_LENGTHS;
        len_tns_order = LEN_TNS_ORDERS;
    }
    else {
        numWindows = 1;
        len_tns_nfilt = LEN_TNS_NFILTL;
        len_tns_length = LEN_TNS_LENGTHL;
        len_tns_order = LEN_TNS_ORDERL;
    }

    /* Write TNS data */
    bits += (numWindows * len_tns_nfilt);
    for (w=0;w<numWindows;w++) {
        TnsWindowData* windowDataPtr = &tnsInfoPtr->windowData[w];
        int numFilters = windowDataPtr->numFilters;
        if (writeFlag) {
            PutBit(bitStream,numFilters,len_tns_nfilt); /* n_filt[] = 0 */
        }
        if (numFilters) {
            bits += LEN_TNS_COEFF_RES;
            resInBits = windowDataPtr->coefResolution;
            if (writeFlag) {
                PutBit(bitStream,resInBits-DEF_TNS_RES_OFFSET,LEN_TNS_COEFF_RES);
            }
            bits += numFilters * (len_tns_length+len_tns_order);
            for (filtNumber=0;filtNumber<numFilters;filtNumber++) {
                TnsFilterData* tnsFilterPtr=&windowDataPtr->tnsFilter[filtNumber];
                int order = tnsFilterPtr->order;
                if (writeFlag) {
                    PutBit(bitStream,tnsFilterPtr->length,len_tns_length);
                    PutBit(bitStream,order,len_tns_order);
                }
                if (order) {
                    bits += (LEN_TNS_DIRECTION + LEN_TNS_COMPRESS);
                    if (writeFlag) {
                        PutBit(bitStream,tnsFilterPtr->direction,LEN_TNS_DIRECTION);
                        PutBit(bitStream,tnsFilterPtr->coefCompress,LEN_TNS_COMPRESS);
                    }
                    bitsToTransmit = resInBits - tnsFilterPtr->coefCompress;
                    bits += order * bitsToTransmit;
                    if (writeFlag) {
                        int i;
                        for (i=1;i<=order;i++) {
                            unsignedIndex = (unsigned long) (tnsFilterPtr->index[i])&(~(~0<<bitsToTransmit));
                            PutBit(bitStream,unsignedIndex,bitsToTransmit);
                        }
                    }
                }
            }
        }
    }
    return bits;
}

static int WriteGainControlData(CoderInfo *coderInfo,
                                BitStream *bitStream,
                                int writeFlag)
{
    int bits = 0;

    if (writeFlag) {
        PutBit(bitStream, 0, LEN_GAIN_PRES);
    }

    bits += LEN_GAIN_PRES;

    return bits;
}

static int WriteSpectralData(CoderInfo *coderInfo,
                             BitStream *bitStream,
                             int writeFlag)
{
    int i, bits = 0;

    /* set up local pointers to data and len */
    /* data array contains data to be written */
    /* len array contains lengths of data words */
    int* data = coderInfo->data;
    int* len  = coderInfo->len;

    if (writeFlag) {
        for(i = 0; i < coderInfo->spectral_count; i++) {
            if (len[i] > 0) {  /* only send out non-zero codebook data */
                PutBit(bitStream, data[i], len[i]); /* write data */
                bits += len[i];
            }
        }
    } else {
        for(i = 0; i < coderInfo->spectral_count; i++) {
            bits += len[i];
        }
    }

    return bits;
}

static int WriteAACFillBits(BitStream* bitStream,
                            int numBits,
                            int writeFlag)
{
    int numberOfBitsLeft = numBits;

    /* Need at least (LEN_SE_ID + LEN_F_CNT) bits for a fill_element */
    int minNumberOfBits = LEN_SE_ID + LEN_F_CNT;

    while (numberOfBitsLeft >= minNumberOfBits)
    {
        int numberOfBytes;
        int maxCount;

        if (writeFlag) {
            PutBit(bitStream, ID_FIL, LEN_SE_ID);   /* Write fill_element ID */
        }
        numberOfBitsLeft -= minNumberOfBits;    /* Subtract for ID,count */

        numberOfBytes = (int)(numberOfBitsLeft/LEN_BYTE);
        maxCount = (1<<LEN_F_CNT) - 1;  /* Max count without escaping */

        /* if we have less than maxCount bytes, write them now */
        if (numberOfBytes < maxCount) {
            int i;
            if (writeFlag) {
                PutBit(bitStream, numberOfBytes, LEN_F_CNT);
                for (i = 0; i < numberOfBytes; i++) {
                    PutBit(bitStream, 0, LEN_BYTE);
                }
            }
            /* otherwise, we need to write an escape count */
        }
        else {
            int maxEscapeCount, maxNumberOfBytes, escCount;
            int i;
            if (writeFlag) {
                PutBit(bitStream, maxCount, LEN_F_CNT);
            }
            maxEscapeCount = (1<<LEN_BYTE) - 1;  /* Max escape count */
            maxNumberOfBytes = maxCount + maxEscapeCount;
            numberOfBytes = (numberOfBytes > maxNumberOfBytes ) ? (maxNumberOfBytes) : (numberOfBytes);
            escCount = numberOfBytes - maxCount;
            if (writeFlag) {
                PutBit(bitStream, escCount, LEN_BYTE);
                for (i = 0; i < numberOfBytes-1; i++) {
                    PutBit(bitStream, 0, LEN_BYTE);
                }
            }
        }
        numberOfBitsLeft -= LEN_BYTE*numberOfBytes;
    }

    return numberOfBitsLeft;
}

static int FindGroupingBits(CoderInfo *coderInfo)
{
    /* This function inputs the grouping information and outputs the seven bit
    'grouping_bits' field that the AAC decoder expects.  */

    int grouping_bits = 0;
    int tmp[8];
    int i, j;
    int index = 0;

    for(i = 0; i < coderInfo->num_window_groups; i++){
        for (j = 0; j < coderInfo->window_group_length[i]; j++){
            tmp[index++] = i;
        }
    }

    for(i = 1; i < 8; i++){
        grouping_bits = grouping_bits << 1;
        if(tmp[i] == tmp[i-1]) {
            grouping_bits++;
        }
    }

    return grouping_bits;
}

/* size in bytes! */
BitStream *OpenBitStream(int size, unsigned char *buffer)
{
    BitStream *bitStream;

    bitStream = AllocMemory(sizeof(BitStream));
    bitStream->size = size;
    bitStream->numBit = 0;
    bitStream->currentBit = 0;
    bitStream->data = buffer;
    SetMemory(bitStream->data, 0, size);

    return bitStream;
}

int CloseBitStream(BitStream *bitStream)
{
    int bytes = bit2byte(bitStream->numBit);

    FreeMemory(bitStream);

    return bytes;
}

static long BufferNumBit(BitStream *bitStream)
{
    return bitStream->numBit;
}

static int WriteByte(BitStream *bitStream,
                     unsigned long data,
                     int numBit)
{
    long numUsed,idx;

    idx = (bitStream->currentBit / BYTE_NUMBIT) % bitStream->size;
    numUsed = bitStream->currentBit % BYTE_NUMBIT;
    if (numUsed == 0)
        bitStream->data[idx] = 0;
    bitStream->data[idx] |= (data & ((1<<numBit)-1)) <<
        (BYTE_NUMBIT-numUsed-numBit);
    bitStream->currentBit += numBit;
    bitStream->numBit = bitStream->currentBit;

    return 0;
}

int PutBit(BitStream *bitStream,
           unsigned long data,
           int numBit)
{
    int num,maxNum,curNum;
    unsigned long bits;

    if (numBit == 0)
        return 0;

    /* write bits in packets according to buffer byte boundaries */
    num = 0;
    maxNum = BYTE_NUMBIT - bitStream->currentBit % BYTE_NUMBIT;
    while (num < numBit) {
        curNum = min(numBit-num,maxNum);
        bits = data>>(numBit-num-curNum);
        if (WriteByte(bitStream, bits, curNum)) {
            return 1;
        }
        num += curNum;
        maxNum = BYTE_NUMBIT;
    }

    return 0;
}

static int ByteAlign(BitStream *bitStream, int writeFlag)
{
    int len, i,j;

    len = BufferNumBit(bitStream);

    j = (8 - (len%8))%8;

    if ((len % 8) == 0) j = 0;
    if (writeFlag) {
        for( i=0; i<j; i++ ) {
            PutBit(bitStream, 0, 1);
        }
    }
    return j;
}

