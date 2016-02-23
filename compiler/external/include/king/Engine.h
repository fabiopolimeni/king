// (C) king.com Ltd 2014

#pragma once

#include <glm/fwd.hpp>
#include <memory>

namespace King {
	class Updater;
	class Engine {
	public:

		enum Sprite {
			SPRITE_CELL,
			SPRITE_BLUE,
			SPRITE_GREEN,
			SPRITE_PURPLE,
			SPRITE_RED,
			SPRITE_YELLOW,
			SPRITE_MAX
		};

		enum Image
		{
			IMAGE_BACKGROUND,
			IMAGE_DIAMONDS,
			IMAGE_TEXT,
			IMAGE_MAX
		};

		const static size_t GRID_WIDTH = 8;
		const static size_t GRID_HEIGHT = 8;

		Engine(const char* assetsDirectory);
		~Engine();

		float GetLastFrameSeconds() const;
		float GetMouseX() const;
		float GetMouseY() const;
		bool GetMouseButtonDown() const;
		
		void Start(Updater& updater);
		void Quit();

		void Render(Sprite texture, const glm::mat4& transform);
		void Render(Sprite texture, float x, float y, float rotation = 0.0f);

		void Write(const char* text, const glm::mat4& transform);
		void Write(const char* text, float x, float y, float rotation = 0.0f);

		int GetWidth() const;
		int GetHeight() const;

	private:

		float CalculateStringWidth(const char* text) const;

		struct Implementation;
		std::unique_ptr<Implementation> mPimpl;
	};
}