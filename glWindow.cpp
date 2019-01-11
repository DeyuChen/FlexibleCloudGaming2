#include "glWindow.h"
#include <iostream>
#include <fstream>

const float MIN_DISTANCE = 0.1f;
const float MAX_DISTANCE = 100.0f;

using namespace std;

glWindow::glWindow(const char* title, int width, int height) :
                   width(width), height(height),
                   nPixels(width*height), window(NULL),
                   viewX(0), viewY(0), viewZ(0), moveSpeed(0.1),
                   elevation(0), azimuth(0),
                   pressedKeys({{SDLK_w, false}, {SDLK_d, false}, {SDLK_s, false},
                                {SDLK_a, false}, {SDLK_e, false}, {SDLK_q, false},
                                {SDLK_PAGEUP, false}, {SDLK_PAGEDOWN, false},
                                {SDLK_UP, false}, {SDLK_DOWN, false}})
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << endl;
        return;
    }

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (window == NULL){
        cerr << "Window could not be created! SDL Error: " << SDL_GetError() << endl;
        return;
    }
};

bool glWindow::create_window(){
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    context = SDL_GL_CreateContext(window);
    if (context == NULL){
        cerr << "OpenGL context could not be created! SDL Error: " << SDL_GetError() << endl;
        return false;
    }

    if (SDL_GL_SetSwapInterval(1) < 0){
        cerr << "Warning: Unable to set VSync! SDL Error: " << SDL_GetError() << endl;
    }

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, MIN_DISTANCE, MAX_DISTANCE);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (glewInit() != GLEW_OK || !init_OpenGL()){
        kill_window();
        return false;
    }

    return true;
}

void glWindow::kill_window(){
    if (window != NULL){
        SDL_DestroyWindow(window);
        window = NULL;
    }

    SDL_Quit();
}

void glWindow::mouse_motion(int x, int y){
    RelX = x;
    RelY = y;
}

void glWindow::key_event(bool down, int key, int x, int y){
    if (pressedKeys.count(key)){
        pressedKeys[key] = down;
    }
}

void glWindow::update_state(){
    azimuth += (RelX * 180 / width);
    elevation += (RelY * 180 / height);
    if (pressedKeys[SDLK_w]){
        viewZ += moveSpeed * cos(azimuth * 3.14159265 / 180.0);
        viewX -= moveSpeed * sin(azimuth * 3.14159265 / 180.0);
    }
    if (pressedKeys[SDLK_d]){
        viewZ -= moveSpeed * sin(azimuth * 3.14159265 / 180.0);
        viewX -= moveSpeed * cos(azimuth * 3.14159265 / 180.0);
    }
    if (pressedKeys[SDLK_s]){
        viewZ -= moveSpeed * cos(azimuth * 3.14159265 / 180.0);
        viewX += moveSpeed * sin(azimuth * 3.14159265 / 180.0);
    }
    if (pressedKeys[SDLK_a]){
        viewZ += moveSpeed * sin(azimuth * 3.14159265 / 180.0);
        viewX += moveSpeed * cos(azimuth * 3.14159265 / 180.0);
    }
    if (pressedKeys[SDLK_e]){
        viewY += moveSpeed;
    }
    if (pressedKeys[SDLK_q]){
        viewY -= moveSpeed;
    }
    if (pressedKeys[SDLK_PAGEUP]){
        for (auto pmesh : pmeshes){
            pmesh->goto_vpercentage(min(100, pmesh->get_vpercentage() + 5));
        }
    }
    if (pressedKeys[SDLK_PAGEDOWN]){
        for (auto pmesh : pmeshes){
            pmesh->goto_vpercentage(max(0, pmesh->get_vpercentage() - 5));
        }
    }
    if (pressedKeys[SDLK_UP]){
        for (auto pmesh : pmeshes){
            pmesh->next();
        }
    }
    if (pressedKeys[SDLK_DOWN]){
        for (auto pmesh : pmeshes){
            pmesh->prev();
        }
    }
}

