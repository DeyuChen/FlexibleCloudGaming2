#include "glWindow.h"
#include <iostream>
#include <fstream>

const float MIN_DISTANCE = 0.1f;
const float MAX_DISTANCE = 100.0f;
const int HOLE_FILL_ROUND = 0;

using namespace std;

inline int getIndex(int x, int y, int width){
    return y * width + x;
}

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

tuple<int, int> glWindow::render_diff(RenderTarget target){
    if (!diffProgram.id && !init_diff_program()){
        return {-1, -1};
    }
    auto [simpColorTex, simpDepthTex] = render_mesh(simp, texture_texture);
    auto [fullColorTex, fullDepthTex] = render_mesh(full, texture_texture);
    glEnable(GL_DEPTH_TEST);
    auto [outColor, outDepth] = render_textures(diffProgram.id, {fullColorTex, simpColorTex, fullDepthTex, simpDepthTex}, target);
    glDisable(GL_DEPTH_TEST);
    colorTexPool.put(simpColorTex);
    colorTexPool.put(fullColorTex);
    depthTexPool.put(simpDepthTex);
    depthTexPool.put(fullDepthTex);
    return {outColor, outDepth};
}

tuple<int, int> glWindow::render_simp(RenderTarget target){
    return render_mesh(simp, target);
}

int glWindow::render_sum(int simpTexid, unsigned char* diff, RenderTarget target){
    if (!sumProgram.id && !init_sum_program()){
        return -1;
    }
    int diffTexid = colorTexPool.get();
    glBindTexture(GL_TEXTURE_2D, diffTexid);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, diff);
    glGenerateMipmap(GL_TEXTURE_2D);
    auto [outTexid, dummy] = render_textures(sumProgram.id, {simpTexid, diffTexid}, target);
    assert(dummy == 0);
    colorTexPool.put(diffTexid);
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

void glWindow::read_depth(int texid, float* buf){
    if (texid){
        glBindTexture(GL_TEXTURE_2D, texid);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, buf);
    } else {
        glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, buf);
    }
}

glm::mat4 glWindow::get_viewMatrix(){
    glPushMatrix();
    glLoadIdentity();
    gluLookAt(0, 0, 0, 0, 0, -1, 0, 1, 0);

    glRotatef(elevation, 1, 0, 0);
    glRotatef(azimuth, 0, 1, 0);
    glTranslatef(viewX, viewY, viewZ);

    glm::mat4 view, projection;
    glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(view));
    glGetFloatv(GL_PROJECTION_MATRIX, glm::value_ptr(projection));
    glPopMatrix();

    return projection * view;
}

void glWindow::release_color_texture(int texid){
    colorTexPool.put(texid);
}

void glWindow::release_depth_texture(int texid){
    depthTexPool.put(texid);
}

tuple<int, int> glWindow::render_mesh(MeshMode mode, RenderTarget target){
    int outColorTex = 0, outDepthTex = 0;
    if (target == screen){
        // render to screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        // render color to texture
        outColorTex = colorTexPool.get();
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outColorTex, 0);
        if (target == texture_texture){
            // render depth to texture
            outDepthTex = depthTexPool.get();
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, outDepthTex, 0);
        }
    }

    glUseProgram(renderProgram.id);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set lookat point
    glPushMatrix();
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

    glEnable(GL_DEPTH_TEST);
    for (auto pmesh : pmeshes){
        pmesh->render(renderProgram.id, mode);
    }
    glDisable(GL_DEPTH_TEST);

    glPopMatrix();
    glUseProgram(0);
    if (target == texture_texture){
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return {outColorTex, outDepthTex};
}

