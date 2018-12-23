#include "PMeshRenderer.h"
#include "glWindow.h"
#include <iostream>
#include <fstream>

using namespace std;

const int width = 1080;
const int height = 720;

int main(int argc, char *argv[]){
    if (argc < 2){
        cout << "Not enough input arguments" << endl;
        return -1;
    }
    
    glWindow window;
    window.create_window("Hello World", width, height);

    ifstream ifs(argv[1], ifstream::in);

    hh::PMesh pm;
    pm.read(ifs);

    /*
    PMeshRenderer pmr(pm);
    
    do {
        cout << pmr.nfaces() << endl;
    } while (pmr.next());
    
    pmr.render();
    */

    int mid = window.add_pmesh(pm);

    SDL_Event e;
    SDL_SetRelativeMouseMode(SDL_TRUE);
    
    unsigned char* simpPixels = new unsigned char[3 * width * height];
    unsigned char* fullPixels = new unsigned char[3 * width * height];

    bool quit = false;
    while(!quit){
        int x = 0, y = 0;
        SDL_GetRelativeMouseState(&x, &y);
        window.mouse_motion(x, y);

        while (SDL_PollEvent(&e) != 0){
            if (e.type == SDL_QUIT){
                quit = true;
            } else if (e.type == SDL_KEYDOWN){
                window.key_press(e.key.keysym.sym, x, y);
            }
        }
        
        window.render(MeshMode::full);
        window.get_pixels(fullPixels);
        window.render(MeshMode::simp);
        window.get_pixels(simpPixels);
        window.render_diff(fullPixels, simpPixels);
        window.display();
    }

    return 0;
}
