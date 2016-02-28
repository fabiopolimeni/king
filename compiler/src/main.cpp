#define GLM_FORCE_RADIANS 

#include <king/Engine.h>
#include <king/Updater.h>

#include <exception>
#include <random>
#include <algorithm>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

//**********************************************************************

using namespace King;
class ExampleGame : public King::Updater
{
	static const int32_t CHECK_STEPS = 2;
	static const int32_t MAX_INIT_HEIGHT = 4;

	struct DiamondTarget
	{
		glm::vec2 position;
		glm::vec2 size;
		glm::vec4 color;
		float rotation;
		int32_t index;
		float time;
	};

	enum class GameState
	{
		INVALID,		
		INIT,			// Game started, grid initialised, but match didn't begin yet.
		MATCH_BEGUN,	// Match is begun
		PLAYER_THINKING,// Waiting for player's move
		GRID_UPDATING,	// Grid is resolving, moving, exploding, spawning
		MATCH_PAUSE,	// Match is in pause, (advantage for the player? we might want to hide the diamonds)
		MATCH_END,		// The match ended, but the game is still running, waiting for the player to restart or quit
		END				// Player quit
	};

	// When the is in GRID_UPDATING, one of the following states
	// is set on at least one of diamonds, but all are in ready state.
	enum class DiamondState : uint8_t
	{
		EMPTY,		// No diamond onto the requested grid cell
		READY,		// Diamond is in its ready state
		SPAWNING,	// Diamond is spawning (moving from the top?)
		SWAP_UP,	// Diamond has been requested to be swapped up
		SWAP_DOWN,	// Diamond has been requested to be swapped down
		SWAP_RIGHT,	// Diamond has been requested to be swapped right
		SWAP_LEFT,	// Diamond has been requested to be swapped left
		DRAGGED,	// Diamond is being moved by the player
		EXPLOD,		// Diamond needs to explode
		SELECTED,	// Diamond has been selected
		FALLING,	// Diamond is falling
		UPDATING,	// Diamond is updating
	};


	GameState mGameState;
	std::unique_ptr<DiamondState> mDiamondStates;
	std::vector<DiamondTarget> mUpdatingDiamonds;

private:

	bool IsGameState(GameState state) const {
		return mGameState == state;
	}

	void SetGameState(GameState state) {
		mGameState = state;
	}

	void InitGrid(int32_t max_rows) {

		std::random_device rd;
		
		std::mt19937 row_gen(rd());
		std::uniform_int_distribution<> row_dis(0, std::min(max_rows, mEngine.GetGridHeight()));

		std::mt19937 diamond_gen(rd());
		std::uniform_int_distribution<> diamond_dis(0, Engine::DIAMOND_YELLOW);

		// Grid starts empty
		memset(mDiamondStates.get(), (uint8_t)DiamondState::EMPTY, sizeof(uint8_t) * mEngine.GetGridSize());

		// Fill first max rows, per column
		for (int32_t c = 0; c < mEngine.GetGridHeight(); ++c) {

			int32_t rows = row_dis(row_gen);
			for (int32_t r = 0; r < rows; ++r) {

				auto grid_index = mEngine.GetGridIndex(c, r);
				Engine::Diamond diamond = static_cast<Engine::Diamond>(diamond_dis(diamond_gen));

				mEngine.AddDiamond(grid_index, diamond);
				mDiamondStates.get()[grid_index] = DiamondState::READY;
			}
		}
	}

	void ClearGrid() {

		for (int32_t c = 0; c < mEngine.GetGridSize(); ++c) {
			mEngine.RemoveDiamond(c);
		}
	}

	// Returns the number of matching diamonds for the current row
	int32_t GetRowSequence(int32_t col, const int32_t row) const {

		// Grab the current diamond of the cell
		Engine::Diamond first_diamond = mEngine.GetGridDiamond(
			mEngine.GetGridIndex(col, row));

		if (first_diamond != Engine::DIAMOND_MAX) {

			int32_t matchings = 0;
			const int32_t steps = mEngine.GetGridWidth() - col;
			for (auto i = matchings; i < steps; ++i) {

				Engine::Diamond cell_diamond = mEngine.GetGridDiamond(
					mEngine.GetGridIndex(++col, row));

				if (cell_diamond == first_diamond) {
					++matchings;
				}
				// Early break, as we can now jump
				// to the next non-matching cell
				else break;
			}

			return matchings;
		}

		// Empty cell
		return 0;
	}

