#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include "CommProto.pb.h"
#include "pool.h"
#include <string>
#include <netinet/in.h>

class Communicator {
public:
    Communicator(Pool<proto::CommProto*> &msgPool,
                 Pool<proto::CommProto*> &msgToSend,
                 Pool<proto::CommProto*> &msgReceived)
        : msgPool(msgPool), msgToSend(msgToSend), msgReceived(msgReceived){};

    ~Communicator();

    int send_msg(const proto::CommProto &msg);
    int recv_msg(proto::CommProto &msg);

    bool init_sender_thread();
    bool init_receiver_thread();

protected:
    static void* sender_service_entry(void* This){((Communicator*)This)->sender_service();}
    static void* receiver_service_entry(void* This){((Communicator*)This)->receiver_service();}
    void sender_service();
    void receiver_service();

    int sockfd;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;

    Pool<proto::CommProto*> &msgPool;
    Pool<proto::CommProto*> &msgToSend;
    Pool<proto::CommProto*> &msgReceived;

    pthread_t senderThread, receiverThread;
};

class ServerComm : public Communicator {
public:
    ServerComm(unsigned short port,
               Pool<proto::CommProto*> &msgPool,
               Pool<proto::CommProto*> &msgToSend,
               Pool<proto::CommProto*> &msgReceived);
private:
    int listen_sockfd;
};

class ClientComm : public Communicator {
public:
    ClientComm(std::string &&ip,
               unsigned short port,
               Pool<proto::CommProto*> &msgPool,
               Pool<proto::CommProto*> &msgToSend,
               Pool<proto::CommProto*> &msgReceived);
};

#endif
