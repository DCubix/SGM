#ifndef SGM_H
#define SGM_H

#include "SDL2/SDL.h"

#include "sgm_cpu.h"

typedef struct sgm_video_t {
	SDL_Texture *screen;
	SDL_Window *window;
	SDL_Renderer *renderer;
} sgmVideo;

extern sgmCPU* sgm_cpu;
extern sgmVideo* sgm_video;

void sgm_run(sgmByte* program, sgmWord n);
void sgm_run_file(const char* fileName);

void sgm_video_init();
void sgm_video_main();
void sgm_video_quit();

#endif // SGM_H
