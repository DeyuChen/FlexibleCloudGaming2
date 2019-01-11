#include "ffmpeg.h"
#include "PMeshController.h"
#include "PMeshRenderer.h"
#include "glWindow.h"
#include "communicator.h"
#include "codec.h"
#include "tokenGenerator.h"
#include "CommProto.pb.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <chrono>

#define SHOW_FRAME_DELAY

using namespace std;

const int width = 1280;
const int height = 960;

PresentMode pmode = patched;

void switch_present_mode(){
    pmode = (pmode == simplified)? patched : simplified;
}

int main(int argc, char *argv[]){
    Pool<proto::CommProto*> sendMsgPool(1);
    Pool<proto::CommProto*> receiveMsgPool(1);
    Queue<proto::CommProto*> msgToUpdate(1);
    Queue<proto::CommProto*> msgToSend(1);
    Queue<proto::CommProto*> msgReceived(1);
    Queue<tuple<int, int, string>> vsplitReceived(50);
    Pool<AVFrame*> framePool(1);
    Queue<AVFrame*> frameDecoded(1);
    Queue<bool> tokenBucket(3);

    ClientCommMT comm("127.0.0.1", 9999, sendMsgPool, receiveMsgPool, msgToSend, msgReceived, vsplitReceived);
    proto::CommProto *msg;
    proto::KeyEvent *keyEvent;

    sendMsgPool.put(new proto::CommProto());
    receiveMsgPool.put(new proto::CommProto());

    set<int> keyToSend = {SDLK_w, SDLK_d, SDLK_s, SDLK_a, SDLK_e, SDLK_q};

    glWindowClientMT window("Client", width, height, pmode, msgToUpdate, msgToSend, framePool, frameDecoded, tokenBucket);

    DecoderMT decoder("h264", width, height, receiveMsgPool, msgReceived, framePool, frameDecoded);

    vector<int> pmIDs;
    vector<int> pmrIDs;

    // nmake sure we will not add vsplits when the pmesh is used for rendering
    pthread_mutex_t pmesh_lock;
    pthread_mutex_init(&pmesh_lock, NULL);

    // receive base mesh from the server
    // TODO: currently assume there will be only one mesh
    msg = msgReceived.get();
    PMeshControllerClientMT pmController(vsplitReceived, pmesh_lock);
    pthread_mutex_lock(&pmesh_lock);
    // TODO: these should be handled by thread
    pmIDs.push_back(pmController.create_pmesh());
    pmController.set_pmesh_info(pmIDs[0], msg->pmesh(0).pmesh_info());
    pmController.set_base_mesh(pmIDs[0], msg->pmesh(0).base_mesh());
    pmrIDs.push_back(window.add_pmesh(pmController.get_pmesh(pmIDs[0])));
    pthread_mutex_unlock(&pmesh_lock);
    msg->Clear();
    receiveMsgPool.put(msg);

    SDL_Event e;
    SDL_SetRelativeMouseMode(SDL_TRUE);

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

    // token bucket for rate control
    TokenGenerator tokenGen(tokenBucket, 30);

#ifdef SHOW_FRAME_DELAY
    auto begin = chrono::high_resolution_clock::now();
#endif

    bool quit = false;
    while (!quit){
        msg = sendMsgPool.get();
        int x = 0, y = 0;
        SDL_GetRelativeMouseState(&x, &y);
        window.mouse_motion(x, y);
        msg->set_mouse_x(x);
        msg->set_mouse_y(y);

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
                        keyEvent = msg->add_key_event();
                        keyEvent->set_down(true);
                        keyEvent->set_key(e.key.keysym.sym);
                    }
                }
            } else if (e.type == SDL_KEYUP){
                window.key_event(false, e.key.keysym.sym, x, y);
                if (keyToSend.count(e.key.keysym.sym)){
                    keyEvent = msg->add_key_event();
                    keyEvent->set_down(false);
                    keyEvent->set_key(e.key.keysym.sym);
                }
            }
        }
        msgToUpdate.put(msg);

#ifdef SHOW_FRAME_DELAY
        auto end = chrono::high_resolution_clock::now();
        auto dur = end - begin;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
        cout << "frame delay: " << ms << "ms" << endl;
        begin = chrono::high_resolution_clock::now();;
#endif
    }

    msg = sendMsgPool.get();
    msg->set_disconnect(true);
    msgToSend.put(msg);

    window.kill_window();

    return 0;
}