	// Returns the number of matching diamonds for the current column
	int32_t GetColumnSequence(const int32_t col, int32_t row) const {

		// Grab the current diamond of the cell
		Engine::Diamond first_diamond = mEngine.GetGridDiamond(
			mEngine.GetGridIndex(col, row));

		if (first_diamond != Engine::DIAMOND_MAX) {

			int32_t matchings = 0;
			const int32_t steps = mEngine.GetGridHeight() - row;
			for (auto i = matchings; i < steps; ++i) {

				Engine::Diamond cell_diamond = mEngine.GetGridDiamond(
					mEngine.GetGridIndex(col, ++row));

				if (cell_diamond == first_diamond) {
					++matchings;
				}
				// Early break, as we can now jump
				// to the next non-matching cell
				else break;
			}

			return matchings;
		}

		// Empty cell
		return 0;
	}

	void ExplodeRow(const int32_t x, const int32_t y, const int32_t steps) {

		for (auto i = 0; i < steps; ++i) {
			mDiamondStates.get()[mEngine.GetGridIndex(x + i, y)] = DiamondState::EXPLOD;
		}
	}

	void ExplodeColumn(const int32_t x, const int32_t y, const int32_t steps) {

		for (auto i = 0; i < steps; ++i) {
			mDiamondStates.get()[mEngine.GetGridIndex(x, y + i)] = DiamondState::EXPLOD;
		}
	}

	bool CheckRowAdjacencies() {

		bool any_explosion = false;
		
		// Iterate through grid rows and resolve adjacencies
		for (int32_t y = 0; y < mEngine.GetGridHeight(); ++y) {

			const auto row = y;
			int32_t x = 0;

			// If not enough positions to check, early break
			while (x + CHECK_STEPS <= mEngine.GetGridWidth()) {

				auto matchings = GetRowSequence(x, row) + 1;
				if (matchings >= CHECK_STEPS) {

					// Remove the matching diamonds from the row
					ExplodeRow(x, row, matchings);
					any_explosion = true;
				}

				// increment x position
				x += matchings;
			}
		}

		return any_explosion;
	}

	bool CheckColumnAdjacencies() {

		bool any_explosion = false;

		// Iterate through grid rows and resolve adjacencies
		for (int32_t x = 0; x < mEngine.GetGridWidth(); ++x) {

			const auto col = x;
			int32_t y = 0;

			// If not enough positions to check, early break
			while (y + CHECK_STEPS <= mEngine.GetGridHeight()) {

				auto matchings = GetColumnSequence(col, y) + 1;
				if (matchings >= CHECK_STEPS) {

					// Remove the matching diamonds from the row
					ExplodeColumn(col, y, matchings);
					any_explosion = true;
				}

				// increment x position
				y += matchings;
			}
		}

		return any_explosion;
	}

	// Check adjacencies and mark diamond positions accordingly
	bool CheckAdjacencies() {

		bool any = false;
		any |= CheckRowAdjacencies();
		any |= CheckColumnAdjacencies();

		return any;
	}

	// Resolve diamond states after a grid check
	void ResolveExplosions() {

		for (auto i = 0; i < mEngine.GetGridSize(); ++i) {

			DiamondState& diamond_state = mDiamondStates.get()[i];
			if (diamond_state == DiamondState::EXPLOD) {
				diamond_state = DiamondState::EMPTY;
				mEngine.RemoveDiamond(i);
			}
			
		}
	}

	// Check whether any of the column needs to start falling and mark cells accordingly
	bool CheckColumnFalling() {

		bool any_moving = false;

		// Iterate through grid rows and resolve adjacencies
		for (int32_t x = 0; x < mEngine.GetGridWidth(); ++x) {
			for (int32_t y = 0; y < mEngine.GetGridHeight(); ++y) {

				const auto index = mEngine.GetGridIndex(x, y);
				DiamondState& diamond_state = mDiamondStates.get()[index];

				// If the current cell is empty and the one above of us is not,
				// then, we want to set the top diamond to empty, and the current
				// one to updating, set the target info to the current position
				int32_t above_index = y + 1;
				if (diamond_state == DiamondState::EMPTY && above_index < mEngine.GetGridHeight()) {
					
					DiamondState& diamond_above = mDiamondStates.get()[above_index];
					diamond_above = DiamondState::EMPTY;
					diamond_state = DiamondState::UPDATING;

					mUpdatingDiamonds.push_back({

					});

					any_moving = true;
				}
			}
		}

		return any_moving;
	}

