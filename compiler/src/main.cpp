#define GLM_FORCE_RADIANS 

#include <king/Engine.h>
#include <king/Updater.h>

#include <exception>

//**********************************************************************

class ExampleGame : public King::Updater {
public:

	ExampleGame()
		: mEngine("./assets")
		, mRotation(0.0f)
		, mYellowDiamondX(100.0f)
		, mYellowDiamondY(100.0f) {
	}

	void Start() {
		mEngine.Start(*this);
	}

	void Update() {
		mEngine.Render(King::Engine::SPRITE_GREEN, 650.0f, 100.0f);
	}

private:
	King::Engine mEngine;
	float mRotation;
	float mYellowDiamondX;
	float mYellowDiamondY;
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


