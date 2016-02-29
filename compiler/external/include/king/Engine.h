// (C) king.com Ltd 2014

#pragma once

#include <glm/fwd.hpp>
#include <memory>

namespace King {
	class Updater;
	class Engine {
	public:

		enum Background {

			// Background
			CELL_NEUTRAL,
			CELL_PICKED,
			CELL_ALLOWED,
			CELL_FORBIDDEN,
			CELL_MAX
		};

		enum Diamond
		{
			// Diamonds
			DIAMOND_BLUE,
			DIAMOND_GREEN,
			DIAMOND_PURPLE,
			DIAMOND_RED,
			DIAMOND_YELLOW,
			DIAMOND_CYAN,
			DIAMOND_BLACK,
			DIAMOND_WHITE,
			DIAMOND_MAX
		};

		enum Text
		{
			// Text
			TEXT_CHAR,
			TEXT_MAX
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
		bool IsMouseButtonDown(uint8_t index) const;
		bool IsKeyDown(uint8_t key) const;
		
		void Start(Updater& updater);
		void Quit();

		void Write(const char* text, const glm::mat4& transform);
		void Write(const char* text, float x, float y, float rotation = 0.0f);

		void ChangeCell(int32_t index, Background new_template);
		void UpdateCell(int32_t index, glm::vec2 size, glm::vec4 color, float rotation);
		
		void GetDiamondData(int32_t index, glm::vec2& position, glm::vec2& size, glm::vec4& color, float& rotation) const;
		void UpdateDiamond(int32_t index, glm::vec2 position, glm::vec2 size, glm::vec4 color, float rotation);
		void MoveDiamond(int32_t index, glm::vec2 translate, glm::vec2 scale, float rotate);
		void ChangeDiamond(int32_t index, Diamond new_template);
		void AddDiamond(int32_t index, Diamond diamond_template);
		void RemoveDiamond(int32_t index);

		void AddFloatingDiamond(float x, float y, Diamond diamond_template);

		int32_t GetCellIndex(int32_t screen_x, int32_t screen_y) const;
		glm::vec2 GetCellPosition(int32_t index) const;
		bool IsCellFull(int32_t index) const;

		int32_t GetGridIndex(int32_t grid_x, int32_t gird_y) const;
		int32_t GetGriRow(int32_t index) const;
		int32_t GetGridColumn(int32_t index) const;
		int32_t GetGridSize() const;

		Diamond GetGridDiamond(int32_t index) const;

		int32_t GetGridWidth() const;
		int32_t GetGridHeight() const;

		int32_t GetWindowWidth() const;
		int32_t GetWindowHeight() const;

	private:

		float CalculateStringWidth(const char* text) const;

		struct Implementation;
		std::unique_ptr<Implementation> mPimpl;
	};
}