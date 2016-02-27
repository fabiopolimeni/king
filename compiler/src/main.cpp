#define GLM_FORCE_RADIANS 

#include <king/Engine.h>
#include <king/Updater.h>

#include <exception>
#include <random>
#include <algorithm>

//**********************************************************************

class ExampleGame : public King::Updater
{

	bool bIsAnimating;

private:

	void InitGrid(int32_t max_rows) {

		std::random_device rd;
		
		std::mt19937 row_gen(rd());
		std::uniform_int_distribution<> row_dis(0, std::min(max_rows, mEngine.GetGridHeight()));

		std::mt19937 diamond_gen(rd());
		std::uniform_int_distribution<> diamond_dis(0, King::Engine::DIAMOND_YELLOW);

		// Fill first max rows, per column
		for (int32_t c = 0; c < mEngine.GetGridHeight(); ++c) {

			int32_t rows = row_dis(row_gen);
			for (int32_t r = 0; r < rows; ++r) {

				auto grid_index = mEngine.GetGridIndex(c, r);
				King::Engine::Diamond diamond = static_cast<King::Engine::Diamond>(diamond_dis(diamond_gen));

				mEngine.AddDiamond(grid_index, diamond);
			}
		}

	}

	bool ResolveGrid() {

		// Iterate through the grid and resolve adjacencies
		for (int32_t c = 0; c < mEngine.GetGridHeight(); ++c) {
			for (int32_t r = 0; r < mEngine.GetGridHeight(); ++r) {


			}
		}

		return false;
	}

	bool IsGridAnimating() const {
		return false;
	}

	void AnimateGrid() {


	}

public:

	ExampleGame() : mEngine("./assets") {
	}

	void RenderBackground() {
	}

	void Start() {
		mEngine.Start(*this);
	}

	bool Init() {
		InitGrid(4);
		return true;
	}

	void Update() {
		
		if (!IsGridAnimating()) {
			ResolveGrid();
		}
	}

private:

	King::Engine mEngine;
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


