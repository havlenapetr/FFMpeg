// FAAC encoder for foobar2000 diskwriter
// Copyright (C) 2003 Janne Hyvärinen
//
// Changes:
//  0.4.2 (2003-12-14): Changed the gapless method again
//  0.4.1 (2003-12-13): Added ctts field writing for MP4 mode
//  0.4   (2003-12-11): Added support for average bitrate controlling
//  0.3.5 (2003-10-17): Changed way gapless encoding is handled (iTunes is buggy...)
//  0.3.4 (2003-10-14): Fixed AAC object type selecting
//  0.3.3 (2003-10-02): Removed gapless support for raw AAC files, it was hacky and recent libfaad changes broke it
//  0.3.2 (2003-09-17): Last fix wasn't perfect and very small input chunks wouldn't have been encoded
//  0.3.1 (2003-09-14): Fixed possible memory access problems
//  0.3   (2003-08-17): Even more corrections to MP4 writing, now encoder delay is taken into account and first MP4 sample is given length 0
//                      writes 'TOOL' metadata tag with libfaac version string
//  0.2.9 (2003-08-16): Fixes in MP4 writing
//  0.2.8 (2003-08-16): Added silence padding at the end, new libfaac doesn't do it itself
//  0.2.7 (2003-08-16): MP4 fixes, now MP4 header stores correct length
//  0.2.6 (2003-08-16): MP4 writing uses correct length for last frame
//  0.2.5 (2003-08-15): "libfaac flushing" added for 0.2.2 removed, foo_mp4 was cutting the end away incorrectly
//  0.2.4 (2003-08-09): Added direct M4A writing and one new mode to bandwidth list
//                      uses LFE mode with 6 channels
//  0.2.3 (2003-08-08): Doesn't write tech info to APEv2 tags anymore
//                      stores original file length now also for AAC
//                      no longer limited to 32bit storage for length
//                      changes to config
//  0.2.2 (2003-08-07): Flushes libfaac now properly to get gapless playback
//                      fixed bandwidth selecting issues in config
//  0.2.1 (2003-08-07): Fixed MP4 writing
//  0.2   (2003-08-07): Added MP4 creation and tagging, reorganized config
//                      reports libfaac version in component info string
//  0.1   (2003-08-06): First public version

#include <mp4.h>
#include "../SDK/foobar2000.h"
#include "resource.h"
#include <commctrl.h>
#include <faac.h>
#include <version.h>

#define FOO_FAAC_VERSION     "0.4.2"

#define FF_AAC  0
#define FF_MP4  1
#define FF_M4A  2

#define FF_DEFAULT_OBJECTTYPE   LOW
#define FF_DEFAULT_MIDSIDE      1
#define FF_DEFAULT_TNS          0
#define FF_DEFAULT_QUANTQUAL    100
#define FF_DEFAULT_AVGBRATE     64
#define FF_DEFAULT_CUTOFF       -1
#define FF_DEFAULT_MP4CONTAINER FF_MP4
#define FF_DEFAULT_USE_QQ       1
#define FF_DEFAULT_USE_AB       0

static cfg_int cfg_objecttype ( "objecttype", FF_DEFAULT_OBJECTTYPE );
static cfg_int cfg_midside ( "midside", FF_DEFAULT_MIDSIDE );
static cfg_int cfg_tns ( "tns", FF_DEFAULT_TNS );
static cfg_int cfg_quantqual ( "quantqual", FF_DEFAULT_QUANTQUAL );
static cfg_int cfg_avgbrate ( "avgbrate", FF_DEFAULT_AVGBRATE );
static cfg_int cfg_cutoff ( "cutoff", FF_DEFAULT_CUTOFF );
static cfg_int cfg_mp4container ( "mp4container", FF_DEFAULT_MP4CONTAINER );
static cfg_int cfg_use_qq ( "use_qq", FF_DEFAULT_USE_QQ );
static cfg_int cfg_use_ab ( "use_ab", FF_DEFAULT_USE_AB );

DECLARE_COMPONENT_VERSION ( "FAAC encoder", FOO_FAAC_VERSION, "Uses libfaac version " FAAC_VERSION );

class diskwriter_faac : public diskwriter {
private:
    // mp4
    MP4FileHandle MP4hFile;
    MP4TrackId MP4track;

