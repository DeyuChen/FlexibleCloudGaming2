#include "ffmpeg.h"
#include "PMeshRenderer.h"
#include "glWindow.h"
#include "communicator.h"
#include "codec.h"
#include <iostream>
#include <fstream>

using namespace std;

const int width = 1280;
const int height = 960;

int main(int argc, char *argv[]){
    if (argc < 2){
        cout << "Not enough input arguments" << endl;
        return -1;
    }

    ClientComm comm("127.0.0.1", 9999);

    glWindow window;
    window.create_window("Client", width, height);

    Decoder decoder("h264", width, height);

    ifstream ifs(argv[1], ifstream::in);

    // TODO: get progressive meshes from the server
    hh::PMesh pm;
    pm.read(ifs);
    int mid = window.add_pmesh(pm);

    SDL_Event e;
    SDL_SetRelativeMouseMode(SDL_TRUE);

    unsigned char* simpPixels = new unsigned char[4 * width * height];

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
        CommonProtocol message;

        int x = 0, y = 0;
        SDL_GetRelativeMouseState(&x, &y);
        window.mouse_motion(x, y);
        comm.set_mouse_state(x, y);

        while (SDL_PollEvent(&e) != 0){
            if (e.type == SDL_QUIT){
                quit = true;
            } else if (e.type == SDL_KEYDOWN && e.key.repeat == 0){
                window.key_event(true, e.key.keysym.sym, x, y);
                comm.add_key_event(true, e.key.keysym.sym);
            } else if (e.type == SDL_KEYUP){
                window.key_event(false, e.key.keysym.sym, x, y);
                comm.add_key_event(false, e.key.keysym.sym);
            }
        }
        window.update_state();

        comm.send_msg();

        window.render(MeshMode::simp);
        window.read_pixels(simpPixels);

        comm.recv_msg();
        comm.get_diff_frame(pkt);

        decoder.decode(frame, pkt);
        av_packet_unref(pkt);

        window.render_sum(simpPixels, frame->data[0]);

        window.display();
    }

    return 0;
}
