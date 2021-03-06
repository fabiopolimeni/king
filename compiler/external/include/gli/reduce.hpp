/// @brief Include to perform reduction operations.
/// @file gli/reduce.hpp

#pragma once

#include "texture1d.hpp"
#include "texture1d_array.hpp"
#include "texture2d.hpp"
#include "texture2d_array.hpp"
#include "texture3d.hpp"
#include "texture_cube.hpp"
#include "texture_cube_array.hpp"

namespace gli
{
	template <typename vec_type>
	struct reduce_func
	{
		typedef vec_type(*type)(vec_type const & A, vec_type const & B);
	};

	template <typename vec_type>
	vec_type reduce(texture1D const & In0, texture1D const & In1, typename reduce_func<vec_type>::type TexelFunc, typename reduce_func<vec_type>::type ReduceFunc);

	template <typename vec_type>
	vec_type reduce(texture1DArray const & In0, texture1DArray const & In1, typename reduce_func<vec_type>::type TexelFunc, typename reduce_func<vec_type>::type ReduceFunc);

	template <typename vec_type>
	vec_type reduce(texture2D const & In0, texture2D const & In1, typename reduce_func<vec_type>::type TexelFunc, typename reduce_func<vec_type>::type ReduceFunc);

	template <typename vec_type>
	vec_type reduce(texture2DArray const & In0, texture2DArray const & In1, typename reduce_func<vec_type>::type TexelFunc, typename reduce_func<vec_type>::type ReduceFunc);

	template <typename vec_type>
	vec_type reduce(texture3D const & In0, texture3D const & In1, typename reduce_func<vec_type>::type TexelFunc, typename reduce_func<vec_type>::type ReduceFunc);

	template <typename vec_type>
	vec_type reduce(textureCube const & In0, textureCube const & In1, typename reduce_func<vec_type>::type TexelFunc, typename reduce_func<vec_type>::type ReduceFunc);

	template <typename vec_type>
	vec_type reduce(textureCubeArray const & In0, textureCubeArray const & In1, typename reduce_func<vec_type>::type TexelFunc, typename reduce_func<vec_type>::type ReduceFunc);

	template <typename texture_type, typename vec_type>
	vec_type reduce(texture_type const & In0, texture_type const & In1, typename reduce_func<vec_type>::type TexelFunc, typename reduce_func<vec_type>::type ReduceFunc);

}//namespace gli

#include "./core/reduce.inl"
