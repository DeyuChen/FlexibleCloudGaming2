#ifndef PMESHRENDERER_H
#define PMESHRENDERER_H

#include "glHeader.h"
#include "PMesh.h"
#include <string>
#include <vector>
#include <fstream>

class PMeshRenderer {
public:
    PMeshRenderer(const hh::PMesh& pm);
    ~PMeshRenderer();
    bool next();
    bool prev();
    bool goto_vpercentage(int percentage);
    int get_vpercentage();
    int nfaces();
    void render(GLuint pid);

private:
    hh::PMeshRStream pmrs;
    hh::PMeshIter pmi;

    void init_buffer();
    void update_buffer_triangle();
    void update_buffer_strip();

    bool buffer_dirty;

    int numTex;
    int numVertices;
    GLuint VBO;
    std::vector<GLuint> VAO;
    std::vector<GLuint> IBO;
    std::vector<int> indSizes;
    hh::Array<hh::A3dColor> defaultColors;
};

#endif
