#ifndef PMESHCONTROLLER_H
#define PMESHCONTROLLER_H

#include "PMesh.h"
#include <utility>
#include <iostream>
#include <vector>
#include <string>

class PMeshController {
public:
    PMeshController(){}
    int create_pmesh(std::istream& is);
    int create_pmesh();
    hh::PMesh& get_pmesh(int id);

    void set_pmesh_info(int id, const std::string &info);
    std::string get_pmesh_info(int id);

    void set_base_mesh(int id, const std::string &base_mesh);
    std::string get_base_mesh(int id);

    void add_vsplit(int id, const std::string &vsplit);
    std::string get_next_vsplit(int id);

private:
    std::vector<std::pair<int, hh::PMesh*>> pmeshes;
};

#endif
