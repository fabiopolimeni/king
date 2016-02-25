#define GLM_FORCE_RADIANS 

#include <king/Engine.h>
#include <king/Updater.h>

#include <exception>

//**********************************************************************

class ExampleGame : public King::Updater
{

public:

	ExampleGame() : mEngine("./assets") {
	}

	void RenderBackground() {
	}

	void Start() {
		mEngine.Start(*this);
	}

	void Update() {

		if (mEngine.IsMouseButtonDown())
		{
			auto cell_index = mEngine.GetCellIndex(float(mEngine.GetMouseX()), float(mEngine.GetMouseY()));
			if (cell_index >= 0)
			{
				mEngine.ChangeCell(cell_index, King::Engine::SPRITE_CELL_FULL);
				mEngine.AddDiamond(cell_index, King::Engine::SPRITE_RED);
			}

			fprintf(stderr, "Background cell: %d (%2.f, %2.f)\n",
				cell_index,	mEngine.GetMouseX(), mEngine.GetMouseY());
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