tuple<int, int> glWindow::render_textures(int programid, vector<int> texids, RenderTarget target){
    int outColorTex = 0, outDepthTex = 0;
    if (target == screen){
        // render to screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        // render color to texture
        outColorTex = colorTexPool.get();
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outColorTex, 0);
        if (target == texture_texture){
            // render depth to texture
            outDepthTex = depthTexPool.get();
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, outDepthTex, 0);
        }
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(programid);

    assert(texids.size() >= 2);
    glUniform1i(glGetUniformLocation(programid, "Texture0"), 0);
    glUniform1i(glGetUniformLocation(programid, "Texture1"), 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texids[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texids[1]);

    if (texids.size() == 4){
        glUniform1i(glGetUniformLocation(programid, "Texture2"), 2);
        glUniform1i(glGetUniformLocation(programid, "Texture3"), 3);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, texids[2]);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, texids[3]);
    }

    glBindVertexArray(pixelBuffer.VAO[0]);

    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, NULL);

    glBindVertexArray(0);
    glUseProgram(0);
    if (target == texture_texture){
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return {outColorTex, outDepthTex};
}

void glWindow::render_warp(Frame3D* frame, unsigned char* warpedFrame, const glm::mat4 &targetView){
    if (!warpProgram.id && !init_warp_program())
        return;

    int outTexid = colorTexPool.get();
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTexid, 0);

    glUseProgram(warpProgram.id);
    glPushMatrix();
    glLoadIdentity();
    gluLookAt(0, 0, 0, 0, 0, -1, 0, 1, 0);

    glBindVertexArray(warpBuffer.VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, warpBuffer.VBO[1]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, nPixels * sizeof(float), frame->color->data[0]);
    glBufferSubData(GL_ARRAY_BUFFER, nPixels * sizeof(float), nPixels * sizeof(float), frame->depth);

    glm::mat4 mvp = targetView * glm::inverse(frame->viewMatrix);
    glUniformMatrix4fv(uLocMVP, 1, GL_FALSE, glm::value_ptr(mvp));

    float grey = 127.0 / 255.0;
    glClearColor(grey, grey, grey, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDrawElements(GL_POINTS, nPixels, GL_UNSIGNED_INT, NULL);
    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, outTexid);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, warpedFrame);

    // hole filling
    // TODO: it is too slow, needs to be done in GPU somehow
    int index;
    unsigned char *buf = new unsigned char[4 * nPixels];
    float *depth = new float[nPixels];
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth);

    for (int n = 0; n < HOLE_FILL_ROUND; n++){
        index = width + 1;
        memcpy(buf, warpedFrame, 4 * nPixels);
        for (int h = 1; h < height - 1; h++){
            for (int w = 1; w < width - 1; w++){
                if (depth[index] == 1){
                    int count = 0, rsum = 0, gsum = 0, bsum = 0;
                    for (int y = h - 1; y <= h + 1; y++){
                        for (int x = w - 1; x <= w + 1; x++){
                            int index2 = getIndex(x, y, width);
                            if (depth[index2] != 1){
                                count++;
                                rsum += (int)buf[4 * index2];
                                gsum += (int)buf[4 * index2 + 1];
                                bsum += (int)buf[4 * index2 + 2];
                            }
                        }
                    }
                    if (count > 0){
                        warpedFrame[4 * index] = rsum / count;
                        warpedFrame[4 * index + 1] = gsum / count;
                        warpedFrame[4 * index + 2] = bsum / count;
                    }
                }
                index++;
            }
            index += 2;
        }
    }
    delete[] buf;
    delete[] depth;

    glBindVertexArray(0);
    glPopMatrix();
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    release_color_texture(outTexid);
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

void glWindow::init_warp_buffer(){
    if (warpBuffer.VBO.size())
        return;

    warpBuffer.VAO.resize(1);
    warpBuffer.VBO.resize(2);
    warpBuffer.IBO.resize(1);
    glGenVertexArrays(1, warpBuffer.VAO.data());
    glGenBuffers(2, warpBuffer.VBO.data());
    glGenBuffers(1, warpBuffer.IBO.data());

    glBindVertexArray(warpBuffer.VAO[0]);

    // preset pixel 2D locations
    glBindBuffer(GL_ARRAY_BUFFER, warpBuffer.VBO[0]);
    vector<float> pixelVertices(2 * nPixels);
    int count = 0;
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            pixelVertices[count++] = (float)j * 2.0 / width - 1.0;
            pixelVertices[count++] = (float)i * 2.0 / height - 1.0;
        }
    }
    glBufferData(GL_ARRAY_BUFFER, 2 * nPixels * sizeof(float), pixelVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // colors
    glBindBuffer(GL_ARRAY_BUFFER, warpBuffer.VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, 2 * nPixels * sizeof(float), NULL, GL_STREAM_DRAW);
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    // depths
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)(nPixels * sizeof(float)));
    glEnableVertexAttribArray(2);

    // index
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, warpBuffer.IBO[0]);
    vector<GLuint> pixelIndices(nPixels);
    for (int i = 0; i < nPixels; i++){
        pixelIndices[i] = i;
    }
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * nPixels, pixelIndices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

