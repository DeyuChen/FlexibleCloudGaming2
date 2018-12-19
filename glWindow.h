#ifndef GLWINDOW_H
#define GLWINDOW_H

#include "glHeader.h"
#include "PMeshRenderer.h"
#include <string>

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
    
    bool createWindow(const char* title, int _width, int _height);
    void killWindow();

    void mouseMotion(int x, int y);
    void keyPress(Sint32 key, int x, int y);

    int addPMesh(const hh::PMesh& pm);


    void render();

private:
    int initOpenGL();
    GLuint loadShaderFromFile(std::string filename, GLenum shaderType);

    SDL_Window* window;
    SDL_GLContext context;

    int width, height;
    float viewX, viewY, viewZ;
    float moveSpeed;
    float elevation;
    float azimuth;

    ShaderProgram renderProgram;
    ShaderProgram warpProgram;

    std::vector<PMeshRenderer*> pmeshes;
};

#endif