    // faac
    faacEncHandle hEncoder;
    faacEncConfigurationPtr myFormat;
    unsigned int objectType;
    unsigned int useMidSide;
    unsigned int useTns;
    int cutOff;
    int bitRate;
    unsigned long quantqual;
    int use_qq, use_ab;

    int create_mp4;

    reader *m_reader;
    mem_block_t<unsigned char> bitbuf;
    mem_block_t<float> floatbuf;
    unsigned long samplesInput, maxBytesOutput;
    int *chanmap;
    unsigned int bufferedSamples;
    unsigned int frameSize;

    string8 path;
    file_info_i_full info;
    unsigned int srate, nch, bps;
    __int64 total_samples, encoded_samples, delay_samples;
    bool encode_error;

public:
    diskwriter_faac()
    {
        objectType = cfg_objecttype;
        useMidSide = cfg_midside;
        useTns = cfg_tns;
        cutOff = cfg_cutoff;
        bitRate = cfg_avgbrate * 1000;
        quantqual = cfg_quantqual;
        use_qq = cfg_use_qq;
        use_ab = cfg_use_ab;
        hEncoder = 0;
        myFormat = 0;

        MP4hFile = 0;
        MP4track = 0;

        create_mp4 = cfg_mp4container;

        m_reader = 0;
    }

    ~diskwriter_faac()
    {
        if ( m_reader ) m_reader->reader_release();
    }

    virtual const char *get_name() { return "AAC"; }

    virtual const char *get_extension()
    {
        switch ( create_mp4 ) {
        case FF_MP4:
        default:
            return "mp4";
        case FF_M4A:
            return "m4a";
        case FF_AAC:
            return "aac";
        }
    }
        
    virtual int open ( const char *filename, metadb_handle *src_file )
    {
        if ( m_reader ) return 0;

        encode_error = false;
        path = filename;
        if ( src_file ) src_file->handle_query ( &info ); else info.reset();

        console::info ( "AAC encoding with FAAC version " FAAC_VERSION );
        console::info ( string_printf ("Source file: %s", (const char *)info.get_file_path()) );
        console::info ( string_printf ("Destination file: %s", (const char *)path) );

        if ( path.is_empty() ) {
            console::error ( "No destination name" );
            return 0;
        }

        m_reader = file::g_open ( path, reader::MODE_WRITE_NEW );
        if ( !m_reader ) {
            console::error ( "Can't write to destination" );
            return 0;
        }

        return 1;
    }
                
