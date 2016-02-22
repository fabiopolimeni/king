#include "ShaderCompiler.hpp"
#include "OGL.hpp"
#include "format.hpp"

#include <cassert>
#include <vector>
#include <string>
#include <fstream>
#include <exception>

namespace gl
{
	gl::enumerator toShaderType(GraphicsPipeline::StageType stage_type)
	{
		switch (stage_type)
		{
		case GraphicsPipeline::eST_VERTEX:
			return GL_VERTEX_SHADER;
		case GraphicsPipeline::eST_TESS_CONTROL:
			return GL_TESS_CONTROL_SHADER;
		case GraphicsPipeline::eST_TESS_EVAL:
			return GL_TESS_EVALUATION_SHADER;
		case GraphicsPipeline::eST_GEOMETRY:
			return GL_GEOMETRY_SHADER;
		case GraphicsPipeline::eST_FRAGMENT:
			return GL_FRAGMENT_SHADER;
		case GraphicsPipeline::eST_COMPUTE:
			return GL_COMPUTE_SHADER;
		default:
			assert(0); // Invalid pipeline shader stage
		}

		return 0;
	}

	gl::enumerator toShaderStage(GraphicsPipeline::StageType stage_type)
	{
		switch (stage_type)
		{
		case GraphicsPipeline::eST_VERTEX:
			return GL_VERTEX_SHADER_BIT;
		case GraphicsPipeline::eST_TESS_CONTROL:
			return GL_TESS_CONTROL_SHADER_BIT;
		case GraphicsPipeline::eST_TESS_EVAL:
			return GL_TESS_EVALUATION_SHADER_BIT;
		case GraphicsPipeline::eST_GEOMETRY:
			return GL_GEOMETRY_SHADER_BIT;
		case GraphicsPipeline::eST_FRAGMENT:
			return GL_FRAGMENT_SHADER_BIT;
		case GraphicsPipeline::eST_COMPUTE:
			return GL_COMPUTE_SHADER_BIT;
		default:
			assert(0); // Invalid pipeline shader stage
		}

		return GL_ALL_SHADER_BITS;
	}

	gl::uint32 createShader(GraphicsPipeline::StageType stage_type, const std::string& shader_source)
	{
		gl::uint32 shader_name = glCreateShader(gl::toShaderType(stage_type));

		const size_t source_length = shader_source.length() + 1;
		char* shader_source_ptr = new char[source_length];
		memcpy_s(shader_source_ptr, source_length, shader_source.c_str(), source_length);
		glShaderSource(shader_name, 1, const_cast<const char**>(&shader_source_ptr), nullptr);
		glCompileShader(shader_name);

		delete[] shader_source_ptr;
		shader_source_ptr = nullptr;

		return shader_name;
	}

	bool checkShader(gl::uint32 const & shader_name, std::vector<char>& log_buffer)
	{
		GLint result = GL_FALSE;
		glGetShaderiv(shader_name, GL_COMPILE_STATUS, &result);

		if (result == GL_TRUE)
			return true;

		int log_length;
		glGetShaderiv(shader_name, GL_INFO_LOG_LENGTH, &log_length);
		if (log_length > 0)
		{
			log_buffer.resize(log_length);
			glGetShaderInfoLog(shader_name, log_length, NULL, &log_buffer[0]);
		}

		return false;
	}

	bool checkProgram(gl::uint32 prog_name, std::vector<char>& log_buffer)
	{
		gl::int32 result = GL_FALSE;
		glGetProgramiv(prog_name, GL_LINK_STATUS, &result);

		if (result == GL_TRUE)
			return true;

		int log_length;
		glGetProgramiv(prog_name, GL_INFO_LOG_LENGTH, &log_length);
		if (log_length > 0)
		{
			log_buffer.resize(log_length);
			glGetProgramInfoLog(prog_name, log_length, NULL, &log_buffer[0]);
		}

		return false;
	}

	std::string loadSource(const char* filename)
	{
		std::string text;

		std::ifstream stream(filename);
		if (!stream.is_open())
			return text;

		stream.seekg(0, std::ios::end);
		text.reserve((size_t)stream.tellg());
		stream.seekg(0, std::ios::beg);

		text.assign(
			(std::istreambuf_iterator<char>(stream)),
			std::istreambuf_iterator<char>());

		return text;
	}
}

GraphicsPipeline ShaderCompiler::buildFromFiles(
	std::array<const char*, GraphicsPipeline::StageType::eST_MAX> filenames)
{
	GraphicsPipeline graphics_pipeline;
	graphics_pipeline.generate(true);

	gl::uint32 stage_mask = 0x0;

	// Iterate through the shader stages
	for (size_t fi = 0; fi < GraphicsPipeline::StageType::eST_MAX; ++fi)
	{
		// If the stage is requested
		if (auto file_name = filenames[fi])
		{
			// Load source code from file
			auto shader_source = gl::loadSource(file_name);

			// Create the relevant shader
			gl::uint32 shader_name = gl::createShader(
				GraphicsPipeline::StageType(fi), shader_source);

			// Check the shader
			std::vector<char> out_log;
			if (!gl::checkShader(shader_name, out_log))
			{
				throw std::runtime_error(fmt::format("Shader {} errors:\n{}\n", file_name, &out_log[0]));
			}

			// Attach shader to program
			glAttachShader(graphics_pipeline.getPorgId(), shader_name);
			glDeleteShader(shader_name);

			// Add stage to the stage mask
			stage_mask |= gl::toShaderStage(GraphicsPipeline::StageType(fi));
		}
	}

	// Link the program
	gl::uint32 program_name = graphics_pipeline.getPorgId();
	glLinkProgram(program_name);

	// Check program
	std::vector<char> prog_log;
	if (!gl::checkProgram(program_name, prog_log))
	{
		throw std::runtime_error(fmt::format("Program errors:\n{}\n", &prog_log[0]));
	}

	// Bind program stages to the pipeline
	glUseProgramStages(graphics_pipeline.getPipeId(), stage_mask, program_name);

	return graphics_pipeline;
}
