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

class PMeshControllerMT : public PMeshController {
public:
    // allow at most 3 meshes to send vsplits concurrently (there can be more idle threads though),
    // TODO: should implement a configurable way to set the limit
    PMeshControllerMT(Queue<std::tuple<int, int, std::string>> &vsplits)
        : vsplits(vsplits), threads(3){}

    ~PMeshControllerMT();

    void init_vsp_generator(int id);

private:
    static void* vsp_generator_entry(void* ptr){
        std::tuple<int, PMeshControllerMT*> *args = (std::tuple<int, PMeshControllerMT*>*)ptr;
        auto [id, This] = *args;
        delete args;
        This->vsp_generator(id);
    }
    void vsp_generator(int id);

    Queue<std::tuple<int, int, std::string>> &vsplits;

    Pool<pthread_t> threads;
};

#endif
