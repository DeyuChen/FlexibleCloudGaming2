#include "codec.h"
#include <iostream>

using namespace std;

Encoder::Encoder(const char *codecName, int width, int height, int bit_rate)
    : codecName(codecName), width(width), height(height), bit_rate(bit_rate), pts(0)
{
    av_register_all();
    codec = avcodec_find_encoder_by_name(codecName);
    if (!codec){
        cerr << "Codec " << codecName << " not found" << endl;
        return;
    }

    c = avcodec_alloc_context3(codec);
    if (!c){
        cerr << "Could not allocate video codec context" << endl;
        return;
    }

    c->bit_rate = bit_rate;
    c->width = width;
    c->height = height;
    c->time_base = (AVRational){1, 25};
    c->framerate = (AVRational){25, 1};

    c->gop_size = 10;
    c->max_b_frames = 0;
    c->pix_fmt = YUV_FORMAT;

    // setting for live streaming
    switch (codec->id){
        case AV_CODEC_ID_H264:
        case AV_CODEC_ID_H265:
            av_opt_set(c->priv_data, "tune", "zerolatency", 0);
            av_opt_set(c->priv_data, "preset", "ultrafast", 0);
            av_opt_set(c->priv_data, "rc-lookahead", "0", 0);
            av_opt_set(c->priv_data, "sync-lookahead", "0", 0);

            break;
    }

    int ret = avcodec_open2(c, codec, NULL);
    if (ret < 0){
        cerr << "Could not open codec: " << ret << endl;
        return;
    }

    frameYUV = av_frame_alloc();
    if (!frameYUV){
        cerr << "Could not allocate video frame" << endl;
        return;
    }
    frameYUV->format = YUV_FORMAT;
    frameYUV->width  = width;
    frameYUV->height = height;
    if (av_frame_get_buffer(frameYUV, 32) < 0){
        cerr << "Could not allocate the video frame data" << endl;
        return;
    }

    // rgb to yuv transformation
    ctxRGB2YUV = sws_getContext(width, height, AV_PIX_FMT_RGBA, width, height, YUV_FORMAT, 0, 0, 0, 0);
    if (!ctxRGB2YUV){
        cerr << "Failed to get sws context!" << endl;
        return;
    }
}

bool Encoder::encode(AVFrame *frameRGB, AVPacket *pkt){
    sws_scale(ctxRGB2YUV, frameRGB->data, frameRGB->linesize, 0, height, frameYUV->data, frameYUV->linesize);

    // TODO: support ffmpeg 4.x (avcodec_send_frame/avcodec_receive_packet)

    int got_output;
    if (avcodec_encode_video2(c, pkt, frameYUV, &got_output) < 0){
        cerr << "Error encoding frame" << endl;
        return false;
    }
    if (!got_output)
        cout << "encode no output" << endl;

    return got_output;
}

EncoderMT::EncoderMT(const char *codecName, int width, int height, int bit_rate,
                     Pool<proto::CommProto*> &msgPool,
                     Queue<proto::CommProto*> &msgToSend,
                     Pool<Frame3D*> &framePool,
                     Queue<Frame3D*> &frameToEncode)
    : Encoder(codecName, width, height, bit_rate),
      msgPool(msgPool), msgToSend(msgToSend),
      framePool(framePool), frameToEncode(frameToEncode)
{
    pkt = av_packet_alloc();
    if (!pkt){
        return;
    }

    init_thread();
}

EncoderMT::~EncoderMT(){
    if (thread){
        pthread_cancel(thread);
        pthread_join(thread, NULL);
    }
}

void EncoderMT::encoder_service(){
    while (true){
        Frame3D *frame = frameToEncode.get();
        encode(frame->color, pkt);
        framePool.put(frame);

        proto::CommProto *msg = msgPool.get();
        string frameStr((char*)pkt->data, pkt->size);
        av_packet_unref(pkt);
        msg->set_diff_frame(frameStr);
        msgToSend.put(msg);
    }
}

Decoder::Decoder(const char *codecName, int width, int height) : 
    codecName(codecName), width(width), height(height)
{
    av_register_all();
    codec = avcodec_find_decoder_by_name(codecName);
    if (!codec){
        cerr << "Codec " << codecName << " not found" << endl;
        return;
    }

    c = avcodec_alloc_context3(codec);
    if (!c){
        cerr << "Could not allocate video codec context" << endl;
        return;
    }

    int ret = avcodec_open2(c, codec, NULL);
    if (ret < 0){
        cerr << "Could not open codec: " << ret << endl;
        return;
    }

    frameYUV = av_frame_alloc();
    if (!frameYUV){
        cerr << "Could not allocate video frame" << endl;
        return;
    }
    frameYUV->format = YUV_FORMAT;
    frameYUV->width  = width;
    frameYUV->height = height;
    if (av_frame_get_buffer(frameYUV, 32) < 0){
        cerr << "Could not allocate the video frame data" << endl;
        return;
    }

    // yuv to rgb transformation
    ctxYUV2RGB = sws_getContext(width, height, YUV_FORMAT, width, height, AV_PIX_FMT_RGBA, 0, 0, 0, 0);
    if (!ctxYUV2RGB){
        cerr << "Failed to get sws context!" << endl;
        return;
    }
}

bool Decoder::decode(AVFrame *frameRGB, AVPacket *pkt){
    int got_output;
    if (avcodec_decode_video2(c, frameYUV, &got_output, pkt) < 0){
        cerr << "Error decoding frame" << endl;
        return false;
    }

    if (!got_output){
        cout << "decode no output" << endl;
        return false;
    }

    sws_scale(ctxYUV2RGB, frameYUV->data, frameYUV->linesize, 0, height, frameRGB->data, frameRGB->linesize);

    return true;
}

DecoderMT::DecoderMT(const char *codecName, int width, int height,
                     Pool<proto::CommProto*> &msgPool,
                     Queue<proto::CommProto*> &msgReceived,
                     Queue<AVFrame*> &frameDecoded)
    : Decoder(codecName, width, height),
      msgPool(msgPool), msgReceived(msgReceived),
      frameDecoded(frameDecoded)
{
    pkt = av_packet_alloc();
    if (!pkt){
        return;
    }

    init_thread();
}

DecoderMT::~DecoderMT(){
    if (thread){
        pthread_cancel(thread);
        pthread_join(thread, NULL);
    }
}

void DecoderMT::decoder_service(){
    while (true){
        proto::CommProto *msg = msgReceived.get();
        pkt->size = msg->diff_frame().size();
        pkt->data = (uint8_t*)msg->diff_frame().c_str();
        AVFrame *frame = frameDecoded.get();
        decode(frame, pkt);
        av_packet_unref(pkt);
        msg->Clear();
        msgPool.put(msg);
        frameDecoded.put(frame);
    }
}