bool glWindow::init_frame_buffer(){
    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

    depthTextures.resize(3);
    glGenTextures(depthTextures.size(), depthTextures.data());
    for (int i = 0; i < depthTextures.size(); i++){
        glBindTexture(GL_TEXTURE_2D, depthTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    depthTexPool.set_capacity(depthTextures.size());
    for (auto tex : depthTextures){
        depthTexPool.put(tex);
    }

    colorTextures.resize(3);
    glGenTextures(colorTextures.size(), colorTextures.data());
    for (int i = 0; i < colorTextures.size(); i++){
        glBindTexture(GL_TEXTURE_2D, colorTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    colorTexPool.set_capacity(colorTextures.size());
    for (auto tex : colorTextures){
        colorTexPool.put(tex);
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

    uLocMVP = glGetUniformLocation(warpProgram.id, "mvp");

    init_warp_buffer();

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

    simpDepth.resize(nPixels);
    fullDepth.resize(nPixels);

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
    //glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

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
                                   Pool<bool> &msgPool,
                                   Queue<bool> &msgToRender,
                                   Pool<Frame3D*> &framePool,
                                   Queue<Frame3D*> &frameToEncode)
    : glWindow(title, width, height),
      msgPool(msgPool), msgToRender(msgToRender),
      framePool(framePool), frameToEncode(frameToEncode)
{
    init_render_thread();
}

void glWindowServerMT::render_service(){
    create_window();

    while (true){
        msgToRender.get();
        auto [colorTex, depthTex] = render_diff(texture_texture);
        glm::mat4 viewMatrix = get_viewMatrix();
        msgPool.put(true);

        Frame3D *frame = framePool.get();
        read_pixels(colorTex, frame->color->data[0]);
        read_depth(depthTex, frame->depth);
        release_color_texture(colorTex);
        release_depth_texture(depthTex);
        frame->viewMatrix = viewMatrix;
        frameToEncode.put(frame);
    }
}

glWindowClientMT::glWindowClientMT(const char* title, int width, int height, PresentMode &pmode,
                                   Queue<proto::CommProto*> &msgToUpdate,
                                   Queue<proto::CommProto*> &msgToSend,
                                   Queue<Frame3D*> &frameDecoded,
                                   Queue<bool> &tokenBucket)
    : glWindow(title, width, height), pmode(pmode),
      msgToUpdate(msgToUpdate), msgToSend(msgToSend),
      frameDecoded(frameDecoded), tokenBucket(tokenBucket)
{
    warpedFrame.resize(4 * nPixels);
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

        // use token bucket for rate control
        // warp old diff frame if diff frame does not arrive on time
        auto [colorTex, depthTex] = render_simp(texture_renderbuffer);
        tokenBucket.get();
        Frame3D *frame = frameDecoded.peek_back();
        if (pmode == simplified){
            memset(frame->color->data[0], 127, 4 * width * height);
            render_sum(colorTex, frame->color->data[0], screen);
        } else {
            glm::mat4 targetView = get_viewMatrix();
            if (targetView != frame->viewMatrix){
                render_warp(frame, warpedFrame.data(), targetView);
                render_sum(colorTex, warpedFrame.data(), screen);
            } else {
                render_sum(colorTex, frame->color->data[0], screen);
            }
        }
        release_color_texture(colorTex);
        display();
    }
}
