#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include "CommonProtocol.pb.h"
#include <string>
#include <netinet/in.h>
#include <tuple>

class Communicator {
public:
    void set_mouse_state(int x, int y);
    std::tuple<int, int> get_mouse_state();
    void add_key_event(bool down, int key);
    int get_key_event_size();
    std::tuple<bool, int> get_key_event(int id);
    void set_diff_frame(AVPacket *pkt);
    void get_diff_frame(AVPacket *pkt);

    int send_msg();
    int recv_msg();

    int get_sockfd(){ return sockfd; }

protected:
    int sockfd;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;

    CommonProtocol outMessage;
    CommonProtocol inMessage;
};

class ServerComm : public Communicator {
public:
    ServerComm(unsigned short port);
private:
    int listen_sockfd;
};

class ClientComm : public Communicator {
public:
    ClientComm(std::string ip, unsigned short port);
};

#endif
