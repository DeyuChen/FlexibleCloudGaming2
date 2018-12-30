#ifndef GLWINDOW_H
#define GLWINDOW_H

#include "opengl.h"
#include "PMeshRenderer.h"
#include "VBufferInfo.h"
#include <string>
#include <queue>

struct ShaderProgram {
    GLuint id = 0;
    GLuint vertexShader;
    GLuint fragmentShader;
};

class glWindow {
public:
    glWindow() : window(NULL), width(0), height(0),
                      viewX(0), viewY(0), viewZ(0), moveSpeed(0.1),
                      elevation(0), azimuth(0)
    {};

    ~glWindow(){};
    
    bool create_window(const char* title, int _width, int _height);
    void kill_window();

    void mouse_motion(int x, int y);
    void key_press(int key, int x, int y);

    int add_pmesh(const hh::PMesh& pm);
    void remove_pmesh(int id);

    void render(MeshMode mode);
    void render_diff(unsigned char* buf1, unsigned char* buf2);
    void render_sum(unsigned char* buf1, unsigned char* buf2);
    void display();
    void read_pixels(unsigned char* buf, GLenum format = GL_RGBA);
    void draw_pixels(const unsigned char* buf, GLenum format = GL_RGBA);

private:
    void init_pixel_buffer();
    bool init_render_program();
    bool init_warp_program();
    bool init_diff_program();
    bool init_sum_program();
    bool init_OpenGL();

    GLuint load_shader_from_file(std::string filename, GLenum shaderType);

    SDL_Window* window;
    SDL_GLContext context;

    int width, height;
    int nPixels;
    float viewX, viewY, viewZ;
    float moveSpeed;
    float elevation;
    float azimuth;

    ShaderProgram renderProgram;
    ShaderProgram warpProgram;
    ShaderProgram diffProgram;
    ShaderProgram sumProgram;

    VBufferInfo pixelBuffer;

    std::vector<PMeshRenderer*> pmeshes;

    std::priority_queue<int, std::vector<int>, std::greater<int>> available_meshID;
};

#endif
