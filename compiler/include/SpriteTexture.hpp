#pragma once

#include <cstdint>

class SpriteTexture
{

public:

	bool create(const char* filename);
	void destroy();
	void use(uint32_t texture_unit);

	inline uint32_t getTexId() const { return mTextureId; }

private:

	uint32_t mTextureId;
};