    virtual int process_samples ( const audio_chunk *src )
    {
        if ( encode_error ) return 0;

        if ( !hEncoder ) {
            encode_error = true;
            nch = src->get_channels();
            srate = src->get_srate();

            // get faac version
            hEncoder = faacEncOpen ( 44100, 2, &samplesInput, &maxBytesOutput );
            myFormat = faacEncGetCurrentConfiguration ( hEncoder );

            if ( myFormat->version == FAAC_CFG_VERSION ) {
                //console::info ( string_printf ("libfaac version %s", (const char *)myFormat->name) );
                faacEncClose ( hEncoder );
            } else {
                console::error ( "Wrong libfaac version" );
                faacEncClose ( hEncoder );
                hEncoder = 0;
                return 0;
            }

            // open the encoder library
            hEncoder = faacEncOpen ( srate, nch, &samplesInput, &maxBytesOutput );

            bufferedSamples = 0;
            frameSize = samplesInput / nch;
            total_samples = 0;
            encoded_samples = 0;
            delay_samples = frameSize;
            bitbuf.check_size ( maxBytesOutput );
            floatbuf.check_size ( samplesInput );
            chanmap = mkChanMap ( nch, 3/*chanC*/, 4/*chanLF*/ );

            if ( cutOff <= 0 ) {
                if ( cutOff < 0 ) {
                    cutOff = 0;
                } else {
                    cutOff = srate / 2;
                }
            }
            if ( (unsigned)cutOff > (srate / 2) ) cutOff = srate / 2;

            // put the options in the configuration struct
            myFormat = faacEncGetCurrentConfiguration ( hEncoder );
            myFormat->aacObjectType = objectType;
            myFormat->mpegVersion = (create_mp4 || objectType == LTP) ? MPEG4 : MPEG2;
            myFormat->useLfe = (nch == 6) ? 1 : 0;
            myFormat->useTns = useTns;
            myFormat->allowMidside = useMidSide;
            if ( use_ab ) myFormat->bitRate = bitRate;
            myFormat->bandWidth = cutOff;
            if ( use_qq ) myFormat->quantqual = quantqual;
            myFormat->outputFormat = create_mp4 ? 0 : 1;
            myFormat->inputFormat = FAAC_INPUT_FLOAT;

            if ( !faacEncSetConfiguration (hEncoder, myFormat) ) {
                console::error ( "Unsupported output format" );
                return 0;
            }

            // initialize MP4 creation
            if ( create_mp4 ) {
                MP4hFile = MP4CreateCb ( 0, 0, 0, open_cb, close_cb, read_cb, write_cb, setpos_cb, getpos_cb, filesize_cb, (void *)m_reader );
                if ( MP4hFile == MP4_INVALID_FILE_HANDLE ) {
                    console::error ( "MP4Create() failed" );
                    return 0;
                }

                MP4SetTimeScale ( MP4hFile, 90000 );

                MP4track = MP4AddAudioTrack ( MP4hFile, srate, MP4_INVALID_DURATION, MP4_MPEG4_AUDIO_TYPE );

                MP4SetAudioProfileLevel ( MP4hFile, 0x0F );

                unsigned char *ASC = 0;
                unsigned long ASCLength = 0;
                faacEncGetDecoderSpecificInfo(hEncoder, &ASC, &ASCLength);

                MP4SetTrackESConfiguration ( MP4hFile, MP4track, (u_int8_t *)ASC, ASCLength );
            }

            cutOff = myFormat->bandWidth;
            quantqual = myFormat->quantqual;
            bitRate = myFormat->bitRate;

            if ( bitRate > 0 ) {
                console::info ( string_printf ("Using quantizer quality %i and average bitrate of %i kbps per channel", quantqual, bitRate/1000) );
            } else {
                console::info ( string_printf ("Using quantizer quality %i and no average bitrate control", quantqual, bitRate) );
            }

            encode_error = false;
        }

        if ( srate != src->get_srate() || nch != src->get_channels() ) return 0;

        {
            unsigned int samples = src->get_sample_count() * nch;
            const audio_sample *s = src->get_data();

            do {
                unsigned int num = (samples+bufferedSamples < samplesInput) ? samples+bufferedSamples : samplesInput;
                if ( num == 0 ) break;

                float *d = (float *)floatbuf.get_ptr() + bufferedSamples;

                for ( unsigned int i = bufferedSamples; i < num; i++ ) {
                    *d++ = (float)((*s++) * 32768.);

                    bufferedSamples++;
                    samples--;
                }

                if ( bufferedSamples == samplesInput ) {
                    if ( nch >= 3 && chanmap ) {
                        chan_remap ( (int *)floatbuf.get_ptr(), nch, frameSize, chanmap );
                    }

                    // call the actual encoding routine
                    int bytesWritten = faacEncEncode ( hEncoder, (int32_t *)floatbuf.get_ptr(), samplesInput, bitbuf.get_ptr(), maxBytesOutput );

                    bufferedSamples = 0;

                    if ( bytesWritten < 0 ) {
                        console::error ( "faacEncEncode() failed" );
                        return 0;
                    }

                    if ( bytesWritten > 0 ) {
                        MP4Duration dur = frameSize;
                        MP4Duration ofs = 0;

                        if ( delay_samples > 0 ) {
                            dur = 0;
                            ofs = delay_samples;
                            delay_samples -= frameSize;
                        }

                        if ( create_mp4 ) {
                            MP4WriteSample ( MP4hFile, MP4track, (const unsigned __int8 *)bitbuf.get_ptr(), bytesWritten, frameSize, ofs );
                        } else {
                            m_reader->write ( bitbuf.get_ptr(), bytesWritten );
                        }

                        encoded_samples += dur;
                    }
                }
            } while ( bufferedSamples == 0 );
        }

        total_samples += src->get_sample_count();

        return 1;
    }

