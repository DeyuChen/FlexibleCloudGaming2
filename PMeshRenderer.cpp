#include "PMeshRenderer.h"
#include <iostream>

using namespace std;

void hh::AWMesh::ogl_render_faces_individually(const hh::PMeshInfo& pminfo, int usetexture){
    
}

PMeshRenderer::PMeshRenderer(const hh::PMesh& pm) : pmrs(pm), pmi(pmrs){
    init_buffer();
}

PMeshRenderer::~PMeshRenderer(){
}

bool PMeshRenderer::next(){
    bool result = pmi.next();
    if (result)
        buffer_dirty = true;
    return result;
}

bool PMeshRenderer::prev(){
    bool result = pmi.prev();
    if (result)
        buffer_dirty = true;
    return result;
}

int PMeshRenderer::nfaces(){
    return pmi._faces.num();
}

void PMeshRenderer::render(){
    if (buffer_dirty){
        update_buffer();
    }
}

void PMeshRenderer::init_buffer(){
    glGenBuffers(1, &VBO);
    int numTex = pmi._materials.num();
    VAO.resize(numTex);
    IBO.resize(numTex);
    indSizes.resize(numTex, 0);
    glGenVertexArrays(numTex, VAO.data());
    glGenBuffers(numTex, IBO.data());
}

void PMeshRenderer::update_buffer(){
    
}
