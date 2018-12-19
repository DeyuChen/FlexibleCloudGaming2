#include "PMeshRenderer.h"
#include "glWindow.h"
#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char *argv[]){
    if (argc < 2){
        cout << "Not enough input arguments" << endl;
        return -1;
    }
    
    glWindow window;
    window.createWindow("Hello World", 1080, 720);

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

    int mid = window.addPMesh(pm);

    SDL_Event e;
    SDL_SetRelativeMouseMode(SDL_TRUE);

    bool quit = false;
    while(!quit){
        int x = 0, y = 0;
        SDL_GetRelativeMouseState(&x, &y);
        window.mouseMotion(x, y);

        while (SDL_PollEvent(&e) != 0){
            if (e.type == SDL_QUIT){
                quit = true;
            } else if (e.type == SDL_KEYDOWN){
                window.keyPress(e.key.keysym.sym, x, y);
            }
        }
        window.render();
    }

    return 0;
}
