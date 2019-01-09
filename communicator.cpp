#include "ffmpeg.h"
#include "communicator.h"
#include <string.h>
#include <iostream>
#include <arpa/inet.h>

using namespace std;

int Communicator::send_msg(const proto::CommProto &msg){
    string s;
    msg.SerializeToString(&s);
    size_t size = s.size();
    string ss((char*)&size, sizeof(size));
    ss += s;

    return send(sockfd, ss.c_str(), ss.size(), 0);
}

int Communicator::recv_msg(proto::CommProto &msg){
    size_t size;
    recv(sockfd, &size, sizeof(size), 0);

    string s(size, '\0');
    int n = 0;
    while (n != size){
        n += recv(sockfd, &s[n], size - n, 0);
    }
    msg.ParseFromString(s);

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

CommunicatorMT::~CommunicatorMT(){
    if (senderThread){
        pthread_cancel(senderThread);
        pthread_join(senderThread, NULL);
    }
    if (receiverThread){
        pthread_cancel(receiverThread);
        pthread_join(receiverThread, NULL);
    }
}

void CommunicatorMT::init_threads(){
    pthread_create(&senderThread, NULL, sender_service_entry, this);
    pthread_create(&receiverThread, NULL, receiver_service_entry, this);
}

ServerCommMT::ServerCommMT(unsigned short port,
                           Pool<proto::CommProto*> &msgPool,
                           Queue<proto::CommProto*> &msgToSend,
                           Queue<proto::CommProto*> &msgReceived,
                           Queue<tuple<int, int, string>> &vsplitToSend)
    : ServerComm(port),
      CommunicatorMT(msgPool, msgToSend, msgReceived),
      vsplitToSend(vsplitToSend)
{
    init_threads();
}

void ServerCommMT::sender_service(){
    unordered_map<int, proto::PMeshProto*> pmesh_lookup;
    tuple<int, int, string> vsp_tuple;
    while (true){
        proto::CommProto *msg = msgToSend.get();

        // TODO: dynamically determine how many vsplits to send
        // TODO: consider unreliable send; should have a vsplit request mechanism
        for (int i = 0; i < 50; i++){
            if (!vsplitToSend.non_blocking_get(vsp_tuple))
                break;
            auto [id, n, vsplit] = vsp_tuple;
            if (pmesh_lookup.count(id) == 0){
                pmesh_lookup[id] = msg->add_pmesh();
            }
            pmesh_lookup[id]->set_id(id);
            proto::VsplitProto *vsp = pmesh_lookup[id]->add_vsplit();
            vsp->set_id(n);
            vsp->set_vsplit(vsplit);
        }

        send_msg(*msg);
        msg->Clear();
        msgPool.put(msg);

        pmesh_lookup.clear();
    }
}

void ServerCommMT::receiver_service(){
    while (true){
        proto::CommProto *msg = msgPool.get();
        recv_msg(*msg);
        msgReceived.put(msg);
    }
}

ClientCommMT::ClientCommMT(string &&ip,
                           unsigned short port,
                           Pool<proto::CommProto*> &msgPool,
                           Queue<proto::CommProto*> &msgToSend,
                           Queue<proto::CommProto*> &msgReceived)
    : ClientComm(ip, port),
      CommunicatorMT(msgPool, msgToSend, msgReceived)
{
    init_threads();
}

void ClientCommMT::sender_service(){
    while (true){
        proto::CommProto *msg = msgToSend.get();
        send_msg(*msg);
        msg->Clear();
        msgPool.put(msg);
    }
}

void ClientCommMT::receiver_service(){
    while (true){
        proto::CommProto *msg = msgPool.get();
        recv_msg(*msg);
        msgReceived.put(msg);
    }
}

