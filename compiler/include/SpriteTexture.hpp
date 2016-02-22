#pragma once

#include <gli/gli.hpp>

#include <cstdint>
#include <memory>

class SpriteTexture
{

public:

	bool create(const char* filename);
	void destroy();
	void use(uint32_t texture_unit);

	int32_t getWidth() const;
	int32_t getHeight() const;

	inline uint32_t getTexId() const { return mTextureId; }

private:

	using Surface = gli::texture;

	uint32_t					mTextureId;
	std::unique_ptr<Surface>	mTexture;
};