int glWindow::add_pmesh(const hh::PMesh& pm){
    int id;
    PMeshRenderer *pmr = new PMeshRenderer(pm);

    if (available_meshID.empty()){
        id = pmeshes.size();
        pmeshes.push_back(pmr);
    } else {
        id = available_meshID.top();
        available_meshID.pop();
        pmeshes[id] = pmr;
    }

    return id;
}

void glWindow::remove_pmesh(int id){
    if (id >= 0 && id < pmeshes.size() && pmeshes[id]){
        delete pmeshes[id];
        pmeshes[id] = NULL;
        available_meshID.push(id);
        // clear empty tail?
    }
}

int glWindow::render_diff(RenderTarget target){
    if (!diffProgram.id && !init_diff_program()){
        return -1;
    }
    int simpTexid = render_mesh(simp, texture);
    int fullTexid = render_mesh(full, texture);
    int outTexid = render_textures(diffProgram.id, fullTexid, simpTexid, target);
    texturePool.put(simpTexid);
    texturePool.put(fullTexid);
    return outTexid;
}

int glWindow::render_simp(RenderTarget target){
    return render_mesh(simp, target);
}

int glWindow::render_sum(int simpTexid, unsigned char* diff, RenderTarget target){
    if (!sumProgram.id && !init_sum_program()){
        return -1;
    }
    int diffTexid = texturePool.get();
    glBindTexture(GL_TEXTURE_2D, diffTexid);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, diff);
    glGenerateMipmap(GL_TEXTURE_2D);
    int outTexid = render_textures(sumProgram.id, simpTexid, diffTexid, target);
    texturePool.put(diffTexid);
    return outTexid;
}

void glWindow::display(){
    SDL_GL_SwapWindow(window);
}

void glWindow::read_pixels(int texid, unsigned char* buf, GLenum format){
    if (texid){
        glBindTexture(GL_TEXTURE_2D, texid);
        glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, buf);
    } else {
        glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, buf);
    }
}

void glWindow::release_texture(int texid){
    texturePool.put(texid);
}

int glWindow::render_mesh(MeshMode mode, RenderTarget target){
    int outTexid;
    if (target == screen){
        // render to screen
        outTexid = 0;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else if (target == texture){
        // render to texture
        outTexid = texturePool.get();
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTexid, 0);
    }

    glUseProgram(renderProgram.id);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set lookat point
    glLoadIdentity();
    gluLookAt(0, 0, 0, 0, 0, -1, 0, 1, 0);

    glRotatef(elevation, 1, 0, 0);
    glRotatef(azimuth, 0, 1, 0);
    glTranslatef(viewX, viewY, viewZ);

    glm::mat4 view, projection;
    glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(view));
    glUniformMatrix4fv(uLocView, 1, GL_FALSE, glm::value_ptr(view));
    glGetFloatv(GL_PROJECTION_MATRIX, glm::value_ptr(projection));
    glUniformMatrix4fv(uLocProjection, 1, GL_FALSE, glm::value_ptr(projection));
    
    glm::vec3 viewPos(viewX, viewY, viewZ);
    glm::vec3 lightPos(0.0, 0.0, 10.0);
    glm::vec3 lightColor(1.0, 1.0, 1.0);
    glUniform3fv(uLocViewPos, 1, glm::value_ptr(viewPos));
    glUniform3fv(uLocLightPos, 1, glm::value_ptr(lightPos));
    glUniform3fv(uLocLightColor, 1, glm::value_ptr(lightColor));

    glUniform1i(glGetUniformLocation(renderProgram.id, "Texture"), 0);

    for (auto pmesh : pmeshes){
        pmesh->render(renderProgram.id, mode);
    }

    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return outTexid;
}

int glWindow::render_textures(int programid, int texid_1, int texid_2, RenderTarget target){
    int outTexid;
    if (target == screen){
        // render to screen
        outTexid = 0;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else if (target == texture){
        // render to texture
        outTexid = texturePool.get();
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTexid, 0);
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(programid);
    glUniform1i(glGetUniformLocation(programid, "Texture0"), 0);
    glUniform1i(glGetUniformLocation(programid, "Texture1"), 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texid_1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texid_2);

    glBindVertexArray(pixelBuffer.VAO[0]);

    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, NULL);

    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return outTexid;
}

