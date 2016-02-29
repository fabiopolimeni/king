#define GLM_FORCE_RADIANS 

#include <king/Engine.h>
#include <king/Updater.h>

#include <exception>
#include <random>
#include <algorithm>
#include <chrono>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/common.hpp>

#define TRACKING

//**********************************************************************

using namespace King;
class ExampleGame : public King::Updater
{
	static const int32_t CHECK_STEPS;
	static const int32_t MAX_INIT_HEIGHT;
	static const float FALLING_TIME;
	static const float ROUND_TIME;
	static const float MATCH_TIME;

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

	float mRoundTime;
	float mMatchTime;
	uint32_t mPoints;

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

		// Restart round timer
		mRoundTime = ROUND_TIME;
		mMatchTime = MATCH_TIME;
		mPoints = 0;

		//// Debug
		//mEngine.AddDiamond(0, Engine::DIAMOND_PURPLE);
		//GetDiamondState(0] = DiamondState::READY;

		//mEngine.AddDiamond(8, Engine::DIAMOND_PURPLE);
		//GetDiamondState(8] = DiamondState::READY;

		//mEngine.AddDiamond(16, Engine::DIAMOND_RED);
		//GetDiamondState(16] = DiamondState::READY;

		//mEngine.AddDiamond(24, Engine::DIAMOND_YELLOW);
		//GetDiamondState(24] = DiamondState::READY;


		// Fill first max rows, per column
		for (int32_t c = 0; c < mEngine.GetGridHeight(); ++c) {

			int32_t rows = row_dis(row_gen);
			for (int32_t r = 0; r < rows; ++r) {

				auto grid_index = mEngine.GetGridIndex(c, r);
				Engine::Diamond diamond = static_cast<Engine::Diamond>(diamond_dis(diamond_gen));

				mEngine.AddDiamond(grid_index, diamond);
				GetDiamondState(grid_index) = DiamondState::READY;
			}
		}

