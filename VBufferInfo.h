#ifndef VBUFFERINFO_H
#define VBUFFERINFO_H

#include <vector>

struct VBufferInfo {
    std::vector<unsigned int> VBO;
    std::vector<unsigned int> VAO;
    std::vector<unsigned int> IBO;
    std::vector<int> indSizes;
    bool dirty = true;
};

#endif
