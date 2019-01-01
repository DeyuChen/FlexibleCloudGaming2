#include "PMeshRenderer.h"
#include "GMesh.h"
#include <iostream>

using namespace std;

PMeshRenderer::PMeshRenderer(const hh::PMesh& pm) : pmrs(pm), pmi(pmrs){
    numTex = pmi._materials.num();

    // number of floats per vertex
    // minimum point * 3 + normal * 3
    vertexDataSize = 6;
    if (pmrs._info._has_uv){
        vertexDataSize += 2;
    } else if (pmrs._info._has_rgb){
        vertexDataSize += 3;
    }

    init_buffer(MeshMode::simp);
    init_buffer(MeshMode::full);
    init_default_colors();
}

PMeshRenderer::~PMeshRenderer(){
}

bool PMeshRenderer::next(){
    simpBuffer.dirty = pmi.next();
    return simpBuffer.dirty;
}

bool PMeshRenderer::prev(){
    simpBuffer.dirty = pmi.prev();
    return simpBuffer.dirty;
}

bool PMeshRenderer::goto_vpercentage(int percentage){
    int nv = pmrs._info._full_nvertices * percentage / 100;
    simpBuffer.dirty = pmi.goto_nvertices(nv);
    return simpBuffer.dirty;
}

int PMeshRenderer::get_vpercentage(){
    return pmi._vertices.num() * 100 / pmrs._info._full_nvertices;
}

void PMeshRenderer::render(GLuint pid, MeshMode mode){
    VBufferInfo *vBuffer;
    if (mode == simp){
        vBuffer = &simpBuffer;
    } else if (mode == full){
        vBuffer = &fullBuffer;
    }

    if (vBuffer->dirty){
        update_buffer_triangle(mode);
    }

    glm::mat4 model;

    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0, 0, 0);
    glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(pid, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(pid, "hasTex"), pmrs._info._has_uv);
    glUniform1i(glGetUniformLocation(pid, "hasColor"), pmrs._info._has_rgb);
    for (int matid = 0; matid < pmi._materials.num(); matid++){
        if (pmrs._info._has_uv){
            glActiveTexture(GL_TEXTURE0);
            //glBindTexture(GL_TEXTURE_2D, pmesh_list[m].mesh->getMesh()->getTexID(i - 1));
        } else if (!pmrs._info._has_rgb){
            glUniform3fv(glGetUniformLocation(pid, "defaultColor"), 1, defaultColors[matid].data());
        }

        glBindVertexArray(vBuffer->VAO[matid]);
        glDrawElements(GL_TRIANGLES, vBuffer->indSizes[matid], GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    glPopMatrix();
}

void PMeshRenderer::init_buffer(MeshMode mode){
    VBufferInfo *vBuffer;
    if (mode == simp){
        vBuffer = &simpBuffer;
    } else if (mode == full){
        vBuffer = &fullBuffer;
    }

    vBuffer->VBO.resize(1);
    vBuffer->VAO.resize(numTex);
    vBuffer->IBO.resize(numTex);
    vBuffer->indSizes.resize(numTex, 0);
    glGenBuffers(1, vBuffer->VBO.data());
    glGenVertexArrays(numTex, vBuffer->VAO.data());
    glGenBuffers(numTex, vBuffer->IBO.data());

    for (int i = 0; i < numTex; i++){
        glBindVertexArray(vBuffer->VAO[i]);

        glBindBuffer(GL_ARRAY_BUFFER, vBuffer->VBO[0]);
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

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vBuffer->IBO[i]);

        glBindVertexArray(0);
    }

    vBuffer->dirty = true;
}

void PMeshRenderer::init_default_colors(){
    defaultColors.reserve(pmi._materials.num());
    for (int matid = 0; matid < pmi._materials.num(); matid++){
        const string& str = pmi._materials.get(matid);
        hh::Vec3<float> co;
        if (!hh::parse_key_vec(str.c_str(), "rgb", co)) {
            // co = V(.8f, .5f, .4f);
            co = hh::V(.6f, .6f, .6f);
        }
        defaultColors.push(co);
    }
}

void PMeshRenderer::update_buffer_triangle(MeshMode mode){
    VBufferInfo *vBuffer;
    int nv;
    if (mode == simp){
        vBuffer = &simpBuffer;
    } else if (mode == full){
        vBuffer = &fullBuffer;
        nv = pmi._vertices.num();
        pmi.goto_nvertices(pmrs._info._full_nvertices);
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
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer->VBO[0]);
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
        vBuffer->indSizes[i] = indices[i].size();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vBuffer->IBO[i]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * vBuffer->indSizes[i], indices[i].data(), GL_STATIC_DRAW);
    }
    
    vBuffer->dirty = false;

    if (mode == full){
        pmi.goto_nvertices(nv);
    }
}
