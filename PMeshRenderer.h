#ifndef PMESHRENDERER_H
#define PMESHRENDERER_H

#include "glHeader.h"
#include "PMesh.h"
#include "common.h"
#include <string>
#include <vector>
#include <fstream>

enum MeshMode {
    simp,
    full
};

class PMeshRenderer {
public:
    PMeshRenderer(const hh::PMesh& pm);
    ~PMeshRenderer();
    bool next();
    bool prev();
    bool goto_vpercentage(int percentage);
    int get_vpercentage();
    int nfaces();
    void render(GLuint pid, MeshMode mode);

private:
    hh::PMeshRStream pmrs;
    hh::PMeshIter pmi;

    void init_buffer(MeshMode mode);
    void init_default_colors();
    void update_buffer_triangle(MeshMode mode);
    void update_buffer_strip(MeshMode mode);

    int numTex;
    int numVertices;
    int vertexDataSize;
    hh::Array<hh::A3dColor> defaultColors;

    VBufferInfo simpBuffer;
    VBufferInfo fullBuffer;
};

#endif
