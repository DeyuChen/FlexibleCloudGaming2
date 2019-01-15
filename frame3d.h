#ifndef FRAME3D_H
#define FRAME3D_H

#include "ffmpeg.h"
#include "opengl.h"

class Frame3D {
public:
    Frame3D(int width, int height, int format);
    ~Frame3D();

    glm::mat4 viewMatrix;    // projection * view
    AVFrame *color;
    float *depth;
};

#endif
