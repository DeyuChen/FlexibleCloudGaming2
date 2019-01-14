#include "frame3d.h"
#include <iostream>

using namespace std;

Frame3D::Frame3D(int width, int height, int format){
    color = av_frame_alloc();
    if (!color) {
        cerr << "Could not allocate video frame" << endl;
        return;
    }
    color->format = format;
    color->width  = width;
    color->height = height;
    if (av_frame_get_buffer(color, 32) < 0){
        cerr << "Could not allocate the video frame data" << endl;
    }

    depth = new float[width * height];
}

Frame3D::~Frame3D(){
    av_frame_free(&color);
    delete[] depth;
}
