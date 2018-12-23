#ifndef GLWINDOW_H
#define GLWINDOW_H

#include "glHeader.h"
#include "PMeshRenderer.h"
#include "common.h"
#include <string>
#include <queue>

struct ShaderProgram {
    GLuint id;
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
    void key_press(Sint32 key, int x, int y);

    int add_pmesh(const hh::PMesh& pm);
    void remove_pmesh(int id);

    void render(MeshMode mode);
    void render_diff(unsigned char* buf1, unsigned char* buf2);
    void display();
    void get_pixels(unsigned char* buf);

private:
    bool init_render_program();
    bool init_warp_program();
    bool init_diff_program();
    bool init_OpenGL();
    
    GLuint load_shader_from_file(std::string filename, GLenum shaderType);

    SDL_Window* window;
    SDL_GLContext context;

    int width, height;
    float viewX, viewY, viewZ;
    float moveSpeed;
    float elevation;
    float azimuth;

    ShaderProgram renderProgram;
    ShaderProgram warpProgram;
    ShaderProgram diffProgram;

    VBufferInfo diffBuffer;

    std::vector<PMeshRenderer*> pmeshes;

    std::priority_queue<int, std::vector<int>, std::greater<int>> available_meshID;
};

#endif