		SetGameState(GameState::INIT);
		fprintf(stdout, "Press ENTER to start the match ...\n");
	}

	void ClearGrid() {

		for (int32_t c = 0; c < mEngine.GetGridSize(); ++c) {
			mEngine.RemoveDiamond(c);
		}
	}

	// Returns the number of matching diamonds for the current row
	int32_t GetRowSequence(int32_t col, const int32_t row) const {

		const int32_t first_index = mEngine.GetGridIndex(col, row);

		// Grab the current diamond of the cell
		Engine::Diamond first_diamond = mEngine.GetGridDiamond(first_index);

		if (first_diamond != Engine::DIAMOND_MAX &&
			(GetDiamondState(first_index) == DiamondState::READY ||
				GetDiamondState(first_index) == DiamondState::EXPLOD)) {

			int32_t matchings = 0;
			const int32_t steps = mEngine.GetGridWidth() - col;
			for (auto i = matchings; i < steps; ++i) {

				const int32_t follow_index = mEngine.GetGridIndex(++col, row);
				Engine::Diamond cell_diamond = mEngine.GetGridDiamond(follow_index);

				if (cell_diamond == first_diamond &&
					(GetDiamondState(follow_index) == DiamondState::READY ||
						GetDiamondState(follow_index) == DiamondState::EXPLOD)) {
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

		const int32_t first_index = mEngine.GetGridIndex(col, row);

		// Grab the current diamond of the cell
		Engine::Diamond first_diamond = mEngine.GetGridDiamond(first_index);

		if (first_diamond != Engine::DIAMOND_MAX &&
			(GetDiamondState(first_index) == DiamondState::READY ||
				GetDiamondState(first_index) == DiamondState::EXPLOD)) {

			int32_t matchings = 0;
			const int32_t steps = mEngine.GetGridHeight() - row;
			for (auto i = matchings; i < steps; ++i) {

				const int32_t follow_index = mEngine.GetGridIndex(col, ++row);
				Engine::Diamond cell_diamond = mEngine.GetGridDiamond(follow_index);

				if (cell_diamond == first_diamond &&
					(GetDiamondState(follow_index) == DiamondState::READY ||
						GetDiamondState(follow_index) == DiamondState::EXPLOD)) {
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
			GetDiamondState(mEngine.GetGridIndex(x + i, y)) = DiamondState::EXPLOD;
#ifdef TRACKING
			fprintf(stdout, "Exploding diamond (%d) from %s\n", i, __FUNCTION__);
#endif
		}
	}

	void ExplodeColumn(const int32_t x, const int32_t y, const int32_t steps) {

		for (auto i = 0; i < steps; ++i) {
			GetDiamondState(mEngine.GetGridIndex(x, y + i)) = DiamondState::EXPLOD;
#ifdef TRACKING
			fprintf(stdout, "Exploding diamond (%d) from %s\n", i, __FUNCTION__);
#endif
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

			DiamondState& diamond_state = GetDiamondState(i);
			if (diamond_state == DiamondState::EXPLOD) {
				diamond_state = DiamondState::EMPTY;
				mEngine.RemoveDiamond(i);

#ifdef TRACKING
				fprintf(stdout, "Removed diamond (%d) from %s\n", i, __FUNCTION__);
#endif
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
				DiamondState& cur_diamond = GetDiamondState(curr_index);

				// If the current cell is not empty and the one below of us is,
				// then, we want to set the current diamond position to empty,
				// one to updating, set the target info to the current position.
				
				//const int32_t below_index = mEngine.GetGridIndex(x, y - 1); // It always exists as we start from row 1
				const int32_t below_index = GetLowestIndex(x, y);
				if (below_index == curr_index) {
					continue;
				}

				DiamondState& below_diamond = GetDiamondState(below_index);
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

#ifdef TRACKING
					fprintf(stdout, "Updating diamond (%d) from %s\n", below_index, __FUNCTION__);
#endif

					// We now remove the diamond from the current position.
					mEngine.RemoveDiamond(curr_index);
					cur_diamond = DiamondState::EMPTY;

#ifdef TRACKING
					fprintf(stdout, "Removed diamond (%d) from %s\n", curr_index, __FUNCTION__);
#endif

					any_moving = true;
				}
			}
		}

		return any_moving;
	}

	// Advance a single diamond by one step
	// Returns false if it finished to update
	bool AdvanceDiamond(DataTarget& diamond_data, float time_step) {

		auto& diamond_state = GetDiamondState(diamond_data.index);

		if (diamond_state == DiamondState::EMPTY) {
			return false;
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

		diamond_data.life -= time_step;
		if (diamond_data.life <= 0.f) {
			diamond_data.life = 0.f;
			GetDiamondState(diamond_data.index) = DiamondState::READY;
			return false;
		}

		return true;
	}

	// Update all diamonds
	void UpdateGrid(float delta_time) {

		for (size_t i = 0; i < mUpdatingDiamonds.size(); ++i) {

			auto& diamond_data = mUpdatingDiamonds[i];
			if (!AdvanceDiamond(diamond_data, delta_time)) {

				// Remove the pending updating data
				const auto last_index = mUpdatingDiamonds.size() - 1;
				if (i < last_index) {

					// If not last element, swap this with last
					mUpdatingDiamonds[i] = mUpdatingDiamonds[last_index];
					--i; // we have to re-evaluate this entry
				}

				// Updated element will always be at the end
				mUpdatingDiamonds.pop_back();
			}
		}
	}

	// Return the diamond state
	DiamondState GetDiamondState(int32_t index) const {
		assert(index < mEngine.GetGridSize());
		return mDiamondStates.get()[index];
	}

	// Return the diamond state
	DiamondState& GetDiamondState(int32_t index) {
		assert(index < mEngine.GetGridSize());
		return mDiamondStates.get()[index];
	}

	// Given a position in the grid returns the lowest available index on the same column.
	// It returns -1 if the given position is already full.
	int32_t GetLowestIndex(const int32_t column, const int32_t row) const {

		int32_t lowest_index = mEngine.GetGridIndex(column, row);
		for (int32_t y = row; y >= 0; --y) {

			auto index = mEngine.GetGridIndex(column, y);
			if (GetDiamondState(index) == DiamondState::EMPTY) {
				assert(!mEngine.IsCellFull(index));
				lowest_index = index;
			}
		}

		return lowest_index;
	}

	// Spawn a new diamond from the top row
	void SpawnDiamond() {

		std::random_device rd;

		std::mt19937 row_gen(rd());
		std::uniform_int_distribution<> col_dis(0, mEngine.GetGridWidth() - 1);

		std::mt19937 diamond_gen(rd());
		std::uniform_int_distribution<> diamond_dis(0, Engine::DIAMOND_YELLOW);

		// Pick the column we want to spawn the object
		int32_t column = col_dis(row_gen);

		// If this column is not saturated, then spawn a new diamond, end the match otherwise.
		auto grid_index = mEngine.GetGridIndex(column, mEngine.GetGridHeight() - 1);
		//auto grid_index = LowestFreeCell(column, mEngine.GetGridHeight() - 1);
		//if (grid_index >= 0) {
		if (GetDiamondState(grid_index) == DiamondState::EMPTY) {

			// Random diamond
			Engine::Diamond diamond = static_cast<Engine::Diamond>(diamond_dis(diamond_gen));

			mEngine.AddDiamond(grid_index, diamond);

			// Special case when there is no dropping position available
			auto below_index = mEngine.GetGridIndex(column, mEngine.GetGridHeight() - 2);
			if (GetDiamondState(below_index) != DiamondState::EMPTY) {
				GetDiamondState(grid_index) = DiamondState::READY;
			}
			else {
				GetDiamondState(grid_index) = DiamondState::SPAWNING;
			}
		}
		else {

			EndMatch();
		}
	}

	// Change cell background in accordance to the state of the cell
	void UpdateBackground() {

		for (int32_t i = 0; i < mEngine.GetGridSize(); ++i) {

			const DiamondState& diamond_state = GetDiamondState(i);
			Engine::Background cell_bg = Engine::Background::CELL_FORBIDDEN;

			switch (diamond_state) {

			case DiamondState::EMPTY:
				cell_bg = Engine::Background::CELL_AVAILABLE;
				break;

			case DiamondState::READY:
				cell_bg = Engine::Background::CELL_ALLOWED;
				break;

			case DiamondState::SELECTED:
			case DiamondState::DRAGGED:
				cell_bg = Engine::Background::CELL_ALLOWED;
				break;
			}

			mEngine.ChangeCell(i, cell_bg);
		}
	}

	// Check whether all the non empty cells have diamonds in ready state
	bool IsGridReady() const {

		for (int32_t i = 0; i < mEngine.GetGridSize(); ++i) {

			const DiamondState& diamond_state = GetDiamondState(i);
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

	bool IsMatching() const {

		return mGameState >= GameState::MATCH_BEGUN && mGameState < GameState::MATCH_END;
	}

	bool CheckGameBegins() {

		if (!IsMatching() && mEngine.IsKeyDown('\r')) {
			SetGameState(GameState::MATCH_BEGUN);
			fprintf(stdout, "Match Begins!\n");
		}

		return IsMatching();
	}

	void RestartMatch() {

		ClearGrid();
		InitGrid(MAX_INIT_HEIGHT);
		UpdateBackground();
	}

	void EndMatch() {

		SetGameState(GameState::MATCH_END);
		fprintf(stdout, "Match Ended!\n");

		RestartMatch();
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
				mEngine.ChangeCell(cell_index, Engine::CELL_PICKED);

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
		, mRoundTime(ROUND_TIME)
		, mMatchTime(MATCH_TIME)
		, mPoints(0)
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

		const float delta_time = mEngine.GetLastFrameSeconds();
		
		// Check the user wants to restart the match
		if (IsMatching() && mEngine.IsKeyDown('r')) {
			RestartMatch();
		}

		// Check whether the match started
		if (!CheckGameBegins()) {
			return;
		}

		// Check we need to resolve the grid
		//if (!IsGridUpdating()) {
		//if (IsGridReady()) {

			// We have to run a two step pass, as removable row
			// diamonds can still account for column explosions,
			// and vice versa.
			if (CheckAdjacencies()) {
				ResolveExplosions();
				SetGameState(GameState::GRID_EXPLODING);
			}

			// If we have exploded some of the diamonds
			// we have to check for falling ones.
			if (CheckFalling(FALLING_TIME)) {
				SetGameState(GameState::GRID_FALLING);
			}

			// TODO: If round is at end and we need to spawn new diamonds

		//}
		//else {

			// Update pending diamonds
			if (mUpdatingDiamonds.size()) {
				UpdateGrid(delta_time);
			}
			
			// Signal that we have finished to update
			// and we are waiting for the player to
			// make a move
			if (IsGridReady()) {
				mUpdatingDiamonds.clear();
				SetGameState(GameState::PLAYER_WAITING);
			}

			// Background feedback
			UpdateBackground();
		//}

		// Update timers
		mRoundTime -= delta_time;
		mMatchTime -= delta_time;

		// Check whether we need to spawn a new diamond
		if (mRoundTime <= 0.f) {
			mRoundTime = ROUND_TIME;
			SpawnDiamond();
		}

		// Check whether the match ended or the player won
		if (mMatchTime <= 0.f) {
			mMatchTime = MATCH_TIME;
			EndMatch();
		}

#ifndef TRACKING
		fprintf(stdout, "Time left: %ds - Next spawns in %.1fs\r",
			int32_t(mMatchTime), mRoundTime);
#endif
	}

private:

	Engine mEngine;
};

//**********************************************************************
// Simple configurations
const int32_t ExampleGame::CHECK_STEPS = 3;
const int32_t ExampleGame::MAX_INIT_HEIGHT = 4;

const float ExampleGame::FALLING_TIME = 1.5f;
const float ExampleGame::ROUND_TIME = 1.0f;
const float ExampleGame::MATCH_TIME = 90.f;

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
