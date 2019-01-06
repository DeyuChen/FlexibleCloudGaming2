#include "ffmpeg.h"
#include "PMeshController.h"
#include "PMeshRenderer.h"
#include "glWindow.h"
#include "communicator.h"
#include "codec.h"
#include "CommProto.pb.h"
#include "pool.h"
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

    Pool<proto::CommProto*> msgPool(2);
    Pool<proto::CommProto*> msgToSend(1);
    Pool<proto::CommProto*> msgReceived(1);
    for (int i = 0; i < 2; i++)
        msgPool.put(new proto::CommProto());

    ServerComm comm(9999, msgPool, msgToSend, msgReceived);

    proto::CommProto *msg;
    proto::PMeshProto *pmeshProto;
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
    msg = msgPool.get();
    pmeshProto = msg->add_pmesh();
    pmeshProto->set_id(pmIDs[0]);
    pmeshProto->set_pmesh_info(pmController.get_pmesh_info(pmIDs[0]));
    pmeshProto->set_base_mesh(pmController.get_base_mesh(pmIDs[0]));
    msgToSend.put(msg);

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
    while (!quit){
        msg = msgReceived.get();
        if (msg->disconnect()){
            quit = true;
            cout << "Client disconnected, terminating" << endl;
            break;
        }
        int x = msg->mouse_x();
        int y = msg->mouse_y();
        window.mouse_motion(x, y);

        for (int i = 0; i < msg->key_event_size(); i++){
            bool down = msg->key_event(i).down();
            int key = msg->key_event(i).key();
            window.key_event(down, key, x, y);
        }
        window.update_state();

        window.set_nvertices(msg->pmesh(0).id(), msg->pmesh(0).nvertices());
        msg->Clear();
        msgPool.put(msg);

        // flush all events to prevent overflow
        SDL_PumpEvents();
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

#ifdef SHOW_RENDERING_TIME
        auto begin = chrono::high_resolution_clock::now();
#endif

        int texid = window.render_diff(texture);
        //window.display();

#ifdef SHOW_RENDERING_TIME
        auto end = chrono::high_resolution_clock::now();
        auto dur = end - begin;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
        cout << "rendering time: " << ms << "ms" << endl;
#endif

        // encoding
        window.read_pixels(texid, frame->data[0]);
        window.release_texture(texid);
        if (encoder.encode(frame, pkt)){
            //cout << pkt->size << endl;
        }

        msg = msgPool.get();
        string frameStr((char*)pkt->data, pkt->size);
        av_packet_unref(pkt);
        msg->set_diff_frame(frameStr);

        // piggyback some vsplits
        // TODO: dynamically determine how many vsplits to send
        // TODO: consider unreliable send; should have a vsplit request mechanism
        pmeshProto = msg->add_pmesh();
        pmeshProto->set_id(pmIDs[0]);
        for (int i = 0; i < 50; i++){
            auto [n, vsplit] = pmController.get_next_vsplit(pmIDs[0]);
            if (n == -1)
                break;
            vsp = pmeshProto->add_vsplit();
            vsp->set_id(n);
            vsp->set_vsplit(vsplit);
        }
        msgToSend.put(msg);
    }

    return 0;
}
