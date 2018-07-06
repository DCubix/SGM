#include "sgm.h"

#include <stdlib.h>
#include <string.h>

#include "sgm_compiler.h"

sgmCPU* sgm_cpu = NULL;
sgmVideo* sgm_video = NULL;

#define WIDTH SGM_VIDEO_WIDTH * 2
#define HEIGHT SGM_VIDEO_HEIGHT * 2

/*
26 28 44
87 41 86
177 65 86
238 123 88
255 208 121
160 240 114
56 184 110
39 110 123
41 54 111
64 91 208
79 164 247
134 236 248
244 244 244
147 182 193
85 113 133
50 64 86
*/

typedef struct sgm_col {
	sgmByte r, g, b;
} sgmColor;

static const sgmColor SGM_PALETTE[] = {
	{ 26, 28, 44 },			// BLACK
	{ 87, 41, 86 },			// PURPLE
	{ 177, 65, 86 },		// RED
	{ 238, 123, 88 },		// ORANGE
	{ 255, 208, 121 },		// YELLOW
	{ 160, 240, 114 },		// LIME
	{ 56, 184, 110 },		// GREEN
	{ 39, 110, 123 },		// DARK GREEN
	{ 41, 54, 111 },		// DARK BLUE
	{ 64, 91, 208 },		// BLUE
	{ 79, 164, 247 },		// DARK CYAN
	{ 134, 236, 248 },		// CYAN
	{ 244, 244, 244 },		// WHITE
	{ 147, 182, 193 },		// LIGHT GRAY
	{ 85, 113, 133 },		// GRAY
	{ 50, 64, 86 }			// DARK GRAY
};

void sgm_video_init() {
	sgm_video = (sgmVideo*) malloc(sizeof(sgmVideo));

	SDL_Init(SDL_INIT_VIDEO);

	sgm_video->window = SDL_CreateWindow(
							"SGM",
							SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							WIDTH, HEIGHT,
							SDL_WINDOW_SHOWN
	);

	if (sgm_video->window == NULL) {
		printf("Could not create Window. %s\n", SDL_GetError());
		return;
	}

	sgm_video->renderer = SDL_CreateRenderer(sgm_video->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (sgm_video->renderer == NULL) {
		printf("Could not create Renderer. %s\n", SDL_GetError());
		return;
	}

	sgm_video->screen = SDL_CreateTexture(
							sgm_video->renderer,
							SDL_PIXELFORMAT_RGB24,
							SDL_TEXTUREACCESS_STREAMING,
							SGM_VIDEO_WIDTH, SGM_VIDEO_HEIGHT
	);

	sgm_cpu = sgm_cpu_new();
}

static void sgm_video_flip() {
	sgmByte* pixels;
	int pitch;
	SDL_LockTexture(sgm_video->screen, NULL, &pixels, &pitch);
	for (int y = 0; y < SGM_VIDEO_HEIGHT; y++) {
		for (int x = 0; x < SGM_VIDEO_WIDTH; x++) {
			int index = x + y * SGM_VIDEO_WIDTH;
			sgmByte col = sgm_cpu->ram[SGM_LOC_VIDEO + index];
			int pidx = index * 3;
			pixels[pidx + 0] = SGM_PALETTE[col].r;
			pixels[pidx + 1] = SGM_PALETTE[col].g;
			pixels[pidx + 2] = SGM_PALETTE[col].b;
		}
	}
	SDL_UnlockTexture(sgm_video->screen);

	// Render
	SDL_RenderClear(sgm_video->renderer);

	SDL_Rect dst = { 0, 0, WIDTH, HEIGHT };
	SDL_RenderCopy(sgm_video->renderer, sgm_video->screen, NULL, &dst);

	SDL_RenderPresent(sgm_video->renderer);
}

void sgm_video_quit() {
	free(sgm_cpu);
	SDL_DestroyTexture(sgm_video->screen);
	SDL_DestroyRenderer(sgm_video->renderer);
	SDL_DestroyWindow(sgm_video->window);
	free(sgm_video);
	SDL_Quit();
}

void sgm_video_main() {
	bool running = true;
	SDL_Event evt;
	while (running) {
		if (sgm_cpu->stop) {
			running = false;
		}

		if (sgm_cpu->flip) {
			sgm_video_flip();
			sgm_cpu->flip = false;
		}

		while (SDL_PollEvent(&evt)) {
			if (evt.type == SDL_QUIT) running = false;
		}

		sgm_cpu_tick(sgm_cpu);
//		SDL_Delay(1);
	}
}

void sgm_run(sgmByte* program, sgmWord n) {
	sgm_video_init();
	sgm_cpu_load(sgm_cpu, program, n);
	sgm_video_main();
	sgm_video_quit();
}

void sgm_run_file(const char* fileName) {
	sgm_video_init();
	sgm_compiler_load(sgm_cpu, fileName);
	sgm_video_main();
	sgm_video_quit();
}