	// Check whether all the non empty cells have diamonds in ready state
	bool IsGridReady() {

		for (auto i = 0; i < mEngine.GetGridSize(); ++i) {

			DiamondState& diamond_state = mDiamondStates.get()[i];
			if (diamond_state != DiamondState::EMPTY && diamond_state != DiamondState::READY) {
				return false;
			}
		}

		return true;
	}

	// Keep animating the diamonds until all become ready
	void AnimateGrid(float delta_time) {

	}

	bool CheckGameBegins() {
		
		if (!IsGameState(GameState::MATCH_BEGUN) && mEngine.IsKeyDown('\r')) {
			SetGameState(GameState::MATCH_BEGUN);
		}

		return IsGameState(GameState::MATCH_BEGUN);
	}


	////////////////////////////////////////////////

	void GenerateRandomDiamond() {

		if (mEngine.IsMouseButtonDown(1)) {

			std::random_device rd;
			std::mt19937 gen(rd());

			//std::uniform_int_distribution<> dis(0, King::Engine::DIAMOND_YELLOW);
			std::binomial_distribution<> dis(King::Engine::DIAMOND_YELLOW, 0.5f);

			King::Engine::Diamond diamond = static_cast<King::Engine::Diamond>(dis(gen));

			auto cell_index = mEngine.GetCellIndex(int32_t(mEngine.GetMouseX()), int32_t(mEngine.GetMouseY()));
			if (cell_index >= 0) {
				mEngine.ChangeCell(cell_index, King::Engine::CELL_FULL);

				// Generate random diamond
				if (!mEngine.IsCellFull(cell_index)) {
					mEngine.AddDiamond(cell_index, diamond);

					fprintf(stdout, "Background cell: %d (%2.f, %2.f)\nDiamond %d\n",
						cell_index, mEngine.GetMouseX(), mEngine.GetMouseY(), diamond);
				}
			}
		}
	}

	void RemoveDiamond() {

		if (mEngine.IsMouseButtonDown(3)) {

			auto cell_index = mEngine.GetCellIndex(int32_t(mEngine.GetMouseX()), int32_t(mEngine.GetMouseY()));
			if (cell_index >= 0) {

				// Remove diamond
				if (mEngine.IsCellFull(cell_index)) {

					mEngine.RemoveDiamond(cell_index);
					fprintf(stdout, "Removed diamond: %d \n", cell_index);
				}
			}
		}
	}

	bool Testing() {

		bool bTest = false;

		if (bTest) {

			GenerateRandomDiamond();
			RemoveDiamond();
		}

		return bTest;
	}

public:

	ExampleGame()
		: mEngine("./assets")
		, mGameState(GameState::INVALID)
		, mDiamondStates(new DiamondState[mEngine.GetGridSize()])
	{
	}

	void RenderBackground() {
	}

	void Start() {
		mEngine.Start(*this);
	}

	bool Init() {
		InitGrid(MAX_INIT_HEIGHT);
		SetGameState(GameState::INIT);
		return true;
	}

	void Update() {

		if (Testing()) {
			return;
		}
		
		// Check the user wants to restart the match
		if ((IsGameState(GameState::MATCH_BEGUN) || IsGameState(GameState::MATCH_END))
			&& mEngine.IsKeyDown('r'))
		{
			ClearGrid();
			InitGrid(MAX_INIT_HEIGHT);
			SetGameState(GameState::INIT);
		}

		// Check whether the match started
		if (!CheckGameBegins()) {
			return;
		}

		// Check we need to resolve the grid
		if (!IsGameState(GameState::GRID_UPDATING)) {

			// We have to run a two step pass, as removable row
			// diamonds can still account for column explosions,
			// and vice versa.
			if (CheckAdjacencies()) {
				ResolveExplosions();
				CheckColumnFalling();
			}
		}

		// Check whether the match ended or the player won
	}

private:

	Engine mEngine;
};

//**********************************************************************

int main(int argc, char *argv[])
{
	try
	{
		ExampleGame game;
		game.Start();
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "Caught exception: %s\n", e.what());
		assert(0);
	}

	return 0;
}


