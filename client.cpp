#include "ffmpeg.h"
#include "PMeshRenderer.h"
#include "glWindow.h"
#include "codec.h"
#include <iostream>
#include <fstream>

using namespace std;

const int width = 1080;
const int height = 720;

int main(int argc, char *argv[]){
    if (argc < 2){
        cout << "Not enough input arguments" << endl;
        return -1;
    }

    glWindow window;
    window.create_window("Client", width, height);

    Decoder decoder("h264", width, height);

    AVPacket *pkt = av_packet_alloc();
    if (!pkt){
        return 1;
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        cerr << "Could not allocate video frame" << endl;
        return 1;
    }
    frame->format = AV_PIX_FMT_RGB24;
    frame->width  = width;
    frame->height = height;
    if (av_frame_get_buffer(frame, 32) < 0){
        cerr << "Could not allocate the video frame data" << endl;
        return 1;
    }

    ifstream ifs(argv[1], ifstream::in);

    hh::PMesh pm;
    pm.read(ifs);
    int mid = window.add_pmesh(pm);

    SDL_Event e;
    SDL_SetRelativeMouseMode(SDL_TRUE);

    unsigned char* simpPixels = new unsigned char[3 * width * height];
    unsigned char* fullPixels = new unsigned char[3 * width * height];

    // TODO: connect to server

    bool quit = false;
    while(!quit){
        int x = 0, y = 0;
        SDL_GetRelativeMouseState(&x, &y);
        window.mouse_motion(x, y);

        while (SDL_PollEvent(&e) != 0){
            if (e.type == SDL_QUIT){
                quit = true;
            } else if (e.type == SDL_KEYDOWN){
                window.key_press(e.key.keysym.sym, x, y);
            }
        }

        window.render(MeshMode::simp);
        window.read_pixels(simpPixels);

        // TODO: recv pkt from server

        // TODO: decode pkt
        
        // TODO: render sum

        window.display();
    }

    return 0;
}
