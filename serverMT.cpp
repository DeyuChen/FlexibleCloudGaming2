#include "ffmpeg.h"
#include "PMeshController.h"
#include "PMeshRenderer.h"
#include "glWindow.h"
#include "communicator.h"
#include "codec.h"
#include "CommProto.pb.h"
#include "pool.h"
#include "frame3d.h"
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

    Pool<proto::CommProto*> sendMsgPool(1);
    Pool<proto::CommProto*> receiveMsgPool(1);
    Queue<proto::CommProto*> msgToSend(1);
    Queue<proto::CommProto*> msgReceived(1);
    Queue<proto::CommProto*> msgToRender(1);
    Queue<tuple<int, int, string>> vsplitToSend(50);
    Pool<Frame3D*> framePool(1);
    Queue<Frame3D*> frameToEncode(1);

    // initializing the communicator which handle network communications
    ServerCommMT comm(9999, sendMsgPool, receiveMsgPool, msgToSend, msgReceived, vsplitToSend);
    proto::CommProto *msg;
    proto::PMeshProto *pmeshProto;

    sendMsgPool.put(new proto::CommProto());
    receiveMsgPool.put(new proto::CommProto());

    // initializing SDL window which also handles all opengl rendering
    glWindowServerMT window("Server", width, height, receiveMsgPool, msgToRender, framePool, frameToEncode);

    // initializing the video encoder
    EncoderMT encoder("libx264", width, height, 8000000, sendMsgPool, msgToSend, framePool, frameToEncode);

    Frame3D *frame = new Frame3D(width, height, AV_PIX_FMT_RGBA);
    framePool.put(frame);

    // initializing the progressive mesh controller
    vector<int> pmIDs;
    vector<int> pmrIDs;

    PMeshControllerServerMT pmController(vsplitToSend);
    ifstream ifs(argv[1], ifstream::in);
    pmIDs.push_back(pmController.create_pmesh(ifs));
    pmrIDs.push_back(window.add_pmesh(pmController.get_pmesh(pmIDs[0])));

    // send base mesh to the client
    msg = sendMsgPool.get();
    pmeshProto = msg->add_pmesh();
    pmeshProto->set_id(pmIDs[0]);
    pmeshProto->set_pmesh_info(pmController.get_pmesh_info(pmIDs[0]));
    pmeshProto->set_base_mesh(pmController.get_base_mesh(pmIDs[0]));
    msgToSend.put(msg);

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
        msgToRender.put(msg);
    }

    window.kill_window();

    return 0;
}