    virtual void flush()
    {
        if ( hEncoder ) {
            if ( nch >= 3 && chanmap ) {
                chan_remap ( (int *)floatbuf.get_ptr(), nch, bufferedSamples/nch, chanmap );
            }

            __int64 samples_left = total_samples - encoded_samples;

            while ( samples_left > 0 ) {
                if ( !bufferedSamples ) {
                    bufferedSamples = samplesInput;
                    memset ( floatbuf.get_ptr(), 0, samplesInput * sizeof(float) );
                }

                int bytesWritten = faacEncEncode ( hEncoder, (int32_t *)floatbuf.get_ptr(), bufferedSamples, bitbuf.get_ptr(), maxBytesOutput );
                bufferedSamples = 0;

                if ( bytesWritten < 0 ) {
                    console::error ( "faacEncEncode() failed" );
                    break;
                }
                else if ( bytesWritten > 0 ) {
                    MP4Duration dur = samples_left > frameSize ? frameSize : samples_left;

                    if ( create_mp4 ) {
                        MP4WriteSample ( MP4hFile, MP4track, (const unsigned __int8 *)bitbuf.get_ptr(), bytesWritten, dur );
                    } else {
                        m_reader->write ( bitbuf.get_ptr(), bytesWritten );
                    }

                    samples_left -= frameSize;
                }
            }

            faacEncClose ( hEncoder );
            hEncoder = 0;
        }

        if ( m_reader ) {
            bool success = !encode_error && (m_reader->get_length() > 0);

            if ( success ) {
                write_tag();
                console::info ( "Encoding finished successfully" );
            }

            if ( create_mp4 ) {
                MP4Close ( MP4hFile );
                MP4hFile = 0;
            }

            m_reader->reader_release();
            m_reader = 0;

            if ( !success ) {
                console::info ( "Encoding failed" );
                file::g_remove ( path );
            }
        }
    }

    virtual const char *get_config_page_name() { return "FAAC encoder"; }

private:
    int write_tag()
    {
        info.info_remove_all();

        if ( !create_mp4 ) {
            return tag_writer::g_run ( m_reader, &info, "ape" );
        } else {
            MP4SetMetadataTool ( MP4hFile, "libfaac version " FAAC_VERSION );

            for ( int i = 0; i < info.meta_get_count(); i++ ) {
                char *pName = (char *)info.meta_enum_name ( i );
                const char *val = info.meta_enum_value ( i );
                if ( !val || (val && !(*val)) ) continue;

                if ( !stricmp (pName, "TITLE") ) {
                    MP4SetMetadataName ( MP4hFile, val );
                }
                else if ( !stricmp (pName, "ARTIST") ) {
                    MP4SetMetadataArtist ( MP4hFile, val );
                }
                else if ( !stricmp (pName, "WRITER") ) {
                    MP4SetMetadataWriter ( MP4hFile, val );
                }
                else if ( !stricmp (pName, "ALBUM") ) {
                    MP4SetMetadataAlbum ( MP4hFile, val );
                }
                else if ( !stricmp (pName, "YEAR") || !stricmp (pName, "DATE") ) {
                    MP4SetMetadataYear ( MP4hFile, val );
                }
                else if ( !stricmp (pName, "COMMENT") ) {
                    MP4SetMetadataComment ( MP4hFile, val );
                }
                else if ( !stricmp (pName, "GENRE") ) {
                    MP4SetMetadataGenre ( MP4hFile, val );
                }
                else if ( !stricmp (pName, "TRACKNUMBER") ) {
                    unsigned __int16 trkn = atoi ( val ), tot = 0;
                    MP4SetMetadataTrack ( MP4hFile, trkn, tot );
                }
                else if ( !stricmp (pName, "DISKNUMBER") || !stricmp (pName, "DISC") ) {
                    unsigned __int16 disk = atoi ( val ), tot = 0;
                    MP4SetMetadataDisk ( MP4hFile, disk, tot );
                }
                else if ( !stricmp (pName, "COMPILATION") ) {
                    unsigned __int8 cpil = atoi ( val );
                    MP4SetMetadataCompilation ( MP4hFile, cpil );
                }
                else if ( !stricmp (pName, "TEMPO") ) {
                    unsigned __int16 tempo = atoi ( val );
                    MP4SetMetadataTempo ( MP4hFile, tempo );
                } else {
                    MP4SetMetadataFreeForm ( MP4hFile, pName, (unsigned __int8*)val, strlen(val) );
                }
            }

            return 1;
        }
    }

