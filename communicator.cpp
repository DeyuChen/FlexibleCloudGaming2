#include "communicator.h"
#include <string.h>
#include <iostream>
#include <arpa/inet.h>

using namespace std;

void Communicator::set_mouse_state(int x, int y){
    outMessage.set_mouse_x(x);
    outMessage.set_mouse_y(y);
}

void Communicator::get_mouse_state(int &x, int &y){
    x = inMessage.mouse_x();
    y = inMessage.mouse_y();
}

void Communicator::add_key_press(int key){
    outMessage.add_key_press(key);
}

int Communicator::get_key_press_size(){
    return inMessage.key_press_size();
}

int Communicator::get_key_press(int id){
    return inMessage.key_press(id);
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
    if (size > 0){
        n = recv(sockfd, &s[0], size, 0);
    }
    inMessage.Clear();
    inMessage.ParseFromString(s);
    return n;
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
