#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include "CommProto.pb.h"
#include "pool.h"
#include <string>
#include <netinet/in.h>
#include <pthread.h>

class Communicator {
public:
    int send_msg(const proto::CommProto &msg);
    int recv_msg(proto::CommProto &msg);

protected:
    int sockfd;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;
};

class ServerComm : public Communicator {
public:
    ServerComm(unsigned short port);
protected:
    int listen_sockfd;
};

class ClientComm : public Communicator {
public:
    ClientComm(std::string ip, unsigned short port);
};

class CommunicatorMT {
public:
    CommunicatorMT(Pool<proto::CommProto*> &sendMsgPool,
                   Pool<proto::CommProto*> &receiveMsgPool,
                   Queue<proto::CommProto*> &msgToSend,
                   Queue<proto::CommProto*> &msgReceived)
        : sendMsgPool(sendMsgPool), receiveMsgPool(receiveMsgPool),
          msgToSend(msgToSend), msgReceived(msgReceived)
    {}

    ~CommunicatorMT();

protected:
    void init_threads();
    static void* sender_service_entry(void* This){ ((CommunicatorMT*)This)->sender_service(); }
    static void* receiver_service_entry(void* This){ ((CommunicatorMT*)This)->receiver_service(); }
    virtual void sender_service() = 0;
    virtual void receiver_service() = 0;

    Pool<proto::CommProto*> &sendMsgPool;
    Pool<proto::CommProto*> &receiveMsgPool;
    Queue<proto::CommProto*> &msgToSend;
    Queue<proto::CommProto*> &msgReceived;

    pthread_t senderThread, receiverThread;
};

class ServerCommMT : public ServerComm, public CommunicatorMT {
public:
    ServerCommMT(unsigned short port,
                 Pool<proto::CommProto*> &sendMsgPool,
                 Pool<proto::CommProto*> &receiveMsgPool,
                 Queue<proto::CommProto*> &msgToSend,
                 Queue<proto::CommProto*> &msgReceived,
                 Queue<std::tuple<int, int, std::string>> &vsplitToSend);

private:
    void sender_service();
    void receiver_service();

    Queue<std::tuple<int, int, std::string>> &vsplitToSend;
};

class ClientCommMT : public ClientComm, public CommunicatorMT {
public:
    ClientCommMT(std::string &&ip,
                 unsigned short port,
                 Pool<proto::CommProto*> &sendMsgPool,
                 Pool<proto::CommProto*> &receiveMsgPool,
                 Queue<proto::CommProto*> &msgToSend,
                 Queue<proto::CommProto*> &msgReceived,
                 Queue<std::tuple<int, int, std::string>> &vsplitReceived);

private:
    void sender_service();
    void receiver_service();

    Queue<std::tuple<int, int, std::string>> &vsplitReceived;
};

#endif
