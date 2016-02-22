#include "SpriteTexture.hpp"
#include "OGL.hpp"

#include <gli/gli.hpp>

namespace
{
	gl::uint32 build(char const* Filename)
	{
		gli::texture Texture(gli::load(Filename));
		if (Texture.empty())
			return 0;

		gli::gl GL;
		gli::gl::format const Format = GL.translate(Texture.format());
		gli::gl::swizzles const Swizzles = GL.translate(Texture.swizzles());
		gl::enumerator Target = GL.translate(Texture.target());

		gl::uint32 TextureName = 0;
		glGenTextures(1, &TextureName);
		glBindTexture(Target, TextureName);
		glTexParameteri(Target, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(Target, GL_TEXTURE_MAX_LEVEL, static_cast<gl::int32>(Texture.levels() - 1));
		glTexParameteri(Target, GL_TEXTURE_SWIZZLE_R, Swizzles[0]);
		glTexParameteri(Target, GL_TEXTURE_SWIZZLE_G, Swizzles[1]);
		glTexParameteri(Target, GL_TEXTURE_SWIZZLE_B, Swizzles[2]);
		glTexParameteri(Target, GL_TEXTURE_SWIZZLE_A, Swizzles[3]);

		glm::tvec3<gl::sizei> const Dimensions(Texture.dimensions());

		switch (Texture.target())
		{
		case gli::TARGET_1D:
			glTexStorage1D(
				Target, static_cast<gl::int32>(Texture.levels()), Format.Internal, Dimensions.x);
			break;
		case gli::TARGET_1D_ARRAY:
		case gli::TARGET_2D:
		case gli::TARGET_CUBE:
			glTexStorage2D(
				Target, static_cast<gl::int32>(Texture.levels()), Format.Internal,
				Dimensions.x, Texture.target() == gli::TARGET_2D ? Dimensions.y : static_cast<gl::sizei>(Texture.layers() * Texture.faces()));
			break;
		case gli::TARGET_2D_ARRAY:
		case gli::TARGET_3D:
		case gli::TARGET_CUBE_ARRAY:
			glTexStorage3D(
				Target, static_cast<gl::int32>(Texture.levels()), Format.Internal,
				Dimensions.x, Dimensions.y, Texture.target() == gli::TARGET_3D ? Dimensions.z : static_cast<gl::sizei>(Texture.layers() * Texture.faces()));
			break;
		default:
			assert(0);
			break;
		}

		for (std::size_t Layer = 0; Layer < Texture.layers(); ++Layer)
			for (std::size_t Face = 0; Face < Texture.faces(); ++Face)
				for (std::size_t Level = 0; Level < Texture.levels(); ++Level)
				{
					glm::tvec3<gl::sizei> Dimensions(Texture.dimensions(Level));
					Target = gli::is_target_cube(Texture.target()) ? static_cast<gl::enumerator>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + Face) : Target;

					switch (Texture.target())
					{
					case gli::TARGET_1D:
						if (gli::is_compressed(Texture.format()))
							glCompressedTexSubImage1D(
								Target, static_cast<gl::int32>(Level), 0, Dimensions.x,
								Format.Internal, static_cast<gl::sizei>(Texture.size(Level)), Texture.data(Layer, Face, Level));
						else
							glTexSubImage1D(
								Target, static_cast<gl::int32>(Level), 0, Dimensions.x,
								Format.External, Format.Type, Texture.data(Layer, Face, Level));
						break;
					case gli::TARGET_1D_ARRAY:
					case gli::TARGET_2D:
					case gli::TARGET_CUBE:
						if (gli::is_compressed(Texture.format()))
							glCompressedTexSubImage2D(
								Target, static_cast<gl::int32>(Level),
								0, 0, Dimensions.x, Texture.target() == gli::TARGET_1D_ARRAY ? static_cast<gl::sizei>(Layer) : Dimensions.y,
								Format.Internal, static_cast<gl::sizei>(Texture.size(Level)), Texture.data(Layer, Face, Level));
						else
							glTexSubImage2D(
								Target, static_cast<gl::int32>(Level),
								0, 0, Dimensions.x, Texture.target() == gli::TARGET_1D_ARRAY ? static_cast<gl::sizei>(Layer) : Dimensions.y,
								Format.External, Format.Type, Texture.data(Layer, Face, Level));
						break;
					case gli::TARGET_2D_ARRAY:
					case gli::TARGET_3D:
					case gli::TARGET_CUBE_ARRAY:
						if (gli::is_compressed(Texture.format()))
							glCompressedTexSubImage3D(
								Target, static_cast<gl::int32>(Level),
								0, 0, 0, Dimensions.x, Dimensions.y, Texture.target() == gli::TARGET_3D ? Dimensions.z : static_cast<gl::sizei>(Layer),
								Format.Internal, static_cast<gl::sizei>(Texture.size(Level)), Texture.data(Layer, Face, Level));
						else
							glTexSubImage3D(
								Target, static_cast<gl::int32>(Level),
								0, 0, 0, Dimensions.x, Dimensions.y, Texture.target() == gli::TARGET_3D ? Dimensions.z : static_cast<gl::sizei>(Layer),
								Format.External, Format.Type, Texture.data(Layer, Face, Level));
						break;
					default: assert(0); break;
					}
				}

		return TextureName;
	}
}

bool SpriteTexture::create(const char * filename)
{
	mTextureId = 0;

	if (filename) {
		mTextureId = build(filename);
	}

	return mTextureId != 0;
}

void SpriteTexture::destroy()
{
	assert(glIsTexture(mTextureId));
	glDeleteTextures(1, &mTextureId);
	mTextureId = 0;
}

void SpriteTexture::use(uint32_t texture_unit)
{
	assert(glIsTexture(mTextureId));
	glBindTextures(texture_unit, 1, &mTextureId);
}
