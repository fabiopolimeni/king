#pragma once

#include <cstdint>
#include <memory>

namespace gli
{
	class texture;
}

using Surface = gli::texture;

class SpriteTexture
{

public:

	~SpriteTexture();

	bool create(const char* filename);
	void destroy();
	void use(uint32_t texture_unit);

	int32_t getWidth() const;
	int32_t getHeight() const;

	inline uint32_t getTexId() const { return mTextureId; }

private:

	uint32_t					mTextureId;

	// Shared pointer to avoid the pimpl problem with incomplete types
	std::shared_ptr<Surface>	mTexture;

};
