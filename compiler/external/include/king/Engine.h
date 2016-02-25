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
			SPRITE_CHAR,
			SPRITE_BLUE,
			SPRITE_GREEN,
			SPRITE_PURPLE,
			SPRITE_RED,
			SPRITE_YELLOW,
			SPRITE_CYAN,
			SPRITE_BLACK,
			SPRITE_WHITE,
			SPRITE_MAX
		};

		enum Image {
			IMAGE_BACKGROUND,
			IMAGE_DIAMONDS,
			IMAGE_TEXT,
			IMAGE_MAX
		};

		Engine(const char* assets_directory);
		~Engine();

		float GetLastFrameSeconds() const;
		float GetMouseX() const;
		float GetMouseY() const;
		bool IsMouseButtonDown() const;
		
		void Start(Updater& updater);
		void Quit();

		void Render(Sprite texture, const glm::mat4& transform);
		void Render(Sprite texture, float x, float y, float rotation = 0.0f);

		void Write(const char* text, const glm::mat4& transform);
		void Write(const char* text, float x, float y, float rotation = 0.0f);

		int32_t GetGridIndex(int32_t screen_x, int32_t screen_y) const;

		bool ChangeGridCell(int32_t index, Sprite new_template);
		bool UpdateGridCell(int32_t index, glm::vec2 scale, glm::vec4 color, float rotation);

		int32_t GetWindowWidth() const;
		int32_t GetWindowHeight() const;

	private:

		float CalculateStringWidth(const char* text) const;

		struct Implementation;
		std::unique_ptr<Implementation> mPimpl;
	};
}