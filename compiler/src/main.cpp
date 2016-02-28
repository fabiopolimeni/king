#define GLM_FORCE_RADIANS 

#include <king/Engine.h>
#include <king/Updater.h>

#include <exception>
#include <random>
#include <algorithm>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/common.hpp>

//**********************************************************************

using namespace King;
class ExampleGame : public King::Updater
{
	static const int32_t CHECK_STEPS;
	static const int32_t MAX_INIT_HEIGHT;
	static const float FALLING_TIME;

	struct DataTarget
	{
		glm::vec2 position;
		glm::vec2 size;
		glm::vec4 color;
		float rotation;
		float time;
		float life;
		int32_t index;
		Engine::Diamond type;
	};

	enum class GameState
	{
		INVALID,		
		INIT,			// Game started, grid initialised, but match didn't begin yet.
		MATCH_BEGUN,	// Match is begun
		GRID_EXPLODING,	// Grid is resolving, moving, exploding, spawning
		GRID_FALLING,	// Grid is resolving, moving, exploding, spawning
		GRID_SPAWNING,	// Grid is resolving, moving, exploding, spawning
		PLAYER_MOVING,	// Player is acting on the grid
		PLAYER_WAITING, //Waiting for player's to move
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
	std::vector<DataTarget> mUpdatingDiamonds;

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
		mUpdatingDiamonds.clear();

		// Debug
		mEngine.AddDiamond(0, Engine::DIAMOND_PURPLE);
		mDiamondStates.get()[0] = DiamondState::READY;

		mEngine.AddDiamond(8, Engine::DIAMOND_PURPLE);
		mDiamondStates.get()[8] = DiamondState::READY;

		mEngine.AddDiamond(16, Engine::DIAMOND_RED);
		mDiamondStates.get()[16] = DiamondState::READY;

		mEngine.AddDiamond(24, Engine::DIAMOND_YELLOW);
		mDiamondStates.get()[24] = DiamondState::READY;


		//// Fill first max rows, per column
		//for (int32_t c = 0; c < mEngine.GetGridHeight(); ++c) {

		//	int32_t rows = row_dis(row_gen);
		//	for (int32_t r = 0; r < rows; ++r) {

		//		auto grid_index = mEngine.GetGridIndex(c, r);
		//		Engine::Diamond diamond = static_cast<Engine::Diamond>(diamond_dis(diamond_gen));

		//		mEngine.AddDiamond(grid_index, diamond);
		//		mDiamondStates.get()[grid_index] = DiamondState::READY;
		//	}
		//}

		SetGameState(GameState::INIT);
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
	bool CheckFalling(float falling_time) {

		bool any_moving = false;

		// Iterate through grid rows and resolve adjacencies
		for (int32_t x = 0; x < mEngine.GetGridWidth(); ++x) {
			for (int32_t y = 1; y < mEngine.GetGridHeight(); ++y) {

				const auto curr_index = mEngine.GetGridIndex(x, y);
				DiamondState& cur_diamond = mDiamondStates.get()[curr_index];

				// If the current cell is not empty and the one below of us is,
				// then, we want to set the current diamond position to empty,
				// one to updating, set the target info to the current position.
				
				const int32_t below_index = mEngine.GetGridIndex(x, y - 1); // It always exists as we start from row 1
				DiamondState& below_diamond = mDiamondStates.get()[below_index];
				if (cur_diamond != DiamondState::EMPTY && below_diamond == DiamondState::EMPTY) {
					
					DataTarget cur_target;
					mEngine.GetDiamondData(curr_index, cur_target.position, cur_target.size, cur_target.color, cur_target.rotation);
					cur_target.position = mEngine.GetCellPosition(below_index);
					cur_target.time = falling_time;
					cur_target.life = falling_time;
					cur_target.index = below_index;
					cur_target.type = mEngine.GetGridDiamond(curr_index);

					mUpdatingDiamonds.push_back(cur_target);

					// Add a new diamond into the below position which we update with the current data
					mEngine.AddDiamond(below_index, mEngine.GetGridDiamond(curr_index));
					mEngine.UpdateDiamond(below_index, mEngine.GetCellPosition(curr_index), cur_target.size, cur_target.color, cur_target.rotation);
					below_diamond = DiamondState::UPDATING;

					// We now remove the diamond from the current position.
					mEngine.RemoveDiamond(curr_index);
					cur_diamond = DiamondState::EMPTY;

					any_moving = true;
				}
			}
		}

		return any_moving;
	}

