#include "ffmpeg.h"
#include "PMeshController.h"
#include "PMeshRenderer.h"
#include "glWindow.h"
#include "communicator.h"
#include "codec.h"
#include <iostream>
#include <fstream>
#include <set>

using namespace std;

const int width = 1280;
const int height = 960;

enum PresentMode {
    simplified,
    patched
};

PresentMode pmode = patched;

void switch_present_mode(){
    pmode = (pmode == simplified)? patched : simplified;
}

PresentMode get_present_mode(){
    return pmode;
}

int main(int argc, char *argv[]){
    ClientComm comm("127.0.0.1", 9999);
    proto::PMeshProto *pmesh_proto;

    set<int> keyToSend = {SDLK_w, SDLK_d, SDLK_s, SDLK_a, SDLK_e, SDLK_q};

    glWindow window;
    window.create_window("Client", width, height);

    Decoder decoder("h264", width, height);

    vector<int> pmIDs;
    vector<int> pmrIDs;

    // receive base mesh from the server
    // TODO: currently assume there will be only one mesh
    // TODO: it might be more efficient to handle the whole communication protocol in main
    comm.recv_msg();
    pmesh_proto = comm.get_pmesh_proto();

    PMeshController pmController;
    pmIDs.push_back(pmController.create_pmesh());
    pmController.set_pmesh_info(pmIDs[0], pmesh_proto->pmesh_info());
    pmController.set_base_mesh(pmIDs[0], pmesh_proto->base_mesh());
    pmrIDs.push_back(window.add_pmesh(pmController.get_pmesh(pmIDs[0])));

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
        int x = 0, y = 0;
        SDL_GetRelativeMouseState(&x, &y);
        window.mouse_motion(x, y);
        comm.set_mouse_state(x, y);

        while (SDL_PollEvent(&e) != 0){
            if (e.type == SDL_QUIT){
                quit = true;
            } else if (e.type == SDL_KEYDOWN && e.key.repeat == 0){
                if (e.key.keysym.sym == SDLK_m){
                    switch_present_mode();
                }
                else {
                    window.key_event(true, e.key.keysym.sym, x, y);
                    if (keyToSend.count(e.key.keysym.sym))
                        comm.add_key_event(true, e.key.keysym.sym);
                }
            } else if (e.type == SDL_KEYUP){
                window.key_event(false, e.key.keysym.sym, x, y);
                if (keyToSend.count(e.key.keysym.sym))
                    comm.add_key_event(false, e.key.keysym.sym);
            }
        }
        window.update_state();

        pmesh_proto = comm.add_pmesh_proto();
        pmesh_proto->set_id(pmrIDs[0]);
        pmesh_proto->set_nvertices(window.get_nvertices(pmrIDs[0]));
        comm.send_msg();

        window.render(MeshMode::simp);
        if (get_present_mode() == simplified)
            window.display();
        window.read_pixels(simpPixels);

        comm.recv_msg();
        comm.get_diff_frame(pkt);

        decoder.decode(frame, pkt);
        av_packet_unref(pkt);

        window.render_sum(simpPixels, frame->data[0]);

        if (get_present_mode() == patched)
            window.display();
    }

    return 0;
}
