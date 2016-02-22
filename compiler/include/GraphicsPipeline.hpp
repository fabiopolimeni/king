#pragma once

#include <cstdint>

class GraphicsPipeline
{

private:

	uint32_t	mPipeId;
	uint32_t	mProgId;

public:

	// Tessellation, geometry and compute
	// stages are here for reference only.
	enum StageType
	{
		eST_VERTEX,
		eST_TESS_CONTROL,
		eST_TESS_EVAL,
		eST_GEOMETRY,
		eST_FRAGMENT,
		eST_COMPUTE,
		eST_MAX
	};

	GraphicsPipeline() : mPipeId(0), mProgId(0) { }

	uint32_t getPipeId() const { return mPipeId; }
	uint32_t getPorgId() const { return mProgId; }

	bool isValid() const;

	bool generate(bool separable);
	void destroy();
	void bind() const;

};