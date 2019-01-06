#include "ffmpeg.h"
#include "communicator.h"
#include <string.h>
#include <iostream>
#include <arpa/inet.h>

using namespace std;

Communicator::~Communicator(){
    if (senderThread){
        pthread_cancel(senderThread);
        pthread_join(senderThread, NULL);
    }
    if (receiverThread){
        pthread_cancel(receiverThread);
        pthread_join(receiverThread, NULL);
    }
}

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

bool Communicator::init_sender_thread(){
    return (pthread_create(&senderThread, NULL, sender_service_entry, this) == 0);
}

bool Communicator::init_receiver_thread(){
    return (pthread_create(&receiverThread, NULL, receiver_service_entry, this) == 0);
}

void Communicator::sender_service(){
    while (true){
        proto::CommProto *msg = msgToSend.get();
        send_msg(*msg);
        msg->Clear();
        msgPool.put(msg);
    }
}

void Communicator::receiver_service(){
    while (true){
        proto::CommProto *msg = msgPool.get();
        recv_msg(*msg);
        msgReceived.put(msg);
    }
}

ServerComm::ServerComm(unsigned short port,
                       Pool<proto::CommProto*> &msgPool,
                       Pool<proto::CommProto*> &msgToSend,
                       Pool<proto::CommProto*> &msgReceived)
    : Communicator(msgPool, msgToSend, msgReceived)
{
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

ClientComm::ClientComm(string &&ip,
                       unsigned short port,
                       Pool<proto::CommProto*> &msgPool,
                       Pool<proto::CommProto*> &msgToSend,
                       Pool<proto::CommProto*> &msgReceived)
    : Communicator(msgPool, msgToSend, msgReceived)
{
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
