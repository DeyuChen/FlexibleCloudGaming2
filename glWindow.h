#ifndef GLWINDOW_H
#define GLWINDOW_H

#include "opengl.h"
#include "PMeshRenderer.h"
#include "VBufferInfo.h"
#include "CommProto.pb.h"
#include "pool.h"
#include <string>
#include <queue>
#include <unordered_map>
#include <pthread.h>

enum RenderTarget {
    screen,
    texture
};

struct ShaderProgram {
    GLuint id = 0;
    GLuint vertexShader;
    GLuint fragmentShader;
};

class glWindow {
public:
    glWindow(const char* title, int width, int height);

    bool create_window();
    void kill_window();

    void mouse_motion(int x, int y);
    void key_event(bool down, int key, int x, int y);
    void update_state();

    int add_pmesh(const hh::PMesh& pm);
    void remove_pmesh(int id);
    int get_nvertices(int id){ return pmeshes[id]->get_nvertices(); }
    bool set_nvertices(int id, int nv){ return pmeshes[id]->set_nvertices(nv); }

    int render_diff(RenderTarget target);
    int render_simp(RenderTarget target);
    int render_sum(int simpTexid, unsigned char* diff, RenderTarget target);
    void display();
    void read_pixels(int texid, unsigned char* buf, GLenum format = GL_RGBA);
    void release_texture(int texid);

protected:
    int render_mesh(MeshMode mode, RenderTarget target);
    int render_textures(int programid, int texid_1, int texid_2, RenderTarget target);

    void init_pixel_buffer();
    bool init_frame_buffer();
    bool init_render_program();
    bool init_warp_program();
    bool init_diff_program();
    bool init_sum_program();
    bool init_OpenGL();

    GLuint load_shader_from_file(std::string filename, GLenum shaderType);

    SDL_Window *window;
    SDL_GLContext context;

    int width, height;
    int nPixels;
    float viewX, viewY, viewZ;
    float RelX, RelY;
    float moveSpeed;
    float elevation;
    float azimuth;

    std::unordered_map<int, bool> pressedKeys;

    ShaderProgram renderProgram;
    ShaderProgram warpProgram;
    ShaderProgram diffProgram;
    ShaderProgram sumProgram;

    GLint uLocView;
    GLint uLocProjection;
    GLint uLocViewPos;
    GLint uLocLightPos;
    GLint uLocLightColor;

    VBufferInfo pixelBuffer;

    GLuint frameBuffer;
    GLuint depthBuffer;
    std::vector<GLuint> renderedTextures;
    std::vector<GLenum> drawBuffers;

    Pool<GLuint> texturePool;

    std::vector<PMeshRenderer*> pmeshes;

    std::priority_queue<int, std::vector<int>, std::greater<int>> available_meshID;

    SDL_Thread *thread;
};

class glWindowServer : public glWindow {
public:
    glWindowServer(const char* title, int width, int height,
                   Pool<proto::CommProto*> &msgPool,
                   Queue<proto::CommProto*> &msgToRender,
                   Pool<AVFrame*> &framePool,
                   Queue<AVFrame*> &frameToEncode) :
        glWindow(title, width, height),
        msgPool(msgPool), msgToRender(msgToRender),
        framePool(framePool), frameToEncode(frameToEncode)
    {}

    void init_render_thread(){ thread = SDL_CreateThread(render_service_entry, "render", this); }

private:
    static int render_service_entry(void* This){
        ((glWindowServer*)This)->render_service();
        return 0;
    }
    void render_service();

    Pool<proto::CommProto*> &msgPool;
    Queue<proto::CommProto*> &msgToRender;
    Pool<AVFrame*> &framePool;
    Queue<AVFrame*> &frameToEncode;
};

#endif
