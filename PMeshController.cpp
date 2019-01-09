#include "PMeshController.h"
#include "PMeshInfo.pb.h"
#include <sstream>
#include <assert.h>

using namespace std;

int PMeshController::create_pmesh(istream& is){
    int id = pmeshes.size();

    hh::PMesh *pm = new hh::PMesh();
    pm->read(is);
    pmeshes.push_back(make_pair(0, pm));

    return id;
}

int PMeshController::create_pmesh(){
    int id = pmeshes.size();

    hh::PMesh *pm = new hh::PMesh();
    pmeshes.push_back(make_pair(0, pm));

    return id;
}

hh::PMesh& PMeshController::get_pmesh(int id){
    assert(id >= 0 && id < pmeshes.size());
    return *(pmeshes[id].second);
}

string PMeshController::get_pmesh_info(int id){
    hh::PMeshInfo &_info = pmeshes[id].second->_info;

    proto::PMeshInfo pmesh_info;
    pmesh_info.set_read_version(_info._read_version);
    pmesh_info.set_has_rgb(_info._has_rgb);
    pmesh_info.set_has_uv(_info._has_uv);
    pmesh_info.set_has_resid(_info._has_resid);
    pmesh_info.set_has_wad2(_info._has_wad2);
    pmesh_info.set_tot_nvsplits(_info._tot_nvsplits);
    pmesh_info.set_full_nvertices(_info._full_nvertices);
    pmesh_info.set_full_nwedges(_info._full_nwedges);
    pmesh_info.set_full_nfaces(_info._full_nfaces);
    pmesh_info.set_full_bbox00(_info._full_bbox[0][0]);
    pmesh_info.set_full_bbox01(_info._full_bbox[0][1]);
    pmesh_info.set_full_bbox02(_info._full_bbox[0][2]);
    pmesh_info.set_full_bbox10(_info._full_bbox[1][0]);
    pmesh_info.set_full_bbox11(_info._full_bbox[1][1]);
    pmesh_info.set_full_bbox12(_info._full_bbox[1][2]);

    string s;
    pmesh_info.SerializeToString(&s);

    return s;
}

void PMeshController::set_pmesh_info(int id, const string &info){
    hh::PMeshInfo &_info = pmeshes[id].second->_info;

    proto::PMeshInfo pmesh_info;
    pmesh_info.ParseFromString(info);

    _info._read_version = pmesh_info.read_version();
    _info._has_rgb = pmesh_info.has_rgb();
    _info._has_uv = pmesh_info.has_uv();
    _info._has_resid = pmesh_info.has_resid();
    _info._has_wad2 = pmesh_info.has_wad2();
    _info._tot_nvsplits = pmesh_info.tot_nvsplits();
    _info._full_nvertices = pmesh_info.full_nvertices();
    _info._full_nwedges = pmesh_info.full_nwedges();
    _info._full_nfaces = pmesh_info.full_nfaces();
    _info._full_bbox[0][0] = pmesh_info.full_bbox00();
    _info._full_bbox[0][1] = pmesh_info.full_bbox01();
    _info._full_bbox[0][2] = pmesh_info.full_bbox02();
    _info._full_bbox[1][0] = pmesh_info.full_bbox10();
    _info._full_bbox[1][1] = pmesh_info.full_bbox11();
    _info._full_bbox[1][2] = pmesh_info.full_bbox12();

    // TODO: determine a good amount to reserve,
    // as the client might not (or not allowed) need the whole mesh at all.
    pmeshes[id].second->_vsplits.reserve(_info._tot_nvsplits);
}

string PMeshController::get_base_mesh(int id){
    hh::PMesh *pmesh = pmeshes[id].second;
    ostringstream oss;
    pmesh->_base_mesh.write(oss, pmesh->_info);

    return oss.str();
}

void PMeshController::set_base_mesh(int id, const string &base_mesh){
    hh::PMesh *pmesh = pmeshes[id].second;
    istringstream iss(base_mesh);
    pmesh->_base_mesh.read(iss, pmesh->_info);
}

tuple<int, string> PMeshController::get_next_vsplit(int id){
    hh::PMesh *pmesh = pmeshes[id].second;
    ostringstream oss;
    if (pmeshes[id].first >= pmesh->_vsplits.num())
        return {-1, ""};
    pmesh->_vsplits[pmeshes[id].first].write(oss, pmesh->_info);
    return {pmeshes[id].first++, oss.str()};
}

void PMeshController::add_vsplit(int id, int n, const string &vsplit){
    hh::PMesh *pmesh = pmeshes[id].second;
    assert(n == pmesh->_vsplits.num());
    istringstream iss(vsplit);
    pmesh->_vsplits.resize(n + 1);
    pmesh->_vsplits[n].read(iss, pmesh->_info);
}

PMeshControllerServerMT::~PMeshControllerServerMT(){
    pthread_t thread;
    while (true){
        if (!threads.non_blocking_get(thread))
            break;
        pthread_cancel(thread);
        pthread_join(thread, NULL);
    }
}

int PMeshControllerServerMT::create_pmesh(istream& is){
    int pid = PMeshController::create_pmesh(is);
    init_vsp_serializer(pid);
    return pid;
}

void PMeshControllerServerMT::init_vsp_serializer(int id){
    tuple<int, PMeshControllerServerMT*> *args = new tuple<int, PMeshControllerServerMT*>(id, this);
    pthread_t thread;
    pthread_create(&thread, NULL, vsp_serializer_entry, (void*)args);
}

void PMeshControllerServerMT::vsp_serializer(int id){
    pthread_t thread = pthread_self();
    threads.put(thread);
    while (true){
        auto [n, vsplit] = get_next_vsplit(id);
        if (n == -1)
            break;
        vsplits.put({0, n, move(vsplit)});
    }
    threads.non_blocking_remove(thread);
    cout << "finished sending vsplits for pmesh " << id << endl;
}

PMeshControllerClientMT::PMeshControllerClientMT(
        Queue<tuple<int, int, string>> &vsplits,
        pthread_mutex_t &lock)
    : vsplits(vsplits), lock(lock)
{
    init_vsp_deserializer();
}

PMeshControllerClientMT::~PMeshControllerClientMT(){
    if (thread){
        pthread_cancel(thread);
        pthread_join(thread, NULL);
    }
}

void PMeshControllerClientMT::vsp_deserializer(){
    while (true){
        auto [id, n, vsplit] = vsplits.get();
        pthread_mutex_lock(&lock);
        add_vsplit(id, n, vsplit);
        pthread_mutex_unlock(&lock);
    }
}
