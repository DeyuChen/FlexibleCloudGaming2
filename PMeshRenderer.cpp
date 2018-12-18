#include "PMeshRenderer.h"
#include <iostream>

using namespace std;

void hh::AWMesh::ogl_render_faces_individually(const hh::PMeshInfo& pminfo, int usetexture){
    
}

PMeshRenderer::PMeshRenderer(const hh::PMesh& pm) : pmrs(pm), pmi(pmrs){
    numTex = pmi._materials.num();
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
        update_buffer_triangle();
    }
}

void PMeshRenderer::init_buffer(){
    glGenBuffers(1, &VBO);
    VAO.resize(numTex);
    IBO.resize(numTex);
    indSizes.resize(numTex, 0);
    glGenVertexArrays(numTex, VAO.data());
    glGenBuffers(numTex, IBO.data());
}

void PMeshRenderer::update_buffer_triangle(){
    // number of floats per vertex
    // minimum point * 3 + normal * 3
    int vertexDataSize = 6;
    if (pmrs._info._has_uv){
        vertexDataSize += 2;
    } else if (pmrs._info._has_rgb){
        vertexDataSize += 3;
    }

    vector<GLfloat> wedges(vertexDataSize * pmi._wedges.num());

    int i = 0;
    for (int w = 0; w < pmi._wedges.num(); w++){
        int v = pmi._wedges[w].vertex;
        memcpy(&wedges[i], pmi._vertices[v].attrib.point.data(), 3 * sizeof(GLfloat));
        memcpy(&wedges[i+3], pmi._wedges[w].attrib.normal.data(), 3 * sizeof(GLfloat));
        if (pmrs._info._has_uv){
            memcpy(&wedges[i+6], pmi._wedges[w].attrib.uv.data(), 2 * sizeof(GLfloat));
        } else if (pmrs._info._has_rgb){
            memcpy(&wedges[i+6], pmi._wedges[w].attrib.rgb.data(), 3 * sizeof(GLfloat));
        }
        i += vertexDataSize;
    }
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * wedges.size(), wedges.data(), GL_STATIC_DRAW);

    vector<vector<GLuint>> indices(numTex);
    for (i = 0; i < numTex; i++){
        indices[i].reserve(3 * pmi._faces.num());
    }
    for (int f = 0; f < pmi._faces.num(); f++){
        int matid = pmi._faces[f].attrib.matid;
        for (i = 0; i < 3; i++){
            indices[matid].push_back(pmi._faces[f].wedges[i]);
        }
    }

    for (i = 0; i < numTex; i++){
        glBindVertexArray(VAO[i]);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexDataSize * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexDataSize * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        if (pmrs._info._has_uv){
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexDataSize * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
            glEnableVertexAttribArray(2);
        } else if (pmrs._info._has_rgb){
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, vertexDataSize * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
            glEnableVertexAttribArray(3);
        }

        indSizes[i] = indices[i].size();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO[i]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indSizes[i], indices[i].data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }
}