    int *mkChanMap ( int channels, int center, int lf )
    {
        if ( !center && !lf ) return 0;
        if ( channels < 3 ) return 0;

        if ( lf > 0 ) {
            lf--;
        } else {
            lf = channels - 1; // default AAC position
        }

        if ( center > 0 ) {
            center--;
        } else {
            center = 0; // default AAC position
        }

        int *map = (int *)calloc ( channels, sizeof(map[0]) );

        int outpos = 0;
        if ( (center >= 0) && (center < channels) ) map[outpos++] = center;

        int inpos = 0;
        for ( ; outpos < (channels - 1); inpos++ ) {
            if ( inpos == center ) continue;
            if ( inpos == lf ) continue;

            map[outpos++] = inpos;
        }

        if ( outpos < channels ) {
            if ( (lf >= 0) && (lf < channels) ) {
                map[outpos] = lf;
            } else {
                map[outpos] = inpos;
            }
        }

        return map;
    }

    void chan_remap ( int *buf, unsigned int channels, unsigned int blocks, int *map )
    {
        int *tmp = (int *)alloca ( channels * sizeof(int) );

        for ( unsigned int i = 0; i < blocks; i++ ) {
            memcpy ( tmp, buf + i * channels, sizeof(int) * channels );

            for ( unsigned int chn = 0; chn < channels; chn++ ) {
                buf[i * channels + chn] = tmp[map[chn]];
            }
        }
    }

    // MP4 I/O callbacks
    static unsigned __int32 open_cb ( const char *pName, const char *mode, void *userData ) { return 1; }

    static void close_cb ( void *userData ) { return; }

    static unsigned __int32 read_cb ( void *pBuffer, unsigned int nBytesToRead, void *userData )
    {
        reader *r = (reader *)userData;
        return r->read ( pBuffer, nBytesToRead );
    }

    static unsigned __int32 write_cb ( void *pBuffer, unsigned int nBytesToWrite, void *userData )
    {
        reader *r = (reader *)userData;
        return r->write ( pBuffer, nBytesToWrite );
    }

    static __int64 getpos_cb ( void *userData )
    {
        reader *r = (reader *)userData;
        return r->get_position();
    }

    static __int32 setpos_cb ( unsigned __int32 pos, void *userData )
    {
        reader *r = (reader *)userData;
        return !r->seek ( pos );
    }

    static __int64 filesize_cb ( void *userData )
    {
        reader *r = (reader *)userData;
        return r->get_length();
    }
};

// -------------------------------------

typedef struct {
    int     mp4;
    char    name[4];
} format_list_t;

static format_list_t format_list[] = {
    { FF_MP4, "MP4" },
    { FF_M4A, "M4A" },
    { FF_AAC, "AAC" },
};

typedef struct {
    int     profile;
    char    name[6];
} profile_list_t;

static profile_list_t profile_list[] = {
    { LOW,  "LC"   },
    { MAIN, "Main" },
    { LTP,  "LTP"  },
};

typedef struct {
    int     cutoff;
    char    name[10];
} cutoff_list_t;

static cutoff_list_t cutoff_list[] = {
    {     -1, "Automatic" },
    {      0, "Full"      },
    {  20000, "20000"     },
    {  19000, "19000"     },
    {  18000, "18000"     },
    {  17000, "17000"     },
    {  16000, "16000"     },
    {  15000, "15000"     },
    {  14000, "14000"     },
    {  13000, "13000"     },
    {  12000, "12000"     },
    {  11000, "11000"     },
    {  10000, "10000"     },
};

#define QQ_MIN      10
#define QQ_MAX      500
#define AB_MIN      8
#define AB_MAX      384

