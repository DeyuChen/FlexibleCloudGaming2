#ifndef CODEC_H
#define CODEC_H

#include "ffmpeg.h"
#include <string>

#define YUV_FORMAT AV_PIX_FMT_YUV420P

class Encoder {
public:
    Encoder(const char *codecName, int width, int height, int bit_rate);
    bool encode(AVFrame *frameRGB, AVPacket *pkt);

private:
    std::string codecName;
    int width, height;
    int bit_rate;
    int pts;

    AVCodec *codec;
    AVCodecContext *c;
    AVFrame *frameYUV;
    SwsContext *ctxRGB2YUV;
};

class Decoder {
public:
    Decoder(const char *codecName, int width, int height);
    bool decode(AVFrame *frameRGB, AVPacket *pkt);

private:
    std::string codecName;
    int width, height;

    AVCodec *codec;
    AVCodecContext *c;
    AVFrame *frameYUV;
    SwsContext *ctxYUV2RGB;
};

#endif
