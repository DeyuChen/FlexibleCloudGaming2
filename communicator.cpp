#include "ffmpeg.h"
#include "communicator.h"
#include <string.h>
#include <iostream>
#include <arpa/inet.h>

using namespace std;

void Communicator::set_mouse_state(int x, int y){
    outMessage.set_mouse_x(x);
    outMessage.set_mouse_y(y);
}

tuple<int, int> Communicator::get_mouse_state(){
    return {inMessage.mouse_x(), inMessage.mouse_y()};
}

void Communicator::add_key_event(bool down, int key){
    KeyEvent *key_event = outMessage.add_key_event();
    key_event->set_down(down);
    key_event->set_key(key);
}

int Communicator::get_key_event_size(){
    return inMessage.key_event_size();
}

tuple<bool, int> Communicator::get_key_event(int id){
    return {inMessage.key_event(id).down(), inMessage.key_event(id).key()};
}

void Communicator::set_diff_frame(AVPacket *pkt){
    string frame((char*)pkt->data, pkt->size);
    outMessage.set_diff_frame(frame);
}

void Communicator::get_diff_frame(AVPacket *pkt){
    string *frame = inMessage.mutable_diff_frame();
    pkt->size = frame->size();
    pkt->data = (uint8_t*)(frame->c_str());
}

int Communicator::send_msg(){
    string s;
    outMessage.SerializeToString(&s);
    outMessage.Clear();
    size_t size = s.size();
    string ss((char*)&size, sizeof(size));
    ss += s;

    return send(sockfd, ss.c_str(), ss.size(), 0);
}

int Communicator::recv_msg(){
    size_t size;
    recv(sockfd, &size, sizeof(size), 0);

    string s(size, '\0');
    int n = 0;
    while (n != size){
        n += recv(sockfd, &s[n], size - n, 0);
    }
    inMessage.ParseFromString(s);
    return size;
}

ServerComm::ServerComm(unsigned short port){
    listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sockfd < 0) {
        cerr << "ERROR opening socket" << endl;
        exit(1);
    }

    int opt = 1;
    setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    memset((char*)&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(port);

    if (bind(listen_sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0){
        cerr << "ERROR on binding" << endl;
        exit(1);
    }

    listen(listen_sockfd, 1);

    cout << "Server started listening to port " << port << endl;

    socklen_t addrlen = sizeof(remote_addr);
    sockfd = accept(listen_sockfd, (struct sockaddr*)&remote_addr, &addrlen);
    if (sockfd < 0) {
        cerr << "ERROR on accept" << endl;
        exit(1);
    }
}

ClientComm::ClientComm(string ip, unsigned short port){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "ERROR opening socket" << endl;
        exit(1);
    }

    memset((char*)&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    remote_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
        cerr << "ERROR on connecting" << endl;
        exit(1);
    }
}
