/// @brief Include to use 2d textures.
/// @file gli/texture2d.hpp

#pragma once

#include "texture.hpp"
#include "image.hpp"
#include "index.hpp"

namespace gli
{
	/// 2d texture
	class texture2D : public texture
	{
	public:
		typedef ivec2 texelcoord_type;

		/// Create an empty texture 2D.
		texture2D();

		/// Create a texture2D and allocate a new storage.
		explicit texture2D(
			format_type Format,
			texelcoord_type const & Dimensions,
			size_type Levels);

		/// Create a texture2D and allocate a new storage with a complete mipmap chain.
		explicit texture2D(
			format_type Format,
			texelcoord_type const & Dimensions);

		/// Create a texture2D view with an existing storage.
		explicit texture2D(
			texture const & Texture);

		/// Create a texture2D view with an existing storage.
		explicit texture2D(
			texture const & Texture,
			format_type Format,
			size_type BaseLayer, size_type MaxLayer,
			size_type BaseFace, size_type MaxFace,
			size_type BaseLevel, size_type MaxLevel);

		/// Create a texture2D view, reference a subset of an existing texture2D instance.
		explicit texture2D(
			texture2D const & Texture,
			size_type BaseLevel, size_type MaxLevel);

		/// Create a view of the image identified by Level in the mipmap chain of the texture.
		image operator[](size_type Level) const;

		/// Return the dimensions of a texture instance: width and height.
		texelcoord_type dimensions(size_type Level = 0) const;

		/// Fetch a texel from a texture. The texture format must be uncompressed.
		template <typename genType>
		genType load(texelcoord_type const & TexelCoord, size_type Level) const;

		/// Write a texel to a texture. The texture format must be uncompressed.
		template <typename genType>
		void store(texelcoord_type const & TexelCoord, size_type Level, genType const & Texel);

		/// Clear the entire texture storage with zeros
		void clear();

		/// Clear the entire texture storage with Texel which type must match the texture storage format block size
		/// If the type of genType doesn't match the type of the texture format, no conversion is performed and the data will be reinterpreted as if is was of the texture format. 
		template <typename genType>
		void clear(genType const & Texel);

		/// Clear a specific image of a texture.
		template <typename genType>
		void clear(size_type Level, genType const & Texel);

	private:
		struct cache
		{
			std::uint8_t* Data;
			texelcoord_type Dim;
#			ifndef NDEBUG
				size_type Size;
#			endif
		};

		void build_cache();
		size_type index_cache(size_type Level) const;

		std::vector<cache> Caches;
	};
}//namespace gli

#include "./core/texture2d.inl"
