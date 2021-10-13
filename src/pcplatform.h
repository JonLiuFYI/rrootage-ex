#pragma once

#include <SDL.h>


#ifdef __cplusplus
extern "C" {
#endif

#include "attractmanager.h"


	void init_imgui(SDL_Window* window);

	void imgui_newframe(SDL_Window* window);

	void imgui_input(SDL_Event* event);

	void initialize_platform(void);
	int refresh_input_device(void);

	void close_platform(void);

	bool load_game(PersistentState* state);
	bool save_game(PersistentState* state);

#ifdef __cplusplus
}
#endif