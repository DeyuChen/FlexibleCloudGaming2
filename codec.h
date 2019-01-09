#ifndef CODEC_H
#define CODEC_H

#include "ffmpeg.h"
#include "pool.h"
#include "CommProto.pb.h"
#include <string>
#include <pthread.h>

#define YUV_FORMAT AV_PIX_FMT_YUV420P

class Encoder {
public:
    Encoder(const char *codecName, int width, int height, int bit_rate);
    bool encode(AVFrame *frameRGB, AVPacket *pkt);

protected:
    std::string codecName;
    int width, height;
    int bit_rate;
    int pts;

    AVCodec *codec;
    AVCodecContext *c;
    AVFrame *frameYUV;
    SwsContext *ctxRGB2YUV;
};

class EncoderMT : public Encoder {
public:
    EncoderMT(const char *codecName, int width, int height, int bit_rate,
              Pool<proto::CommProto*> &msgPool,
              Queue<proto::CommProto*> &msgToSend,
              Pool<AVFrame*> &framePool,
              Queue<AVFrame*> &frameToEncode);

    ~EncoderMT();

private:
    bool encode(AVFrame *frameRGB);

    void init_thread(){ pthread_create(&thread, NULL, encoder_service_entry, this); }
    static void* encoder_service_entry(void* This){ ((EncoderMT*)This)->encoder_service(); }
    void encoder_service();

    AVPacket *pkt;

    Pool<proto::CommProto*> &msgPool;
    Queue<proto::CommProto*> &msgToSend;
    Pool<AVFrame*> &framePool;
    Queue<AVFrame*> &frameToEncode;

    pthread_t thread;
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
