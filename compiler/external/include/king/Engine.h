// (C) king.com Ltd 2014

#pragma once

#include <glm/fwd.hpp>
#include <memory>

namespace King {
	class Updater;
	class Engine {
	public:

		enum Sprite {
			
			// Background
			SPRITE_CELL_EMPTY,
			SPRITE_CELL_FULL,
			SPRITE_CELL_ALLOWED,
			SPRITE_CELL_FORBIDDEN,

			// Text
			SPRITE_CHAR,
			
			// Diamonds
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

		void ChangeCell(int32_t index, Sprite new_template);
		void UpdateCell(int32_t index, glm::vec2 size, glm::vec4 color, float rotation);
		
		void UpdateDiamond(int32_t index, glm::vec2 position, glm::vec2 size, glm::vec4 color, float rotation);
		void MoveDiamond(int32_t index, glm::vec2 translate, glm::vec2 scale, float rotate);
		void ChangeDiamond(int32_t index, Sprite new_template);
		void AddDiamond(int32_t index, Sprite diamond_template);
		void RemoveDiamond(int32_t index);

		void AddFloatingDiamond(float x, float y, Sprite diamond_template);

		int32_t GetCellIndex(int32_t screen_x, int32_t screen_y) const;
		glm::vec2 GetCellPosition(int32_t index) const;
		bool IsCellFull(int32_t index) const;

		int32_t GetWindowWidth() const;
		int32_t GetWindowHeight() const;

	private:

		float CalculateStringWidth(const char* text) const;

		struct Implementation;
		std::unique_ptr<Implementation> mPimpl;
	};
}