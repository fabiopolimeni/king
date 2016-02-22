#include "Engine.h"

#include <stdexcept>
#include <algorithm>
#include <vector>

#define GLM_FORCE_RADIANS 
#include <glm/gtc/matrix_transform.hpp>
#include <glew/glew.h>
#include <glm/glm.hpp>
#include <sdl/Sdl.h>

#include "Font.h"
#include "GlContext.h"
#include "Sdl.h"
#include "SdlWindow.h"
#include "SdlSurface.h"
#include "Updater.h"

#include "SpriteBatch.hpp"
#include "SpriteTexture.hpp"

namespace King {
	static const int WindowWidth = 800;
	static const int WindowHeight = 600;
	static const float MaxFrameTicks = 300.0f;
	static const float TextScale = 0.5f;

	struct Engine::EngineImplementation {
		
		Sdl mSdl;
		SdlWindow mSdlWindow;
		GlContext mGlContext;

		std::unique_ptr<SpriteTexture> mSpriteTextures[Engine::SPRITE_MAX];
		std::unique_ptr<SpriteBatch> mSpriteBatches[Engine::SPRITE_MAX];

		float mElapsedTicks;
		float mLastFrameSeconds;
		Updater* mUpdater;
		bool mQuit;

		float mMouseX;
		float mMouseY;
		bool mMouseButtonDown;
		
		EngineImplementation()
			: mSdl(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE)
			, mSdlWindow(WindowWidth, WindowHeight)
			, mGlContext(mSdlWindow)
			, mLastFrameSeconds(1.0f / 60.0f)
			, mMouseX(WindowWidth * 0.5f)
			, mMouseY(WindowHeight * 0.5f)
			, mMouseButtonDown(false)
			, mQuit(false)
			, mUpdater(nullptr)
			, mElapsedTicks(static_cast<float>(SDL_GetTicks()))
		{
		}

		void Start();
		void ParseEvents();
	};

	Engine::Engine(const char* assetsDirectory)
		: mPimpl(new EngineImplementation) {

		// VSync enabled
		SDL_GL_SetSwapInterval(1);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		std::string assets_dir(assetsDirectory);
		std::string texture_files[Engine::SPRITE_MAX] = {
			assets_dir + "/textures/Cell.dds",
			assets_dir + "/textures/Diamonds.dds",
			assets_dir + "/textures/berlin_sans_demi_72_0.dds"
		};

		std::string vert_shader_file = assets_dir + "/shaders/sprite.vert";
		std::string frag_shader_file = assets_dir + "/shaders/sprite.frag";

		glm::mat4 projection = glm::ortho(0.0f, float(WindowWidth), float(WindowHeight), 0.0f, -1.0f, 1.0f);

		for (size_t si = 0; si < Engine::SPRITE_MAX; ++si)
		{
			auto* sprite_textrue = new SpriteTexture();
			sprite_textrue->create(texture_files[si].c_str());

			auto* sprite_batch = new SpriteBatch();
			sprite_batch->init(projection, sprite_textrue->getTexId(),
				vert_shader_file.c_str(), frag_shader_file.c_str(),
				SpriteBatch::MAX_TEMPLATES, SpriteBatch::MAX_INSTANCES
			);

			// Add texture and sprite batch to the managed pointers
			mPimpl->mSpriteTextures[si].reset(sprite_textrue);
			mPimpl->mSpriteBatches[si].reset(sprite_batch);
		}
	}

	Engine::~Engine() {
	}

	float Engine::GetLastFrameSeconds() const {
		return mPimpl->mLastFrameSeconds;
	}

	float Engine::GetMouseX() const {
		return mPimpl->mMouseX;
	}

	float Engine::GetMouseY() const {
		return mPimpl->mMouseY;
	}

	bool Engine::GetMouseButtonDown() const {
		return mPimpl->mMouseButtonDown;
	}
	
	void Engine::Quit() {
		mPimpl->mQuit = true;
	}

	void Engine::Start(Updater& updater) {
		mPimpl->mUpdater = &updater;
		mPimpl->mSdlWindow.Show();
		mPimpl->Start();
	}

	int Engine::GetTextureHeight(Texture texture) const {
		//return mPimpl->mSdlSurfaceContainer[texture]->Height();
		return 0;
	}

	int Engine::GetTextureWidth(Texture texture) const {
		//return mPimpl->mSdlSurfaceContainer[texture]->Width();
		return 0;
	}

	void Engine::Render(Engine::Texture texture, const glm::mat4& transform) {
		//glLoadMatrixf(reinterpret_cast<const float*>(&transform));

		//SdlSurface& surface = *mPimpl->mSdlSurfaceContainer[texture];

		//surface.Bind();

		//glBegin(GL_QUADS);
		//glTexCoord2i(0, 1); glVertex2i(0, surface.Height());
		//glTexCoord2i(1, 1); glVertex2i(surface.Width(), surface.Height());
		//glTexCoord2i(1, 0); glVertex2i(surface.Width(), 0);
		//glTexCoord2i(0, 0); glVertex2i(0, 0);
		//glEnd();
	}