class config_faac : public config {
    static void update ( HWND wnd )
    {
        int i;
        HWND wnd_format = GetDlgItem ( wnd, IDC_FORMAT );
        HWND wnd_profile = GetDlgItem ( wnd, IDC_PROFILE );
        HWND wnd_cutoff = GetDlgItem ( wnd, IDC_CUTOFF );

        for ( i = 0; i < sizeof(format_list)/sizeof(*format_list); i++ ) {
            if ( (cfg_mp4container == format_list[i].mp4) ) {
                uSendMessage ( wnd_format, CB_SETCURSEL, i, 0 );
                break;
            }
        }

        for ( i = 0; i < sizeof(profile_list)/sizeof(*profile_list); i++ ) {
            if ( cfg_objecttype == profile_list[i].profile ) {
                uSendMessage ( wnd_profile, CB_SETCURSEL, i, 0 );
                break;
            }
        }

        bool cutoff_found = false;

        for ( i = 0; i < sizeof(cutoff_list)/sizeof(*cutoff_list); i++ ) {
            if ( cfg_cutoff == cutoff_list[i].cutoff ) {
                uSendMessage ( wnd_cutoff, CB_SETCURSEL, i, 0 );
                cutoff_found = true;
                break;
            }
        }

        if ( !cutoff_found ) uSetDlgItemText ( wnd, IDC_CUTOFF, string_printf ("%i", (int)cfg_cutoff) );

        uSendDlgItemMessage ( wnd, IDC_QUANTQUAL_SLIDER, TBM_SETPOS, 1, cfg_quantqual );
        uSetDlgItemText ( wnd, IDC_QUANTQUAL_EDIT, string_printf ("%i", (int)cfg_quantqual) );

        uSendDlgItemMessage ( wnd, IDC_AVGBRATE_SLIDER, TBM_SETPOS, 1, cfg_avgbrate );
        uSetDlgItemText ( wnd, IDC_AVGBRATE_EDIT, string_printf ("%i", (int)cfg_avgbrate) );

        CheckDlgButton ( wnd, IDC_USE_QQ, cfg_use_qq );
        CheckDlgButton ( wnd, IDC_USE_AB, cfg_use_ab );

        CheckDlgButton ( wnd, IDC_MIDSIDE, cfg_midside );
        CheckDlgButton ( wnd, IDC_TNS, cfg_tns );
    }

