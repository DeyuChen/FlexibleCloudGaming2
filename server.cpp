#include "ffmpeg.h"
#include "PMeshController.h"
#include "PMeshRenderer.h"
#include "glWindow.h"
#include "codec.h"
#include "communicator.h"
#include <iostream>
#include <fstream>
#include <vector>
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
    proto::PMeshProto *pmesh_proto;
    proto::VsplitProto *vsp;

    glWindow window;
    window.create_window("Server", width, height);

    Encoder encoder("libx264", width, height, 8000000);

    vector<int> pmIDs;
    vector<int> pmrIDs;

    PMeshController pmController;
    ifstream ifs(argv[1], ifstream::in);
    pmIDs.push_back(pmController.create_pmesh(ifs));
    pmrIDs.push_back(window.add_pmesh(pmController.get_pmesh(pmIDs[0])));

    // send base mesh to the client
    pmesh_proto = comm.add_pmesh_proto();
    pmesh_proto->set_id(pmIDs[0]);
    pmesh_proto->set_pmesh_info(pmController.get_pmesh_info(pmIDs[0]));
    pmesh_proto->set_base_mesh(pmController.get_base_mesh(pmIDs[0]));
    comm.send_msg();

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
        auto [x, y] = comm.get_mouse_state();
        window.mouse_motion(x, y);

        for (int i = 0; i < comm.get_key_event_size(); i++){
            auto [down, key] = comm.get_key_event(i);
            window.key_event(down, key, x, y);
        }
        window.update_state();

        pmesh_proto = comm.get_pmesh_proto();
        window.set_nvertices(pmesh_proto->id(), pmesh_proto->nvertices());

        // flush all events to prevent overflow
        SDL_PumpEvents();
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

#ifdef SHOW_RENDERING_TIME
        auto begin = chrono::high_resolution_clock::now();
#endif

        window.render_diff_to_screen();
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

        // piggyback some vsplits
        // TODO: dynamically determine how many vsplits to send
        // TODO: consider unreliable send; should have a vsplit request mechanism
        pmesh_proto = comm.add_pmesh_proto();
        pmesh_proto->set_id(pmIDs[0]);
        for (int i = 0; i < 50; i++){
            auto [n, vsplit] = pmController.get_next_vsplit(pmIDs[0]);
            if (n == -1)
                break;
            vsp = pmesh_proto->add_vsplit();
            vsp->set_id(n);
            vsp->set_vsplit(vsplit);
        }
        comm.send_msg();

        av_packet_unref(pkt);
    }

    return 0;
}