	// Check whether all the non empty cells have diamonds in ready state
	bool IsGridReady() const {

		for (auto i = 0; i < mEngine.GetGridSize(); ++i) {

			DiamondState& diamond_state = mDiamondStates.get()[i];
			if (diamond_state != DiamondState::EMPTY && diamond_state != DiamondState::READY) {
				return false;
			}
		}

		return true;
	}

	bool IsGridUpdating() const {

		return IsGameState(GameState::GRID_EXPLODING)
			|| IsGameState(GameState::GRID_FALLING)
			|| IsGameState(GameState::GRID_SPAWNING)
			|| IsGameState(GameState::PLAYER_MOVING);
	}

	bool IsGameBegun() const {

		return mGameState >= GameState::MATCH_BEGUN;
	}

	// Keep animating the diamonds until all become ready
	void AnimateGrid(float delta_time) {

	}

	bool CheckGameBegins() {
		
		if (!IsGameState(GameState::MATCH_BEGUN) && mEngine.IsKeyDown('\r')) {
			SetGameState(GameState::MATCH_BEGUN);
		}

		return IsGameBegun();
	}

	// Advance a single diamond by one step
	// Returns false if it finished to update
	bool AdvanceDiamond(DataTarget& diamond_data, float time_step) {

		bool finished = false;

		diamond_data.life -= time_step;
		if (diamond_data.life <= 0.f) {
			diamond_data.life = 0.f;
			finished = true;
		}

		float stamp = 1.f - diamond_data.life / diamond_data.time;

		glm::vec2 position;
		glm::vec2 size;
		glm::vec4 color;
		float rotation;
		mEngine.GetDiamondData(diamond_data.index, position, size, color, rotation);
		mEngine.UpdateDiamond(diamond_data.index, 
			glm::mix(position, diamond_data.position, stamp),
			glm::mix(size, diamond_data.size, stamp),
			glm::mix(color, diamond_data.color, stamp),
			glm::mix(rotation, diamond_data.rotation, stamp));

		return !finished;
	}

	// Update all diamonds
	void UpdateGrid(float delta_time) {

		for (size_t i = 0; i < mUpdatingDiamonds.size(); ++i) {

			auto& diamond_data = mUpdatingDiamonds[i];
			if (!AdvanceDiamond(diamond_data, delta_time)) {
				mDiamondStates.get()[diamond_data.index] = DiamondState::READY;
			}
		}
	}

	////////////////////////////////////////////////

	void GenerateRandomDiamond() {

		if (mEngine.IsMouseButtonDown(1)) {

			std::random_device rd;
			std::mt19937 gen(rd());
			std::binomial_distribution<> dis(Engine::DIAMOND_YELLOW, 0.5f);

			Engine::Diamond diamond = static_cast<Engine::Diamond>(dis(gen));

			auto cell_index = mEngine.GetCellIndex(int32_t(mEngine.GetMouseX()), int32_t(mEngine.GetMouseY()));
			if (cell_index >= 0) {
				mEngine.ChangeCell(cell_index, Engine::CELL_FULL);

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
		}

		// Check whether the match started
		if (!CheckGameBegins()) {
			return;
		}

		// Check we need to resolve the grid
		if (!IsGridUpdating()) {

			// We have to run a two step pass, as removable row
			// diamonds can still account for column explosions,
			// and vice versa.
			if (CheckAdjacencies()) {
				ResolveExplosions();
				SetGameState(GameState::GRID_EXPLODING);
				return;
			}

			// If we have exploded some of the diamonds
			// we have to check for falling ones.
			if (CheckFalling(FALLING_TIME)) {
				SetGameState(GameState::GRID_FALLING);
				return;
			}

			// TODO: If round is at end and we need to spawn new diamonds

		}
		else {

			// Update pending diamonds
			UpdateGrid(mEngine.GetLastFrameSeconds());
			
			// Signal that we have finished to update
			// and we are waiting for the player to
			// make a move
			if (IsGridReady()) {
				mUpdatingDiamonds.clear();
				SetGameState(GameState::PLAYER_WAITING);
			}
		}

		// Check whether the match ended or the player won
	}

private:

	Engine mEngine;
};

//**********************************************************************
// Simple configurations
const int32_t ExampleGame::CHECK_STEPS = 2;
const int32_t ExampleGame::MAX_INIT_HEIGHT = 4;
const float ExampleGame::FALLING_TIME = 1.3f;

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


