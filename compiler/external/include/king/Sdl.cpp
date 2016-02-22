#include "Sdl.h"

#include <sdl/SDL.h>
#include <stdexcept>

namespace King {
	Sdl::Sdl(int flags) {
		if (SDL_Init(flags) != 0) {
			throw std::runtime_error("Failed to init SDL");
		}

		// Try to make this engine a little bit up to date :)
		// OpenGL 4.3 is reasonable supported on all major
		// desktop PC GPU vendors, ATI, nVIDIA and Intel.
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	}
	Sdl::~Sdl() {
		SDL_Quit();
	}
}