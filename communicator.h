#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include "CommProto.pb.h"
#include "pool.h"
#include <string>
#include <netinet/in.h>
#include <pthread.h>

class Communicator {
public:
    Communicator(Pool<proto::CommProto*> &msgPool,
                 Queue<proto::CommProto*> &msgToSend,
                 Queue<proto::CommProto*> &msgReceived)
        : msgPool(msgPool), msgToSend(msgToSend), msgReceived(msgReceived){};

    ~Communicator();

protected:
    int send_msg(const proto::CommProto &msg);
    int recv_msg(proto::CommProto &msg);

    void init_threads();
    static void* sender_service_entry(void* This){ ((Communicator*)This)->sender_service(); }
    static void* receiver_service_entry(void* This){ ((Communicator*)This)->receiver_service(); }
    virtual void sender_service() = 0;
    void receiver_service();

    int sockfd;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;

    Pool<proto::CommProto*> &msgPool;
    Queue<proto::CommProto*> &msgToSend;
    Queue<proto::CommProto*> &msgReceived;

    pthread_t senderThread, receiverThread;
};

class ServerComm : public Communicator {
public:
    ServerComm(unsigned short port,
               Pool<proto::CommProto*> &msgPool,
               Queue<proto::CommProto*> &msgToSend,
               Queue<proto::CommProto*> &msgReceived,
               Queue<std::tuple<int, int, std::string>> &vsplitToSend);

private:
    void sender_service();

    int listen_sockfd;

    Queue<std::tuple<int, int, std::string>> &vsplitToSend;
};

class ClientComm : public Communicator {
public:
    ClientComm(std::string &&ip,
               unsigned short port,
               Pool<proto::CommProto*> &msgPool,
               Queue<proto::CommProto*> &msgToSend,
               Queue<proto::CommProto*> &msgReceived);

private:
    void sender_service();
};

#endif
