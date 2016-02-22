#pragma once

#include "GraphicsPipeline.hpp"

#include <array>

class ShaderCompiler
{
	
public:

	// Expects a list of shader source files per stage.
	// A null pointer if the stage should not be taken into account.
	static GraphicsPipeline buildFromFiles(
		std::array<const char*, GraphicsPipeline::StageType::eST_MAX> filenames);

};
