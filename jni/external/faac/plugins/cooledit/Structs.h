
#ifndef Structs_h
#define Structs_h

typedef struct mec
{
bool					AutoCfg;
bool					UseQuality;
faacEncConfiguration	EncCfg;
} MY_ENC_CFG;
// -----------------------------------------------------------------------------------------------

typedef struct mdc
{
bool					DefaultCfg;
BYTE					Channels;
DWORD					BitRate;
faacDecConfiguration	DecCfg;
} MY_DEC_CFG;

#endif