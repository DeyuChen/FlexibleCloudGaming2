#ifndef GLWINDOW_H
#define GLWINDOW_H

#include "ffmpeg.h"
#include "opengl.h"
#include "PMeshRenderer.h"
#include "VBufferInfo.h"
#include "pool.h"
#include "frame3d.h"
#include "CommProto.pb.h"
#include <string>
#include <queue>
#include <unordered_map>
#include <tuple>
#include <pthread.h>

enum PresentMode {
    simplified,
    patched
};

enum RenderTarget {
    screen,
    texture_renderbuffer,
    texture_texture
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

    std::tuple<int, int> render_diff(RenderTarget target);
    std::tuple<int, int> render_simp(RenderTarget target);
    int render_sum(int simpTexid, unsigned char* diff, RenderTarget target);
    void display();
    void read_pixels(int texid, unsigned char* buf, GLenum format = GL_RGBA);
    void read_depth(int texid, float* buf);
    glm::mat4 get_viewMatrix();
    void release_color_texture(int texid);
    void release_depth_texture(int texid);

protected:
    std::tuple<int, int> render_mesh(MeshMode mode, RenderTarget target);
    std::tuple<int, int> render_textures(int programid, std::vector<int> texids, RenderTarget target);
    void render_warp(Frame3D* frame, unsigned char* warpedFrame, const glm::mat4 &targetView);

    void init_pixel_buffer();
    void init_warp_buffer();
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
    GLint uLocMVP;

    VBufferInfo pixelBuffer;
    VBufferInfo warpBuffer;

    GLuint frameBuffer;
    GLuint depthBuffer;
    std::vector<GLuint> colorTextures;
    std::vector<GLuint> depthTextures;
    std::vector<GLenum> drawBuffers;

    Pool<GLuint> colorTexPool;
    Pool<GLuint> depthTexPool;

    std::vector<PMeshRenderer*> pmeshes;

    std::priority_queue<int, std::vector<int>, std::greater<int>> available_meshID;
    
    std::vector<float> simpDepth;
    std::vector<float> fullDepth;
};

class glWindowServerMT : public glWindow {
public:
    glWindowServerMT(const char* title, int width, int height,
                     Pool<proto::CommProto*> &msgPool,
                     Queue<proto::CommProto*> &msgToRender,
                     Pool<Frame3D*> &framePool,
                     Queue<Frame3D*> &frameToEncode);

private:
    void init_render_thread(){ thread = SDL_CreateThread(render_service_entry, "render", this); }
    static int render_service_entry(void* This){
        ((glWindowServerMT*)This)->render_service();
        return 0;
    }
    void render_service();

    Pool<proto::CommProto*> &msgPool;
    Queue<proto::CommProto*> &msgToRender;
    Pool<Frame3D*> &framePool;
    Queue<Frame3D*> &frameToEncode;

    SDL_Thread *thread;
};

class glWindowClientMT : public glWindow {
public:
    glWindowClientMT(const char* title, int width, int height, PresentMode &pmode,
                     Queue<proto::CommProto*> &msgToUpdate,
                     Queue<proto::CommProto*> &msgToSend,
                     Queue<Frame3D*> &frameDecoded,
                     Queue<bool> &tokenBucket);

private:
    void init_render_thread(){ thread = SDL_CreateThread(render_service_entry, "render", this); }
    static int render_service_entry(void* This){
        ((glWindowClientMT*)This)->render_service();
        return 0;
    }
    void render_service();

    PresentMode &pmode;

    Queue<proto::CommProto*> &msgToUpdate;
    Queue<proto::CommProto*> &msgToSend;
    Queue<Frame3D*> &frameDecoded;
    Queue<bool> &tokenBucket;

    std::vector<unsigned char> warpedFrame;

    SDL_Thread *thread;
};

#endif
