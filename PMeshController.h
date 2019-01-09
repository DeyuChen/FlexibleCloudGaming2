#ifndef PMESHCONTROLLER_H
#define PMESHCONTROLLER_H

#include "PMesh.h"
#include "pool.h"
#include <utility>
#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <pthread.h>

class PMeshController {
public:
    int create_pmesh(std::istream& is);
    int create_pmesh();
    hh::PMesh& get_pmesh(int id);

    std::string get_pmesh_info(int id);
    void set_pmesh_info(int id, const std::string &info);

    std::string get_base_mesh(int id);
    void set_base_mesh(int id, const std::string &base_mesh);

    std::tuple<int, std::string> get_next_vsplit(int id);
    void add_vsplit(int id, int n, const std::string &vsplit);

protected:
    std::vector<std::pair<int, hh::PMesh*>> pmeshes;
};

class PMeshControllerServerMT : public PMeshController {
public:
    // allow at most 3 meshes to send vsplits concurrently (there can be more idle threads though),
    // TODO: should implement a configurable way to set the limit
    PMeshControllerServerMT(Queue<std::tuple<int, int, std::string>> &vsplits)
        : vsplits(vsplits), threads(3){}

    ~PMeshControllerServerMT();

    int create_pmesh(std::istream& is);

private:
    void init_vsp_serializer(int id);
    static void* vsp_serializer_entry(void* ptr){
        std::tuple<int, PMeshControllerServerMT*> *args = (std::tuple<int, PMeshControllerServerMT*>*)ptr;
        auto [id, This] = *args;
        delete args;
        This->vsp_serializer(id);
    }
    void vsp_serializer(int id);

    Queue<std::tuple<int, int, std::string>> &vsplits;

    Pool<pthread_t> threads;
};

class PMeshControllerClientMT : public PMeshController {
public:
    PMeshControllerClientMT(Queue<std::tuple<int, int, std::string>> &vsplits, pthread_mutex_t &lock);

    ~PMeshControllerClientMT();

private:
    void init_vsp_deserializer(){ pthread_create(&thread, NULL, vsp_deserializer_entry, this); }
    static void* vsp_deserializer_entry(void* This){
        ((PMeshControllerClientMT*)This)->vsp_deserializer();
    }
    void vsp_deserializer();

    Queue<std::tuple<int, int, std::string>> &vsplits;

    pthread_t thread;
    pthread_mutex_t &lock;
};

#endif
