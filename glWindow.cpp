#include "glWindow.h"
#include <iostream>
#include <fstream>

using namespace std;

bool glWindow::createWindow(const char* title, int _width, int _height){
    width = _width;
    height = _height;

    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (window == NULL){
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
        return false;
    }

    context = SDL_GL_CreateContext(window);
    if (context == NULL){
        printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
        return false;
    }

    if (SDL_GL_SetSwapInterval(1) < 0){
        printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
    }

    if (glewInit() != GLEW_OK || !initOpenGL()){
        killWindow();
        return false;
    }

    return true;
}

void glWindow::killWindow(){
    if (window != NULL){
        SDL_DestroyWindow(window);
        window = NULL;
    }

    SDL_Quit();
}

/*
void glWindow::mouseMotion(int x, int y, bool leftButton, bool rightButton){
    float RelX = x / (float)width;
    float RelY = y / (float)height;
    
    azimuth += (RelX * 180);
    elevation += (RelY * 180);
}

void glWindow::keyPress(Sint32 key, int x, int y){
    switch(key){
        case SDLK_w:
            viewZ += moveSpeed * cos(azimuth * 3.14159265 / 180.0);
            viewX -= moveSpeed * sin(azimuth * 3.14159265 / 180.0);
            break;
            
        case SDLK_d:
            viewZ -= moveSpeed * sin(azimuth * 3.14159265 / 180.0);
            viewX -= moveSpeed * cos(azimuth * 3.14159265 / 180.0);
            break;
            
        case SDLK_s:
            viewZ -= moveSpeed * cos(azimuth * 3.14159265 / 180.0);
            viewX += moveSpeed * sin(azimuth * 3.14159265 / 180.0);
            break;
            
        case SDLK_a:
            viewZ += moveSpeed * sin(azimuth * 3.14159265 / 180.0);
            viewX += moveSpeed * cos(azimuth * 3.14159265 / 180.0);
            break;
            
        case SDLK_PAGEUP:
            for (int i = 0; i < pmesh_list.size(); i++)
                changeMesh(pmesh_list[i].mesh, 5, true);
            break;
            
        case SDLK_PAGEDOWN:
            for (int i = 0; i < pmesh_list.size(); i++)
                changeMesh(pmesh_list[i].mesh, -5, true);
            break;
            
        case SDLK_UP:
            for (int i = 0; i < pmesh_list.size(); i++)
                changeMesh(pmesh_list[i].mesh, 1, false);
            break;
            
        case SDLK_DOWN:
            for (int i = 0; i < pmesh_list.size(); i++)
                changeMesh(pmesh_list[i].mesh, -1, false);
            break;
            
        case SDLK_m:
            renderingMode = (renderingMode + 1) % 3;
            break;

        case SDLK_n:
            renderingMode = (renderingMode + 2) % 3;
            break;
            
        case SDLK_t:
            conf.texture = !conf.texture;
            break;
    }
}
*/

int glWindow::initOpenGL(){
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    // load 3D shader program
    renderProgram.vertexShader = loadShaderFromFile("3DShader.vert", GL_VERTEX_SHADER);
    renderProgram.fragmentShader = loadShaderFromFile("3DShader.frag", GL_FRAGMENT_SHADER);

    renderProgram.id = glCreateProgram();

    glAttachShader(renderProgram.id, renderProgram.vertexShader);
    glAttachShader(renderProgram.id, renderProgram.fragmentShader);

    glBindAttribLocation(renderProgram.id, 0, "in_Position");
    glBindAttribLocation(renderProgram.id, 1, "in_Normal");
    glBindAttribLocation(renderProgram.id, 2, "in_Color");
    glBindAttribLocation(renderProgram.id, 3, "in_TexCoord");

    glLinkProgram(renderProgram.id);

    GLint isLinked = GL_TRUE;
    glGetProgramiv(renderProgram.id, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE){
        cerr << "Failed to link shader program" << endl;
        return false;
    }

    glUniform1i(glGetUniformLocation(renderProgram.id, "Texture"), 0);
    
    // load warp shader program
    warpProgram.vertexShader = loadShaderFromFile("warpShader.vert", GL_VERTEX_SHADER);
    warpProgram.fragmentShader = loadShaderFromFile("warpShader.frag", GL_FRAGMENT_SHADER);

    warpProgram.id = glCreateProgram();

    glAttachShader(warpProgram.id, warpProgram.vertexShader);
    glAttachShader(warpProgram.id, warpProgram.fragmentShader);

    glBindAttribLocation(warpProgram.id, 0, "in_Position");
    glBindAttribLocation(warpProgram.id, 1, "in_Color");
    glBindAttribLocation(warpProgram.id, 2, "in_Depth");

    glLinkProgram(warpProgram.id);

    glGetProgramiv(warpProgram.id, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE){
        cerr << "Failed to link shader program" << endl;
        return false;
    }

    return true;
}

GLuint glWindow::loadShaderFromFile(string filename, GLenum shaderType){
	GLuint shaderID = 0;
	string content;
	ifstream ifs(filename.c_str());

    if (ifs){
        content.assign((istreambuf_iterator<char>(ifs) ), istreambuf_iterator<char>());

        shaderID = glCreateShader(shaderType);
        const char *pcontent = content.c_str();
        glShaderSource(shaderID, 1, (const GLchar**)&pcontent, NULL);
        glCompileShader(shaderID);

        GLint shaderCompiled = GL_FALSE;
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &shaderCompiled);
        if (shaderCompiled != GL_TRUE){
            cerr << "Shader failed to compile" << endl;

            int maxLength;
            char *log;
            glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);
            log = (char *)malloc(maxLength);
            glGetShaderInfoLog(shaderID, maxLength, &maxLength, log);
            cout << log << endl;
            free(log);

            glDeleteShader(shaderID);
            shaderID = 0;
        }
	}
    else {
        cout << "Open shader file failed" << endl;
    }

	return shaderID;
}