void glWindow::init_pixel_buffer(){
    if (pixelBuffer.VBO.size())
        return;

    float pixelVertices[] = {
        // positions   // texCoords
        -1.0f, -1.0f,  0.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    unsigned int pixelIndices[] = {0, 1, 2, 3};

    pixelBuffer.VBO.resize(1);
    pixelBuffer.VAO.resize(1);
    pixelBuffer.IBO.resize(1);
    glGenBuffers(2, pixelBuffer.VBO.data());
    glGenVertexArrays(1, pixelBuffer.VAO.data());
    glGenBuffers(1, pixelBuffer.IBO.data());

    glBindVertexArray(pixelBuffer.VAO[0]);

    glBindBuffer(GL_ARRAY_BUFFER, pixelBuffer.VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pixelVertices), &pixelVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pixelBuffer.IBO[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(pixelIndices), &pixelIndices, GL_STATIC_DRAW);

    glBindVertexArray(0);
}

bool glWindow::init_frame_buffer(){
    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

    renderedTextures.resize(3);
    glGenTextures(renderedTextures.size(), renderedTextures.data());
    for (int i = 0; i < renderedTextures.size(); i++){
        glBindTexture(GL_TEXTURE_2D, renderedTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    texturePool.set_capacity(renderedTextures.size());
    for (auto tex : renderedTextures){
        texturePool.put(tex);
    }

    drawBuffers = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, drawBuffers.data());

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        cerr << "init_frame_buffer() failed!" << endl;
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

bool glWindow::init_render_program(){
    // load 3D shader program
    renderProgram.vertexShader = load_shader_from_file("3DShader.vert", GL_VERTEX_SHADER);
    renderProgram.fragmentShader = load_shader_from_file("3DShader.frag", GL_FRAGMENT_SHADER);

    renderProgram.id = glCreateProgram();

    glAttachShader(renderProgram.id, renderProgram.vertexShader);
    glAttachShader(renderProgram.id, renderProgram.fragmentShader);

    glBindAttribLocation(renderProgram.id, 0, "in_Position");
    glBindAttribLocation(renderProgram.id, 1, "in_Normal");
    glBindAttribLocation(renderProgram.id, 2, "in_TexCoord");
    glBindAttribLocation(renderProgram.id, 3, "in_Color");

    glLinkProgram(renderProgram.id);

    GLint isLinked = GL_TRUE;
    glGetProgramiv(renderProgram.id, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE){
        cerr << "Failed to link shader program" << endl;
        return false;
    }

    uLocView = glGetUniformLocation(renderProgram.id, "view");
    uLocProjection = glGetUniformLocation(renderProgram.id, "projection");
    uLocViewPos = glGetUniformLocation(renderProgram.id, "viewPos");
    uLocLightPos = glGetUniformLocation(renderProgram.id, "lightPos");
    uLocLightColor = glGetUniformLocation(renderProgram.id, "lightColor");

    return true;
}

bool glWindow::init_warp_program(){
    // load warp shader program
    warpProgram.vertexShader = load_shader_from_file("warpShader.vert", GL_VERTEX_SHADER);
    warpProgram.fragmentShader = load_shader_from_file("warpShader.frag", GL_FRAGMENT_SHADER);

    warpProgram.id = glCreateProgram();

    glAttachShader(warpProgram.id, warpProgram.vertexShader);
    glAttachShader(warpProgram.id, warpProgram.fragmentShader);

    glBindAttribLocation(warpProgram.id, 0, "in_Position");
    glBindAttribLocation(warpProgram.id, 1, "in_Color");
    glBindAttribLocation(warpProgram.id, 2, "in_Depth");

    glLinkProgram(warpProgram.id);

    GLint isLinked = GL_TRUE;
    glGetProgramiv(warpProgram.id, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE){
        cerr << "Failed to link shader program" << endl;
        return false;
    }

    return true;
}

bool glWindow::init_diff_program(){
    diffProgram.vertexShader = load_shader_from_file("diffShader.vert", GL_VERTEX_SHADER);
    diffProgram.fragmentShader = load_shader_from_file("diffShader.frag", GL_FRAGMENT_SHADER);

    diffProgram.id = glCreateProgram();

    glAttachShader(diffProgram.id, diffProgram.vertexShader);
    glAttachShader(diffProgram.id, diffProgram.fragmentShader);

    glBindAttribLocation(diffProgram.id, 0, "in_Position");
    glBindAttribLocation(diffProgram.id, 1, "in_TexCoord");

    glLinkProgram(diffProgram.id);

    GLint isLinked = GL_TRUE;
    glGetProgramiv(diffProgram.id, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE){
        cerr << "Failed to link shader program" << endl;
        return false;
    }

    return true;
}

bool glWindow::init_sum_program(){
    sumProgram.vertexShader = load_shader_from_file("sumShader.vert", GL_VERTEX_SHADER);
    sumProgram.fragmentShader = load_shader_from_file("sumShader.frag", GL_FRAGMENT_SHADER);

    sumProgram.id = glCreateProgram();

    glAttachShader(sumProgram.id, sumProgram.vertexShader);
    glAttachShader(sumProgram.id, sumProgram.fragmentShader);

    glBindAttribLocation(sumProgram.id, 0, "in_Position");
    glBindAttribLocation(sumProgram.id, 1, "in_TexCoord");

    glLinkProgram(sumProgram.id);

    GLint isLinked = GL_TRUE;
    glGetProgramiv(sumProgram.id, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE){
        cerr << "Failed to link shader program" << endl;
        return false;
    }

    return true;
}

bool glWindow::init_OpenGL(){
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    if (!init_render_program())
        return false;

    init_pixel_buffer();

    if (!init_frame_buffer())
        return false;
    
    return true;
}

GLuint glWindow::load_shader_from_file(string filename, GLenum shaderType){
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
            cerr << "Shader failed to compile: " << filename << endl;

            int maxLength;
            char *log;
            glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);
            log = (char *)malloc(maxLength);
            glGetShaderInfoLog(shaderID, maxLength, &maxLength, log);
            cerr << log << endl;
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

glWindowServerMT::glWindowServerMT(const char* title, int width, int height,
                                   Pool<proto::CommProto*> &msgPool,
                                   Queue<proto::CommProto*> &msgToRender,
                                   Pool<AVFrame*> &framePool,
                                   Queue<AVFrame*> &frameToEncode)
    : glWindow(title, width, height),
      msgPool(msgPool), msgToRender(msgToRender),
      framePool(framePool), frameToEncode(frameToEncode)
{
    init_render_thread();
}

void glWindowServerMT::render_service(){
    create_window();

    while (true){
        proto::CommProto *msg = msgToRender.get();
        update_state();
        set_nvertices(msg->pmesh(0).id(), msg->pmesh(0).nvertices());
        msg->Clear();
        msgPool.put(msg);

        int texid = render_diff(texture);
        AVFrame *frame = framePool.get();
        read_pixels(texid, frame->data[0]);
        release_texture(texid);
        frameToEncode.put(frame);
    }
}

glWindowClientMT::glWindowClientMT(const char* title, int width, int height, PresentMode &pmode,
                                   Queue<proto::CommProto*> &msgToUpdate,
                                   Queue<proto::CommProto*> &msgToSend,
                                   Pool<AVFrame*> &framePool,
                                   Queue<AVFrame*> &frameDecoded)
    : glWindow(title, width, height), pmode(pmode),
      msgToUpdate(msgToUpdate), msgToSend(msgToSend),
      framePool(framePool), frameDecoded(frameDecoded)
{
    init_render_thread();
}

void glWindowClientMT::render_service(){
    create_window();

    while (true){
        proto::CommProto *msg = msgToUpdate.get();
        update_state();
        for (int i = 0; i < pmeshes.size(); i++){
            proto::PMeshProto *pmeshProto = msg->add_pmesh();
            pmeshProto->set_id(0);
            pmeshProto->set_nvertices(get_nvertices(0));
        }
        msgToSend.put(msg);

        int texid = render_simp(texture);
        AVFrame *frame = frameDecoded.get();
        if (pmode == simplified)
            memset(frame->data[0], 127, 4 * width * height);
        render_sum(texid, frame->data[0], screen);
        release_texture(texid);
        framePool.put(frame);
        display();
    }
}
