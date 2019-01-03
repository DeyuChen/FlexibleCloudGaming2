#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include "CommProto.pb.h"
#include <string>
#include <netinet/in.h>

class Communicator {
public:
    int send_msg(const proto::CommProto &message);
    int recv_msg(proto::CommProto &message);

protected:
    int sockfd;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;

    proto::CommProto outMessage;
    proto::CommProto inMessage;
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
