#include "GraphicsPipeline.hpp"

#include <cassert>
#include <glew/glew.h>

bool GraphicsPipeline::isValid() const
{
	return glIsProgram(mProgId) && glIsProgramPipeline(mPipeId);
}

bool GraphicsPipeline::generate(bool separable)
{
	mProgId = glCreateProgram();
	glGenProgramPipelines(1, &mPipeId);

	if (separable) {
		glProgramParameteri(mProgId, GL_PROGRAM_SEPARABLE, GL_TRUE);
	}

	return true;
}

void GraphicsPipeline::destroy()
{
	glDeleteProgram(mProgId);
	mProgId = 0;

	glDeleteProgramPipelines(1, &mPipeId);
	mPipeId = 0;
}

void GraphicsPipeline::bind() const
{
	// bind shader programs
	assert(glIsProgram(mProgId));
	assert(glIsProgramPipeline(mPipeId));
	glBindProgramPipeline(mPipeId);
}
