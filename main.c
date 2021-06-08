
#define __STDC_CONSTANT_MACROS
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavutil/samplefmt.h"

#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_thread.h"


static char* program_name = "SimplePlayer";
static const char* input_filename = "F:/_git_home_/SimplePlayer/input/comic_cut.flv";

//---SDL defination
static int default_width = 640;
static int default_height = 480;
static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_RendererInfo renderer_info = { 0 };


static void do_exit()
{

    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
    exit(0);
}



int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER))
    {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

    window = SDL_CreateWindow(program_name, 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, default_width, default_height, 
        SDL_WINDOW_RESIZABLE);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    if (window)
    {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer)
        {
            printf("Failed to initialize a hardware accelerated renderer: %s\n", SDL_GetError());
            renderer = SDL_CreateRenderer(window, -1, 0);
        }
        if (renderer)
        {
            if (!SDL_GetRendererInfo(renderer, &renderer_info))
            {
                printf("Initialized %s renderer.\n", renderer_info.name);
            }
        }
    }
    if (!window || !renderer || !renderer_info.num_texture_formats) {
        printf("Failed to create window or renderer: %s", SDL_GetError());
        do_exit();
    }



    return 0;
}