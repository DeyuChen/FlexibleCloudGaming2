#include "ffmpeg.h"
#include "PMeshRenderer.h"
#include "glWindow.h"
#include "codec.h"
#include "communicator.h"
#include <iostream>
#include <fstream>
#include <chrono>

using namespace std;

const int width = 1280;
const int height = 960;

int main(int argc, char *argv[]){
    if (argc < 2){
        cout << "Not enough input arguments" << endl;
        return -1;
    }

    ServerComm comm(9999);

    glWindow window;
    window.create_window("Server", width, height);

    Encoder encoder("libx264", width, height, 8000000);

    ifstream ifs(argv[1], ifstream::in);

    hh::PMesh pm;
    pm.read(ifs);
    int mid = window.add_pmesh(pm);

    unsigned char* simpPixels = new unsigned char[4 * width * height];
    unsigned char* fullPixels = new unsigned char[4 * width * height];

    AVPacket *pkt = av_packet_alloc();
    if (!pkt){
        return 1;
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        cerr << "Could not allocate video frame" << endl;
        return 1;
    }
    frame->format = AV_PIX_FMT_RGBA;
    frame->width  = width;
    frame->height = height;
    if (av_frame_get_buffer(frame, 32) < 0){
        cerr << "Could not allocate the video frame data" << endl;
        return 1;
    }

    bool quit = false;
    while(!quit){
        comm.recv_msg();
        int x, y;
        comm.get_mouse_state(x, y);
        window.mouse_motion(x, y);

        for (int i = 0; i < comm.get_key_press_size(); i++){
            window.key_press(comm.get_key_press(i), x, y);
        }

        // flush all events to prevent overflow
        SDL_PumpEvents();
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

#ifdef SHOW_RENDERING_TIME
        auto begin = chrono::high_resolution_clock::now();
#endif

        window.render(MeshMode::full);
        window.read_pixels(fullPixels);
        window.render(MeshMode::simp);
        window.read_pixels(simpPixels);
        window.render_diff(fullPixels, simpPixels);
        //window.display();

#ifdef SHOW_RENDERING_TIME
        auto end = chrono::high_resolution_clock::now();
        auto dur = end - begin;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
        cout << "rendering time: " << ms << "ms" << endl;
#endif

        // encoding
        window.read_pixels(frame->data[0]);
        if (encoder.encode(frame, pkt)){
            //cout << pkt->size << endl;
        }

        comm.set_diff_frame(pkt);
        comm.send_msg();

        av_packet_unref(pkt);
    }

    return 0;
}
