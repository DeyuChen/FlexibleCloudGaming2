#include "ffmpeg.h"
#include "PMeshController.h"
#include "PMeshRenderer.h"
#include "glWindow.h"
#include "communicator.h"
#include "codec.h"
#include "CommProto.pb.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <chrono>

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
    proto::CommProto message;
    proto::KeyEvent *keyEvent;
    proto::PMeshProto *pmeshProto;

    set<int> keyToSend = {SDLK_w, SDLK_d, SDLK_s, SDLK_a, SDLK_e, SDLK_q};

    glWindow window;
    window.create_window("Client", width, height);

    Decoder decoder("h264", width, height);

    vector<int> pmIDs;
    vector<int> pmrIDs;

    // receive base mesh from the server
    // TODO: currently assume there will be only one mesh
    // TODO: it might be more efficient to handle the whole communication protocol in main
    comm.recv_msg(message);

    PMeshController pmController;
    pmIDs.push_back(pmController.create_pmesh());
    pmController.set_pmesh_info(pmIDs[0], message.pmesh(0).pmesh_info());
    pmController.set_base_mesh(pmIDs[0], message.pmesh(0).base_mesh());
    pmrIDs.push_back(window.add_pmesh(pmController.get_pmesh(pmIDs[0])));

    SDL_Event e;
    SDL_SetRelativeMouseMode(SDL_TRUE);

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

#ifdef SHOW_FRAME_DELAY
    auto begin = chrono::high_resolution_clock::now();
#endif

    bool quit = false;
    while(!quit){
        message.Clear();

        int x = 0, y = 0;
        SDL_GetRelativeMouseState(&x, &y);
        window.mouse_motion(x, y);
        message.set_mouse_x(x);
        message.set_mouse_y(y);

        while (SDL_PollEvent(&e) != 0){
            if (e.type == SDL_QUIT){
                quit = true;
            } else if (e.type == SDL_KEYDOWN && e.key.repeat == 0){
                if (e.key.keysym.sym == SDLK_m){
                    switch_present_mode();
                }
                else {
                    window.key_event(true, e.key.keysym.sym, x, y);
                    if (keyToSend.count(e.key.keysym.sym)){
                        keyEvent = message.add_key_event();
                        keyEvent->set_down(true);
                        keyEvent->set_key(e.key.keysym.sym);
                    }
                }
            } else if (e.type == SDL_KEYUP){
                window.key_event(false, e.key.keysym.sym, x, y);
                if (keyToSend.count(e.key.keysym.sym)){
                    keyEvent = message.add_key_event();
                    keyEvent->set_down(false);
                    keyEvent->set_key(e.key.keysym.sym);
                }
            }
        }
        window.update_state();

        pmeshProto = message.add_pmesh();
        pmeshProto->set_id(pmrIDs[0]);
        pmeshProto->set_nvertices(window.get_nvertices(pmrIDs[0]));

        comm.send_msg(message);

        window.render_simp_to_texture0();

        message.Clear();
        comm.recv_msg(message);

        pkt->size = message.diff_frame().size();
        pkt->data = (uint8_t*)message.diff_frame().c_str();
        decoder.decode(frame, pkt);
        av_packet_unref(pkt);

        if (get_present_mode() == simplified)
            memset(frame->data[0], 127, 4 * width * height);
        window.render_sum_to_screen(frame->data[0]);

        window.display();

#ifdef SHOW_FRAME_DELAY
        auto end = chrono::high_resolution_clock::now();
        auto dur = end - begin;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
        cout << "frame delay: " << ms << "ms" << endl;
        begin = chrono::high_resolution_clock::now();;
#endif

        // read piggybacked vsplits
        pmeshProto = message.mutable_pmesh(0);
        for (int i = 0; i < pmeshProto->vsplit_size(); i++){
            pmController.add_vsplit(pmeshProto->id(), pmeshProto->vsplit(i).id(), pmeshProto->vsplit(i).vsplit());
        }
    }

    return 0;
}