    static BOOL CALLBACK ConfigProc ( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
    {
        HWND wnd_format = GetDlgItem ( wnd, IDC_FORMAT );
        HWND wnd_profile = GetDlgItem ( wnd, IDC_PROFILE );
        HWND wnd_cutoff = GetDlgItem ( wnd, IDC_CUTOFF );

        switch ( msg ) {
        case WM_INITDIALOG:
            {
                int i;

                for ( i = 0; i < sizeof(format_list)/sizeof(*format_list); i++ ) {
                    uSendMessageText ( wnd_format, CB_ADDSTRING, 0, format_list[i].name );
                }

                for ( i = 0; i < sizeof(profile_list)/sizeof(*profile_list); i++ ) {
                    uSendMessageText ( wnd_profile, CB_ADDSTRING, 0, profile_list[i].name );
                }

                for ( i = 0; i < sizeof(cutoff_list)/sizeof(*cutoff_list); i++ ) {
                    uSendMessageText ( wnd_cutoff, CB_ADDSTRING, 0, cutoff_list[i].name );
                }

                uSendDlgItemMessage ( wnd, IDC_QUANTQUAL_SLIDER, TBM_SETRANGE, 0, MAKELONG(QQ_MIN, QQ_MAX) );
                uSendDlgItemMessage ( wnd, IDC_AVGBRATE_SLIDER, TBM_SETRANGE, 0, MAKELONG(AB_MIN, AB_MAX) );

                update ( wnd );
            }
            return TRUE;

        case WM_COMMAND:
            switch ( wp ) {
            case IDC_FORMAT | (CBN_SELCHANGE<<16):
                {
                    int t = (int)uSendMessage ( wnd_format, CB_GETCURSEL, 0, 0 );
                    if ( t >= 0 ) cfg_mp4container = format_list[t].mp4;
                }
                break;

            case IDC_PROFILE | (CBN_SELCHANGE<<16):
                {
                    int t = (int)uSendMessage ( wnd_profile, CB_GETCURSEL, 0, 0 );
                    if ( t >= 0 ) cfg_objecttype = profile_list[t].profile;
                }
                break;

            case IDC_QUANTQUAL_EDIT | (EN_KILLFOCUS<<16):
                {
                    cfg_quantqual = GetDlgItemInt ( wnd, IDC_QUANTQUAL_EDIT, 0, 0 );
                    if ( cfg_quantqual < QQ_MIN || cfg_quantqual > QQ_MAX ) {
                        if ( cfg_quantqual < QQ_MIN ) {
                            cfg_quantqual = QQ_MIN;
                        } else {
                            cfg_quantqual = QQ_MAX;
                        }
                        uSetDlgItemText ( wnd, IDC_QUANTQUAL_EDIT, string_printf ("%i", (int)cfg_quantqual) );
                    }
                    uSendDlgItemMessage ( wnd, IDC_QUANTQUAL_SLIDER, TBM_SETPOS, 1, cfg_quantqual );
                }
                break;

            case IDC_AVGBRATE_EDIT | (EN_KILLFOCUS<<16):
                {
                    cfg_avgbrate = GetDlgItemInt ( wnd, IDC_AVGBRATE_EDIT, 0, 0 );
                    if ( cfg_avgbrate < AB_MIN || cfg_quantqual > AB_MAX ) {
                        if ( cfg_avgbrate< AB_MIN ) {
                            cfg_avgbrate = AB_MIN;
                        } else {
                            cfg_avgbrate = AB_MAX;
                        }
                        uSetDlgItemText ( wnd, IDC_AVGBRATE_EDIT, string_printf ("%i", (int)cfg_avgbrate) );
                    }
                    uSendDlgItemMessage ( wnd, IDC_AVGBRATE_SLIDER, TBM_SETPOS, 1, cfg_avgbrate );
                }
                break;

            case IDC_CUTOFF | (CBN_SELCHANGE<<16):
                {
                    int t = (int)uSendMessage ( wnd_cutoff, CB_GETCURSEL, 0, 0 );
                    if ( t >= 0 ) cfg_cutoff = cutoff_list[t].cutoff;
                }
                break;

            case IDC_CUTOFF | (CBN_KILLFOCUS<<16):
                {
                    int t = (int)uSendMessage ( wnd_cutoff, CB_GETCURSEL, 0, 0 );
                    if ( t < 0 ) {
                        cfg_cutoff = GetDlgItemInt ( wnd, IDC_CUTOFF, 0, 0 );
                        if ( cfg_cutoff > 0 && cfg_cutoff < 100 ) {
                            cfg_cutoff = 100;
                            uSetDlgItemText ( wnd, IDC_CUTOFF, string_printf ("%i", (int)cfg_cutoff) );
                        }
                        else if ( cfg_cutoff <= 0 ) {
                            uSendMessage ( wnd_cutoff, CB_SETCURSEL, 0, 0 );
                            cfg_cutoff = cutoff_list[0].cutoff;
                        }
                    }
                }
                break;

            case IDC_MIDSIDE:
                {
                    cfg_midside = !cfg_midside;
                }
                break;

            case IDC_TNS:
                {
                    cfg_tns = !cfg_tns;
                }
                break;

            case IDC_USE_QQ:
                {
                    cfg_use_qq = !cfg_use_qq;
                }
                break;

            case IDC_USE_AB:
                {
                    cfg_use_ab = !cfg_use_ab;
                }
                break;

            case IDC_DEFAULTS:
                {
                    cfg_objecttype = FF_DEFAULT_OBJECTTYPE;
                    cfg_midside = FF_DEFAULT_MIDSIDE;
                    cfg_tns = FF_DEFAULT_TNS;
                    cfg_avgbrate = FF_DEFAULT_AVGBRATE;
                    cfg_quantqual = FF_DEFAULT_QUANTQUAL;
                    cfg_cutoff = FF_DEFAULT_CUTOFF;
                    cfg_use_qq = FF_DEFAULT_USE_QQ;
                    cfg_use_ab = FF_DEFAULT_USE_AB;
                    update ( wnd );
                }
                break;
            }
            break;

        case WM_HSCROLL:
            switch ( uGetWindowLong((HWND)lp, GWL_ID) ) {
            case IDC_QUANTQUAL_SLIDER:
                cfg_quantqual = uSendMessage ( (HWND)lp, TBM_GETPOS, 0, 0 );
                uSetDlgItemText ( wnd, IDC_QUANTQUAL_EDIT, string_printf ("%i", (int)cfg_quantqual) );
                break;

            case IDC_AVGBRATE_SLIDER:
                cfg_avgbrate = uSendMessage ( (HWND)lp, TBM_GETPOS, 0, 0 );
                uSetDlgItemText ( wnd, IDC_AVGBRATE_EDIT, string_printf ("%i", (int)cfg_avgbrate) );
                break;
            }
            break;
        }

        return FALSE;
    }

    virtual HWND create ( HWND parent ) {
        return uCreateDialog ( IDD_CONFIG, parent, ConfigProc );
    }

    virtual const char *get_name() { return "FAAC encoder"; }

    virtual const char *get_parent_name() { return "Diskwriter"; }
};

static service_factory_t<diskwriter, diskwriter_faac> foo_faac;
static service_factory_single_t<config, config_faac> foo_faac_cfg;
