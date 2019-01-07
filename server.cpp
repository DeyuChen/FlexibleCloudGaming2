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
    Queue<proto::CommProto*> msgToSend(1);
    Queue<proto::CommProto*> msgReceived(1);
    Queue<proto::CommProto*> msgToRender(1);
    Queue<tuple<int, int, string>> vsplitToSend(50);
    Pool<AVFrame*> framePool(1);
    Queue<AVFrame*> frameToEncode(1);

    // initializing the communicator which handle network communications
    ServerComm comm(9999, msgPool, msgToSend, msgReceived, vsplitToSend);
    proto::CommProto *msg;
    proto::PMeshProto *pmeshProto;
    proto::VsplitProto *vsp;

    for (int i = 0; i < 2; i++)
        msgPool.put(new proto::CommProto());

    // initializing SDL window which also handles all opengl rendering
    glWindowServer window(msgPool, msgToRender, framePool, frameToEncode);
    window.create_window("Server", width, height);

    // initializing the video encoder
    Encoder encoder("libx264", width, height, 8000000, msgPool, msgToSend, framePool, frameToEncode);

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
    framePool.put(frame);

    // initializing the progressive mesh controller
    vector<int> pmIDs;
    vector<int> pmrIDs;

    PMeshController pmController(vsplitToSend);
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

    pmController.init_vsp_generator(pmIDs[0]);

    bool quit = false;
    while (!quit){
        // flush all events to prevent overflow
        SDL_PumpEvents();
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

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
        frame = framePool.get();
        window.read_pixels(texid, frame->data[0]);
        window.release_texture(texid);
        frameToEncode.put(frame);
    }

    return 0;
}
