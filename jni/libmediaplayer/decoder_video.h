#ifndef FFMPEG_DECODER_VIDEO_H
#define FFMPEG_DECODER_VIDEO_H

#include "decoder.h"

class DecoderVideoCallback
{
public:
    virtual void onDecode(AVFrame* frame, double pts);
};

class DecoderVideo : public IDecoder
{
public:
    DecoderVideo(AVStream* stream, DecoderVideoCallback* callback);
    ~DecoderVideo();

private:
    AVFrame*                                            mFrame;
    double                                              mVideoClock;
    DecoderVideoCallback*                               mCallback;

    bool                                                prepare();
    double                                              synchronize(AVFrame *src_frame, double pts);
    bool                                                decode();
    bool                                                process(AVPacket *packet);

    static int                                          getBuffer(struct AVCodecContext *c, AVFrame *pic);
    static void                                         releaseBuffer(struct AVCodecContext *c, AVFrame *pic);
};

#endif //FFMPEG_DECODER_AUDIO_H
