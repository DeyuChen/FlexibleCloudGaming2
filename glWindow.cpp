#include "glWindow.h"
#include <iostream>
#include <fstream>

const float MIN_DISTANCE = 0.1f;
const float MAX_DISTANCE = 100.0f;

using namespace std;

bool glWindow::create_window(const char* title, int _width, int _height){
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
    float RelX = x / (float)width;
    float RelY = y / (float)height;

    azimuth += (RelX * 180);
    elevation += (RelY * 180);
}

void glWindow::key_press(Sint32 key, int x, int y){
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

        case SDLK_e:
            viewY += moveSpeed;
            break;

        case SDLK_q:
            viewY -= moveSpeed;
            break;

        case SDLK_PAGEUP:
            for (auto pmesh : pmeshes){
                pmesh->goto_vpercentage(min(100, pmesh->get_vpercentage() + 5));
            }
            break;

        case SDLK_PAGEDOWN:
            for (auto pmesh : pmeshes){
                pmesh->goto_vpercentage(max(0, pmesh->get_vpercentage() - 5));
            }
            break;

        case SDLK_UP:
            for (auto pmesh : pmeshes){
                pmesh->next();
            }
            break;

        case SDLK_DOWN:
            for (auto pmesh : pmeshes){
                pmesh->prev();
            }
            break;

        /*
        case SDLK_m:
            renderingMode = (renderingMode + 1) % 3;
            break;
        case SDLK_n:
            renderingMode = (renderingMode + 2) % 3;
            break;
        case SDLK_t:
            conf.texture = !conf.texture;
            break;
        */
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

void glWindow::render(MeshMode mode){
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
    glUniformMatrix4fv(glGetUniformLocation(renderProgram.id, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glGetFloatv(GL_PROJECTION_MATRIX, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(renderProgram.id, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glm::vec3 viewPos(viewX, viewY, viewZ);
    glm::vec3 lightPos(0.0, 0.0, 10.0);
    glm::vec3 lightColor(1.0, 1.0, 1.0);
    glUniform3fv(glGetUniformLocation(renderProgram.id, "viewPos"), 1, glm::value_ptr(viewPos));
    glUniform3fv(glGetUniformLocation(renderProgram.id, "lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(renderProgram.id, "lightColor"), 1, glm::value_ptr(lightColor));

    for (auto pmesh : pmeshes){
        pmesh->render(renderProgram.id, mode);
    }

    glUseProgram(0);
}

void glWindow::render_diff(unsigned char* buf1, unsigned char* buf2){
    vector<GLfloat> pixelColors(2 * width * height);
    int count = 0;
    for (int i = 0; i < width * height; i++){
        memcpy(&pixelColors[count++], buf1, 3 * sizeof(unsigned char));
        memcpy(&pixelColors[count++], buf2, 3 * sizeof(unsigned char));
        buf1 += 3;
        buf2 += 3;
    }

    glBindVertexArray(diffBuffer.VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, diffBuffer.VBO[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 2 * width * height * sizeof(GLfloat), sizeof(GLfloat) * pixelColors.size(), pixelColors.data());

    glUseProgram(diffProgram.id);
    glDrawElements(GL_POINTS, width * height, GL_UNSIGNED_INT, NULL);
    glUseProgram(0);

    glBindVertexArray(0);
}

void glWindow::display(){
    SDL_GL_SwapWindow(window);
}

void glWindow::get_pixels(unsigned char* buf){
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buf);
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

    glUniform1i(glGetUniformLocation(renderProgram.id, "Texture"), 0);

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
    glBindAttribLocation(diffProgram.id, 1, "in_Color_1");
    glBindAttribLocation(diffProgram.id, 2, "in_Color_2");

    glLinkProgram(diffProgram.id);

    GLint isLinked = GL_TRUE;
    glGetProgramiv(diffProgram.id, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE){
        cerr << "Failed to link shader program" << endl;
        return false;
    }

    // initialize vbuffer
    diffBuffer.VBO.resize(1);
    diffBuffer.VAO.resize(1);
    diffBuffer.IBO.resize(1);
    glGenBuffers(2, diffBuffer.VBO.data());
    glGenVertexArrays(1, diffBuffer.VAO.data());
    glGenBuffers(1, diffBuffer.IBO.data());

    vector<GLfloat> pixelVertices(4 * width * height);

    int count = 0;
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            pixelVertices[count++] = (GLfloat)j * 2.0 / width - 1.0;
            pixelVertices[count++] = (GLfloat)i * 2.0 / height - 1.0;
        }
    }

    vector<GLuint> pixelIndices(width * height);
    for (int i = 0; i < pixelIndices.size(); i++){
        pixelIndices[i] = i;
    }

    glBindVertexArray(diffBuffer.VAO[0]);

    glBindBuffer(GL_ARRAY_BUFFER, diffBuffer.VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * pixelVertices.size(), pixelVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_FALSE, 2 * sizeof(GLfloat), (void*)(2 * width * height * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_UNSIGNED_BYTE, GL_FALSE, 2 * sizeof(GLfloat), (void*)((2 * width * height + 1) * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, diffBuffer.IBO[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * pixelIndices.size(), pixelIndices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    return true;
}

bool glWindow::init_OpenGL(){
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    if (!init_render_program()){
        return false;
    }

    if (!init_diff_program()){
        return false;
    }
    
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
