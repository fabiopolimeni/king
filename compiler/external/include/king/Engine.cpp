#include "Engine.h"

#include <stdexcept>
#include <algorithm>
#include <vector>
#include <array>

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
	
	const static size_t GRID_DIM = 8;
	const static size_t MAX_GLYPHS = 256;

	struct Engine::Implementation {
		
		Sdl mSdl;
		SdlWindow mSdlWindow;
		GlContext mGlContext;

		std::unique_ptr<SpriteTexture> mTextures[Engine::IMAGE_MAX];
		std::unique_ptr<SpriteBatch> mBatches[Engine::IMAGE_MAX];

		typedef std::vector<std::unique_ptr<SpriteBatch::Template>> TemplateSet;
		std::array<TemplateSet, Engine::IMAGE_MAX> mTemplates;

		std::shared_ptr<SpriteBatch::Instance> mBackground[GRID_DIM * GRID_DIM];
		
		std::shared_ptr<SpriteBatch::Instance> mDiamonds[GRID_DIM * GRID_DIM];
		Engine::Diamond mDiamondsTemplateMap[GRID_DIM * GRID_DIM];

		std::shared_ptr<SpriteBatch::Instance> mText[MAX_GLYPHS];

		std::vector<std::shared_ptr<SpriteBatch::Instance>> mPendingDiamonds;

		float mElapsedTicks;
		float mLastFrameSeconds;
		Updater* mUpdater;
		bool mQuit;

		float mMouseX;
		float mMouseY;
		bool mMouseButtonDown;
		uint8_t mMouseButtonsMask;

		bool mKeyDown[256];
		
		Implementation()
			: mSdl(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE)
			, mSdlWindow(WindowWidth, WindowHeight)
			, mGlContext(mSdlWindow)
			, mLastFrameSeconds(1.0f / 60.0f)
			, mMouseX(WindowWidth * 0.5f)
			, mMouseY(WindowHeight * 0.5f)
			, mMouseButtonDown(false)
			, mMouseButtonsMask(0x0)
			, mQuit(false)
			, mUpdater(nullptr)
			, mElapsedTicks(static_cast<float>(SDL_GetTicks()))
		{
			memset(mKeyDown, 0, sizeof(mKeyDown));
		}

		~Implementation()
		{
			mUpdater = nullptr;
		}

		int32_t GetGridDims() const;
		int32_t GetGridStartX() const;
		int32_t GetGridStartY() const;
		int32_t GetGridWidth() const;
		int32_t GetGridHeight() const;

		float GetCellSize() const;
		int32_t GetNumOfGridCells() const;

		std::unique_ptr<SpriteBatch>& GetBackgroundBatch();
		std::unique_ptr<SpriteBatch>& GetDiamondBatch();
		std::unique_ptr<SpriteBatch>& GetTextBatch();

		TemplateSet& GetBackgroundTemplates();
		TemplateSet& GetDiamondTemplates();
		TemplateSet& GetTextTemplates();

		void Start();
		void ParseEvents();

		void InitSpriteBatches(const std::string & assets_dir);
		void InitSpriteTemplates();
		void InitSpriteIntances();
	};

	//////////////////////////////////////////////////////////////////////////
	// ENGINE
	//////////////////////////////////////////////////////////////////////////

	Engine::Engine(const char* assets_directory)
		: mPimpl(new Implementation) {

		// VSync enabled
		SDL_GL_SetSwapInterval(1);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		std::string assets_dir(assets_directory);
		mPimpl->InitSpriteBatches(assets_dir);
		mPimpl->InitSpriteTemplates();
		mPimpl->InitSpriteIntances();
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

	bool Engine::IsMouseButtonDown(uint8_t index) const {
		
		return (
			mPimpl->mMouseButtonDown &&
			mPimpl->mMouseButtonsMask & (1 << index));
	}

	bool Engine::IsKeyDown(uint8_t key) const
	{
		return mPimpl->mKeyDown[key];
	}
	
	void Engine::Quit() {
		mPimpl->mQuit = true;
	}

	void Engine::Start(Updater& updater) {
		mPimpl->mUpdater = &updater;
		mPimpl->mSdlWindow.Show();
		mPimpl->Start();
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
			transformation = glm::translate(transformation,
				glm::vec3(-CalculateStringWidth(text)/2.0f, -20.0f, 0.0f));
		}

		Write(text, transformation);
	}

	void Engine::ChangeCell(int32_t index, Background new_template)
	{
		assert(index < mPimpl->GetNumOfGridCells());
		auto& instance = mPimpl->mBackground[index];
		auto& grid_batch = mPimpl->GetBackgroundBatch();
		const auto& sprite_template = mPimpl->GetBackgroundTemplates()[new_template];
		grid_batch->swapInstanceTemplate(instance, *sprite_template);
	}

	void Engine::UpdateCell(int32_t index, glm::vec2 size, glm::vec4 color, float rotation)
	{
		assert(index < mPimpl->GetNumOfGridCells());
		auto& instance = mPimpl->mBackground[index];
		auto& grid_batch = mPimpl->GetBackgroundBatch();
		grid_batch->updateInstance(instance, grid_batch->getInstancePosition(instance), size, color, rotation);
	}

	void Engine::GetDiamondData(int32_t index, glm::vec2 & position, glm::vec2 & size, glm::vec4 & color, float & rotation) const
	{
		assert(index < mPimpl->GetNumOfGridCells());
		auto& diamonds_batch = mPimpl->GetDiamondBatch();
		auto& instance = mPimpl->mDiamonds[index];

		// Moving is relative to current instance state.
		position = diamonds_batch->getInstancePosition(instance);
		size = diamonds_batch->getInstanceSize(instance);
		color = diamonds_batch->getInstanceColor(instance);
		rotation = diamonds_batch->getInstanceRotation(instance);
	}

	void Engine::UpdateDiamond(int32_t index, glm::vec2 position, glm::vec2 size, glm::vec4 color, float rotation)
	{
		assert(index < mPimpl->GetNumOfGridCells());
		auto& instance = mPimpl->mDiamonds[index];
		auto& diamonds_batch = mPimpl->GetDiamondBatch();
		diamonds_batch->updateInstance(instance, position, size, color, rotation);
	}

	void Engine::MoveDiamond(int32_t index, glm::vec2 translate, glm::vec2 scale, float rotate)
	{
		assert(index < mPimpl->GetNumOfGridCells());
		auto& diamonds_batch = mPimpl->GetDiamondBatch();
		auto& instance = mPimpl->mDiamonds[index];

		// Moving is relative to current instance state.
		glm::vec2 position = diamonds_batch->getInstancePosition(instance) + translate;
		glm::vec2 size = diamonds_batch->getInstanceSize(instance) * scale;
		glm::vec4 color = diamonds_batch->getInstanceColor(instance);
		float rotation = diamonds_batch->getInstanceRotation(instance) + rotate;

		diamonds_batch->updateInstance(instance, position, size, color, rotation);
	}

	void Engine::ChangeDiamond(int32_t index, Diamond new_template)
	{
		assert(index < mPimpl->GetNumOfGridCells());
		auto& instance = mPimpl->mDiamonds[index];
		auto& diamonds_batch = mPimpl->GetDiamondBatch();
		const auto& sprite_template = mPimpl->GetDiamondTemplates()[new_template];
		diamonds_batch->swapInstanceTemplate(instance, *sprite_template);
		mPimpl->mDiamondsTemplateMap[index] = new_template;
	}

	void Engine::AddDiamond(int32_t index, Diamond diamond_template)
	{
		assert(index < mPimpl->GetNumOfGridCells());

		// Remove any previous diamond sprite instance
		RemoveDiamond(index);

		auto& instance = mPimpl->mDiamonds[index];
		auto& diamonds_batch = mPimpl->GetDiamondBatch();
		mPimpl->mDiamonds[index] = diamonds_batch->addInstance(*mPimpl->GetDiamondTemplates()[diamond_template]);
		diamonds_batch->updateInstance(instance, GetCellPosition(index), glm::vec2(mPimpl->GetCellSize()));
		mPimpl->mDiamondsTemplateMap[index] = diamond_template;
	}

	void Engine::RemoveDiamond(int32_t index)
	{
		if (IsCellFull(index))
		{
			auto& diamonds_batch = mPimpl->GetDiamondBatch();
			diamonds_batch->removeInstance(mPimpl->mDiamonds[index]);
			mPimpl->mDiamonds[index] = nullptr;
		}
	}

	void Engine::AddFloatingDiamond(float x, float y, Diamond diamond_template)
	{
		auto& diamonds_batch = mPimpl->GetDiamondBatch();
		
		mPimpl->mPendingDiamonds.push_back(
			diamonds_batch->addInstance(*mPimpl->GetDiamondTemplates()[diamond_template]));
		
		diamonds_batch->updateInstance(mPimpl->mPendingDiamonds.back(),
			glm::vec2(x, y), glm::vec2(float(mPimpl->GetCellSize())));
	}

	int32_t Engine::GetCellIndex(int32_t screen_x, int32_t screen_y) const
	{
		screen_y = GetWindowHeight() - screen_y;

		const int32_t end_x = mPimpl->GetGridStartX() + mPimpl->GetGridWidth();
		const int32_t end_y = mPimpl->GetGridStartY() + mPimpl->GetGridHeight();

		if (screen_x >= mPimpl->GetGridStartX() && screen_x < end_x
			&& screen_y >= mPimpl->GetGridStartY() && screen_y < end_y) {

			int32_t grid_x = screen_x - mPimpl->GetGridStartX();
			int32_t grid_y = screen_y - mPimpl->GetGridStartY();

			int32_t cell_x = grid_x / int32_t(mPimpl->GetCellSize());
			int32_t cell_y = grid_y / int32_t(mPimpl->GetCellSize());

			if (cell_y < mPimpl->GetGridDims() && cell_x < mPimpl->GetGridDims()) {
				return cell_y * mPimpl->GetGridDims() + cell_x;
			}
		}

		return -1;
	}

	glm::vec2 Engine::GetCellPosition(int32_t index) const
	{
		assert(index < mPimpl->GetNumOfGridCells());
		int32_t row_id = index / mPimpl->GetGridDims();
		int32_t col_id = index - (row_id * mPimpl->GetGridDims());

		return glm::vec2(
			col_id * mPimpl->GetCellSize() + mPimpl->GetGridStartX(),
			row_id * mPimpl->GetCellSize() + mPimpl->GetGridStartY());
	}

	bool Engine::IsCellFull(int32_t index) const
	{
		assert(index < mPimpl->GetNumOfGridCells());
		return mPimpl->mDiamonds[index].get() != nullptr;
	}

	int32_t Engine::GetGridIndex(int32_t grid_x, int32_t gird_y) const
	{
		return gird_y * mPimpl->GetGridDims() + grid_x;
	}

	int32_t Engine::GetGriRow(int32_t index) const
	{
		assert(index < mPimpl->GetNumOfGridCells());
		return index / mPimpl->GetGridDims();
	}

	int32_t Engine::GetGridColumn(int32_t index) const
	{
		assert(index < mPimpl->GetNumOfGridCells());
		return index - (GetGriRow(index) * mPimpl->GetGridDims());
	}

	int32_t Engine::GetGridSize() const
	{
		return mPimpl->GetNumOfGridCells();
	}

	Engine::Diamond Engine::GetGridDiamond(int32_t index) const
	{
		assert(index < mPimpl->GetNumOfGridCells());
		return IsCellFull(index)
			? mPimpl->mDiamondsTemplateMap[index]
			: Diamond::DIAMOND_MAX;
	}

	int32_t Engine::GetGridWidth() const
	{
		return mPimpl->GetGridDims();
	}

	int32_t Engine::GetGridHeight() const
	{
		return mPimpl->GetGridDims();
	}

	int32_t Engine::GetWindowWidth() const {
		return WindowWidth;
	}

	int32_t Engine::GetWindowHeight() const {
		return WindowHeight;
	}

	//////////////////////////////////////////////////////////////////////////
	// ENGINE IMPLEMENTATION
	//////////////////////////////////////////////////////////////////////////

	int32_t Engine::Implementation::GetGridDims() const {
		return GRID_DIM;
	}

	int32_t Engine::Implementation::GetGridStartX() const {
		return 10;
	}

	int32_t Engine::Implementation::GetGridStartY() const {
		return 30;
	}

	int32_t Engine::Implementation::GetGridWidth() const {
		return GetGridHeight();
	}

	int32_t Engine::Implementation::GetGridHeight() const {
		return WindowHeight - GetGridStartY() * 2;
	}

	float Engine::Implementation::GetCellSize() const {
		return float(GetGridHeight() / GetGridDims());
	}

	int32_t Engine::Implementation::GetNumOfGridCells() const {
		return GetGridDims() * GetGridDims();
	}

	std::unique_ptr<SpriteBatch>& Engine::Implementation::GetBackgroundBatch()
	{
		return mBatches[Engine::IMAGE_BACKGROUND];
	}

	std::unique_ptr<SpriteBatch>& Engine::Implementation::GetDiamondBatch()
	{
		return mBatches[Engine::IMAGE_DIAMONDS];
	}

	std::unique_ptr<SpriteBatch>& Engine::Implementation::GetTextBatch()
	{
		return mBatches[Engine::IMAGE_TEXT];
	}

	Engine::Implementation::TemplateSet& Engine::Implementation::GetBackgroundTemplates() {
		return mTemplates[Engine::IMAGE_BACKGROUND];
	}

	Engine::Implementation::TemplateSet& Engine::Implementation::GetDiamondTemplates() {
		return mTemplates[Engine::IMAGE_DIAMONDS];
	}

	Engine::Implementation::TemplateSet& Engine::Implementation::GetTextTemplates() {
		return mTemplates[Engine::IMAGE_TEXT];
	}

	void Engine::Implementation::Start() {
		
		if (!mUpdater->Init()) {
			return;
		}

		while (!mQuit)
		{
			SDL_GL_SwapWindow(mSdlWindow);

			static float depth_value = 1.0f;
			static glm::vec4 view_color(.35f);
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

			// Give a chance to update
			if (mUpdater) {
				mUpdater->Update();
			}

			// Render all the batches
			for (auto i = 0; i < Engine::IMAGE_MAX; ++i)
			{
				//if (0 == i) continue;
				auto& sprite_batch = mBatches[i];
				sprite_batch->flushBuffers();
				sprite_batch->draw();
			}
		}
	}

	void Engine::Implementation::InitSpriteBatches(const std::string & assets_dir) {
		std::string texture_files[Engine::IMAGE_MAX] = {
			assets_dir + "/textures/Cells.dds",
			assets_dir + "/textures/Diamonds.dds",
			assets_dir + "/textures/berlin_sans_demi_72_0.dds"
		};

		std::string vert_shader_file = assets_dir + "/shaders/sprite.vert";
		std::string frag_shader_file = assets_dir + "/shaders/sprite.frag";

		glm::mat4 projection = glm::ortho(
			0.0f, static_cast<float>(WindowWidth),
			0.0f, static_cast<float>(WindowHeight), -1.0f, 1.0f);

		// Initialise textures and sprite batches
		for (size_t si = 0; si < Engine::IMAGE_MAX; ++si)
		{
			auto* sprite_textrue = new SpriteTexture();
			sprite_textrue->create(texture_files[si].c_str());

			auto* sprite_batch = new SpriteBatch();
			sprite_batch->init(projection, sprite_textrue->getTexId(),
				vert_shader_file.c_str(), frag_shader_file.c_str(),
				SpriteBatch::MAX_TEMPLATES, SpriteBatch::MAX_INSTANCES);

			// Add texture and sprite batch to the managed pointers
			mTextures[si].reset(sprite_textrue);
			mBatches[si].reset(sprite_batch);
		}
	}

	void Engine::Implementation::InitSpriteTemplates() {

		// Generate background templates
		{
			float x_step = float(mTextures[Engine::IMAGE_BACKGROUND]->getHeight()) /
				float(mTextures[Engine::IMAGE_BACKGROUND]->getWidth());

			for (size_t c_it = 0; c_it < Engine::CELL_MAX; ++c_it) {
				GetBackgroundTemplates().push_back(std::make_unique<SpriteBatch::Template>(
					mBatches[Engine::IMAGE_BACKGROUND]->createTemplate(
						glm::vec4(c_it * x_step, 1.f, (c_it + 1) * x_step, 0.f))));
			}
		}

		// Generate diamond templates
		{
			float x_step = float(mTextures[Engine::IMAGE_DIAMONDS]->getHeight()) /
				float(mTextures[Engine::IMAGE_DIAMONDS]->getWidth());

			for (size_t d_it = 0; d_it < Engine::DIAMOND_MAX; ++d_it) {
				GetDiamondTemplates().push_back(std::make_unique<SpriteBatch::Template>(
					mBatches[Engine::IMAGE_DIAMONDS]->createTemplate(
						glm::vec4(d_it * x_step, 1.f, (d_it + 1) * x_step, 0.f))));
			}
		}

		// TODO: Cache font glyphs
	}

	void Engine::Implementation::InitSpriteIntances() {
		// Create an instance for the cell sprite
		auto& sprite_batch = GetBackgroundBatch();

		int32_t n_cells = GetNumOfGridCells();
		float grid_step = GetCellSize();
		float gird_scale = 0.98f;
		
		int32_t row_id = -1;
		int32_t col_id = 0;

		// Create background cell instances
		auto& templates = GetBackgroundTemplates();
		for (int32_t i = 0; i < n_cells; ++i) {
			const auto& sprite_template = templates[Engine::CELL_AVAILABLE];
			mBackground[i] = sprite_batch->addInstance(*sprite_template);

			// Step up into the columns
			col_id = i % GetGridDims();
			if (col_id == 0) {
				row_id += 1;
			}

			// Update cell position
			glm::vec2 cell_pos(col_id * grid_step + GetGridStartX(), row_id * grid_step + GetGridStartY());
			glm::vec2 cell_size(grid_step * gird_scale);
			sprite_batch->updateInstance(mBackground[i], cell_pos, cell_size);
		}

		// Initialise diamond templates mapping
		for (auto i = 0; i < GetNumOfGridCells(); ++i) {
			mDiamondsTemplateMap[i] = Engine::DIAMOND_MAX;
		}
	}

	void Engine::Implementation::ParseEvents() {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN: {
				if (event.key.keysym.sym < std::numeric_limits<uint8_t>::max()) {
					mKeyDown[event.key.keysym.sym] = true;
				}

				if (event.key.keysym.sym == SDLK_ESCAPE) {
					mQuit = true;
				}

				break;
			}
			case SDL_KEYUP: {
				if (event.key.keysym.sym < std::numeric_limits<uint8_t>::max()) {
					mKeyDown[event.key.keysym.sym] = false;
				}

				break;
			}
			case SDL_QUIT:
				mQuit = true;
				break;
			case SDL_MOUSEBUTTONDOWN:
				mMouseButtonsMask |= (1 << event.button.button);
				mMouseButtonDown = true;
				break;
			case SDL_MOUSEBUTTONUP:
				mMouseButtonsMask &= ~(1 << event.button.button);
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
