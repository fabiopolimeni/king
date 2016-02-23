#include "GlContext.h"
#include "SdlWindow.h"

#include <stdexcept>
#include <string>
#include <cassert>

#include <glew/glew.h>
#include <glew/wglew.h>

#define DEBUG_DRIVER_VERBOSE
void CALLBACK DebugCallback(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
	assert(userParam);
	King::GlContext* ctx = static_cast<King::GlContext*>(const_cast<GLvoid*>(userParam));

	char debSource[32] = {};
	char debType[32] = {};
	char debSev[32] = {};

	if (source == GL_DEBUG_SOURCE_API_ARB)
		strcpy_s(debSource, sizeof(debSource), "OpenGL");
	else if (source == GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB)
		strcpy_s(debSource, sizeof(debSource), "Windows");
	else if (source == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB)
		strcpy_s(debSource, sizeof(debSource), "Shader Compiler");
#if defined(DEBUG_DRIVER_VERBOSE)
	else if (source == GL_DEBUG_SOURCE_THIRD_PARTY_ARB)
		strcpy_s(debSource, sizeof(debSource), "Third Party");
	else if (source == GL_DEBUG_SOURCE_APPLICATION_ARB)
		strcpy_s(debSource, sizeof(debSource), "Application");
	else if (source == GL_DEBUG_SOURCE_OTHER_ARB)
		strcpy_s(debSource, sizeof(debSource), "Other");
	else
		assert(0);
#endif

	if (type == GL_DEBUG_TYPE_ERROR)
		strcpy_s(debType, sizeof(debType), "error");
	else if (type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)
		strcpy_s(debType, sizeof(debType), "deprecated behavior");
	else if (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
		strcpy_s(debType, sizeof(debType), "undefined behavior");
	else if (type == GL_DEBUG_TYPE_PORTABILITY)
		strcpy_s(debType, sizeof(debType), "portability");
	else if (type == GL_DEBUG_TYPE_PERFORMANCE)
		strcpy_s(debType, sizeof(debType), "performance");
#if defined(DEBUG_DRIVER_VERBOSE)
	else if (type == GL_DEBUG_TYPE_OTHER)
		strcpy_s(debType, sizeof(debType), "message");
	else if (type == GL_DEBUG_TYPE_MARKER)
		strcpy_s(debType, sizeof(debType), "marker");
	else if (type == GL_DEBUG_TYPE_PUSH_GROUP)
		strcpy_s(debType, sizeof(debType), "push group");
	else if (type == GL_DEBUG_TYPE_POP_GROUP)
		strcpy_s(debType, sizeof(debType), "pop group");
	else
		assert(0);
#endif

	if (severity == GL_DEBUG_SEVERITY_HIGH_ARB)
	{
		strcpy_s(debSev, sizeof(debSev), "high");
	}
	else if (severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)
		strcpy_s(debSev, sizeof(debSev), "medium");
#if defined(DEBUG_DRIVER_VERBOSE)
	else if (severity == GL_DEBUG_SEVERITY_LOW_ARB)
		strcpy_s(debSev, sizeof(debSev), "low");
	else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
		strcpy_s(debSev, sizeof(debSev), "notification");
	else
		assert(0);
#endif

	if (debType[0] != 0)
	{
		char error_msg[512] = { 0 };
		sprintf_s(error_msg, "%s: %s(%s) %d: %s\n", debSource, debType, debSev, id, message);

		if (strcmp(debType, "error") == 0) {
			throw std::runtime_error(std::string(error_msg));
		}
		else {
			fprintf(stdout, "%s", error_msg);
		}
	}
}

namespace King {
	GlContext::GlContext(SdlWindow& sdlWindow)
		: mContext(SDL_GL_CreateContext(sdlWindow), SDL_GL_DeleteContext)
	{
		const char* error = SDL_GetError();
		if (*error != '\0') {
			throw std::runtime_error(std::string("Error initialising OpenGL context: ") + error);
		}

		glewExperimental = GL_TRUE;
		GLenum err = glewInit();
		
		// Notoriously GLEW causes an error at initialisation
		glGetError();

		if (GLEW_OK != err)
		{
			throw std::runtime_error(std::string("Error initialising GLEW : ") + (const char*)glewGetErrorString(err));
		}

		fprintf(stdout, "OpenGL context: %s\n", glGetString(GL_VERSION));

		if (glDebugMessageCallback != NULL)
		{
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
			glDebugMessageCallback(&DebugCallback, this);
		}

		CheckError("OpenGL context create");
	}
	
	GlContext::operator SDL_GLContext() {
		return mContext.get();
	}

	bool GlContext::CheckError(const char * name) const
	{
		int glerror;
		if ((glerror = glGetError()) != GL_NO_ERROR)
		{
			std::string error_string;
			switch (glerror)
			{
			case GL_INVALID_ENUM:
				error_string = "GL_INVALID_ENUM";
				break;
			case GL_INVALID_VALUE:
				error_string = "GL_INVALID_VALUE";
				break;
			case GL_INVALID_OPERATION:
				error_string = "GL_INVALID_OPERATION";
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				error_string = "GL_INVALID_FRAMEBUFFER_OPERATION";
				break;
			case GL_OUT_OF_MEMORY:
				error_string = "GL_OUT_OF_MEMORY";
				break;
			default:
				error_string = "UNKNOWN";
				break;
			}

			char error_msg[256] = { 0 };
			sprintf_s(error_msg, sizeof(error_msg), "OpenGL Error(%s): %s\n", error_string.c_str(), name);
			throw std::runtime_error(std::string(error_msg));
		}

		return glerror == GL_NO_ERROR;
	}
}