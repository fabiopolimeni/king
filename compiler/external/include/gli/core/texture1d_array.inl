#include "../levels.hpp"

namespace gli
{
	inline texture1DArray::texture1DArray()
	{}

	inline texture1DArray::texture1DArray(format_type Format, texelcoord_type const & Dimensions, size_type Layers)
		: texture(TARGET_1D_ARRAY, Format, texture::texelcoord_type(Dimensions.x, 1, 1), Layers, 1, gli::levels(Dimensions))
	{
		this->build_cache();
	}

	inline texture1DArray::texture1DArray(format_type Format, texelcoord_type const & Dimensions, size_type Layers, size_type Levels)
		: texture(TARGET_1D_ARRAY, Format, texture::texelcoord_type(Dimensions.x, 1, 1), Layers, 1, Levels)
	{
		this->build_cache();
	}

	inline texture1DArray::texture1DArray(texture const & Texture)
		: texture(Texture, TARGET_1D_ARRAY, Texture.format())
	{
		this->build_cache();
	}

	inline texture1DArray::texture1DArray
	(
		texture const & Texture,
		format_type Format,
		size_type BaseLayer, size_type MaxLayer,
		size_type BaseFace, size_type MaxFace,
		size_type BaseLevel, size_type MaxLevel
	)
		: texture(
			Texture, TARGET_1D_ARRAY, Format,
			BaseLayer, MaxLayer,
			BaseFace, MaxFace,
			BaseLevel, MaxLevel)
	{
		this->build_cache();
	}

	inline texture1DArray::texture1DArray
	(
		texture1DArray const & Texture,
		size_type BaseLayer, size_type MaxLayer,
		size_type BaseLevel, size_type MaxLevel
	)
		: texture(
			Texture, TARGET_1D_ARRAY,
			Texture.format(),
			Texture.base_layer() + BaseLayer, Texture.base_layer() + MaxLayer,
			Texture.base_face(), Texture.max_face(),
			Texture.base_level() + BaseLevel, Texture.base_level() + MaxLevel)
	{
		this->build_cache();
	}

	inline texture1D texture1DArray::operator[](size_type Layer) const
	{
		GLI_ASSERT(!this->empty());
		GLI_ASSERT(Layer < this->layers());

		return texture1D(
			*this, this->format(),
			this->base_layer() + Layer, this->base_layer() + Layer,
			this->base_face(), 	this->max_face(),
			this->base_level(), this->max_level());
	}

	inline texture1DArray::texelcoord_type texture1DArray::dimensions(size_type Level) const
	{
		GLI_ASSERT(!this->empty());

		return this->Caches[this->index_cache(0, Level)].Dim;
	}

	template <typename genType>
	inline genType texture1DArray::load(texelcoord_type const & TexelCoord, size_type Layer, size_type Level) const
	{
		GLI_ASSERT(!this->empty());
		GLI_ASSERT(!is_compressed(this->format()));
		GLI_ASSERT(block_size(this->format()) == sizeof(genType));

		cache const & Cache = this->Caches[this->index_cache(Layer, Level)];

		std::size_t const Index = linear_index(TexelCoord, Cache.Dim);
		GLI_ASSERT(Index < Cache.Size / sizeof(genType));

		return reinterpret_cast<genType const * const>(Cache.Data)[Index];
	}

	template <typename genType>
	inline void texture1DArray::store(texelcoord_type const & TexelCoord, size_type Layer, size_type Level, genType const & Texel)
	{
		GLI_ASSERT(!this->empty());
		GLI_ASSERT(!is_compressed(this->format()));
		GLI_ASSERT(block_size(this->format()) == sizeof(genType));

		cache& Cache = this->Caches[this->index_cache(Layer, Level)];
		GLI_ASSERT(glm::all(glm::lessThan(TexelCoord, Cache.Dim)));

		std::size_t const Index = linear_index(TexelCoord, Cache.Dim);
		GLI_ASSERT(Index < Cache.Size / sizeof(genType));

		reinterpret_cast<genType*>(Cache.Data)[Index] = Texel;
	}

	inline void texture1DArray::clear()
	{
		this->texture::clear();
	}

	template <typename genType>
	inline void texture1DArray::clear(genType const & Texel)
	{
		this->texture::clear<genType>(Texel);
	}

	template <typename genType>
	inline void texture1DArray::clear(size_type Layer, size_type Level, genType const & Texel)
	{
		this->texture::clear<genType>(Layer, 0, Level, Texel);
	}

	inline texture1DArray::size_type texture1DArray::index_cache(size_type Layer, size_type Level) const
	{
		return Layer * this->levels() + Level;
	}

	inline void texture1DArray::build_cache()
	{
		this->Caches.resize(this->layers() * this->levels());

		for(size_type Layer = 0; Layer < this->layers(); ++Layer)
		for(size_type Level = 0; Level < this->levels(); ++Level)
		{
			cache& Cache = this->Caches[this->index_cache(Layer, Level)];
			Cache.Data = this->data<std::uint8_t>(Layer, 0, Level);
			Cache.Dim = glm::max(texelcoord_type(this->texture::dimensions(Level)), texelcoord_type(1));
#			ifndef NDEBUG
				Cache.Size = this->size(Level);
#			endif
		}
	}
}//namespace gli


