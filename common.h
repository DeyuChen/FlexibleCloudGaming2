#ifndef COMMON_H
#define COMMON_H

struct VBufferInfo {
    std::vector<GLuint> VBO;
    std::vector<GLuint> VAO;
    std::vector<GLuint> IBO;
    std::vector<int> indSizes;
    bool dirty;
};

#endif