	void Engine::Render(Texture texture, float x, float y, float rotation) {
		glm::mat4 transformation;
		transformation = glm::translate(transformation, glm::vec3(x, y, 0.0f));
		if (rotation) {
			transformation = glm::rotate(transformation, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
		}

		Render(texture, transformation);
	}

	Glyph& FindGlyph(char c) {
		auto found = std::lower_bound(std::begin(Font), std::end(Font), c);
		if (found == std::end(Font) || c < *found) {
			found = std::lower_bound(std::begin(Font), std::end(Font), static_cast<int>('_'));
		}
		return *found;
	}

	float Engine::CalculateStringWidth(const char* text) const {
		int advance = 0;
		for (; *text; ++text) {
			Glyph& g = FindGlyph(*text);
			advance += g.advance;
		}
		return advance*TextScale;
	}

	void Engine::Write(const char* text, const glm::mat4& transform) {
		//glLoadMatrixf(reinterpret_cast<const float*>(&transform));
		int advance = 0;
		for (; *text;++text) {
			Glyph& g = FindGlyph(*text);

		/*	float fontTexWidth  = static_cast<float>(mPimpl->mFontSdlSurface->Width());
			float fontTexHeight = static_cast<float>(mPimpl->mFontSdlSurface->Height());

			float uvLeft = static_cast<float>(g.x) / fontTexWidth;
			float uvRight = static_cast<float>(g.x + g.width) / fontTexWidth;
			float uvBottom = static_cast<float>(g.y) / fontTexHeight;
			float uvTop = static_cast<float>(g.y + g.height) / fontTexHeight;

			float worldLeft = static_cast<float>(g.xoffset + advance);
			float worldRight = static_cast<float>(g.xoffset + g.width + advance);
			float worldBottom = static_cast<float>(g.yoffset);
			float worldTop = static_cast<float>(g.yoffset + g.height);

			mPimpl->mFontSdlSurface->Bind();*/

			//glBegin(GL_QUADS);
			//glTexCoord2f(uvLeft / 2.0f, uvTop / 2.0f); glVertex2f(worldLeft * TextScale, worldTop * TextScale);
			//glTexCoord2f(uvRight / 2.0f, uvTop / 2.0f); glVertex2f(worldRight * TextScale, worldTop * TextScale);
			//glTexCoord2f(uvRight / 2.0f, uvBottom / 2.0f); glVertex2f(worldRight * TextScale, worldBottom * TextScale);
			//glTexCoord2f(uvLeft / 2.0f, uvBottom / 2.0f); glVertex2f(worldLeft * TextScale, worldBottom * TextScale);
			//glEnd();
			advance += g.advance;
		}
	}

	void Engine::Write(const char* text, float x, float y, float rotation) {
		glm::mat4 transformation;
		transformation = glm::translate(transformation, glm::vec3(x, y, 0.0f));
		if (rotation) {
			transformation = glm::rotate(transformation, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
			transformation = glm::translate(transformation, glm::vec3(-CalculateStringWidth(text)/2.0f, -20.0f, 0.0f));
		}

		Write(text, transformation);
	}

	int Engine::GetWidth() const {
		return WindowWidth;
	}

	int Engine::GetHeight() const {
		return WindowHeight;
	}

	void Engine::EngineImplementation::Start() {
		while (!mQuit) {
			SDL_GL_SwapWindow(mSdlWindow);

			static float depth_value = 1.0f;
			static glm::vec4 view_color(.95f);
			glClearBufferfv(GL_DEPTH, 0, &depth_value);
			glClearBufferfv(GL_COLOR, 0, &view_color[0]);

			glm::vec4 viewport = glm::vec4(0.0f, 0.0f, WindowWidth, WindowHeight);
			glViewportIndexedfv(0, &viewport[0]);

			ParseEvents();
			
			float currentTicks = static_cast<float>(SDL_GetTicks());
			float lastFrameTicks = currentTicks - mElapsedTicks;
			mElapsedTicks = currentTicks;

			lastFrameTicks = std::min(lastFrameTicks, MaxFrameTicks);
			mLastFrameSeconds = lastFrameTicks * 0.001f;

			if (mUpdater) {
				mUpdater->Update();
			}
		}
	}

	void Engine::EngineImplementation::ParseEvents() {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
			case SDL_KEYDOWN:
				mQuit = true;
				break;
			case SDL_MOUSEBUTTONDOWN:
				mMouseButtonDown = true;
				break;
			case SDL_MOUSEBUTTONUP:
				mMouseButtonDown = false;
				break;
			case SDL_MOUSEMOTION:
				mMouseX = static_cast<float>(event.motion.x);
				mMouseY = static_cast<float>(event.motion.y);
				break;
			default:
				break;
			}
		}
	}
}
