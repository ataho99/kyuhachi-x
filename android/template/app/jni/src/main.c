#include <SDL.h>

int main(int argc, char *argv[])
{
    SDL_Window *win;
    SDL_Renderer *ren;
    Uint32 start;
    SDL_Event ev;
    int run = 1;

    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    win = SDL_CreateWindow("t4", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_FULLSCREEN);
    ren = win ? SDL_CreateRenderer(win, -1, 0) : NULL;
    if (!ren) {
        SDL_Log("window/renderer failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    start = SDL_GetTicks();
    while (run && SDL_GetTicks() - start < 5000) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) run = 0;
        }
        SDL_SetRenderDrawColor(ren, 0, 96, 160, 255);
        SDL_RenderClear(ren);
        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
