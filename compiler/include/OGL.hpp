#pragma once

#include <glew/glew.h>

namespace gl
{
	typedef GLenum		enumerator;
	typedef GLbitfield	bitfield;
	typedef GLuint		uint32;
	typedef GLint		int32;
	typedef GLsizei		sizei;
	typedef GLboolean	boolean;
	typedef GLbyte		byte;
	typedef GLshort		int16;
	typedef GLubyte		ubyte;
	typedef GLushort	uint16;
	typedef GLfloat		float32;
	typedef GLclampf	clampf32;
	typedef GLdouble	float64;
	typedef GLclampd	clampf64;
	typedef GLvoid		ptr;
	typedef GLint64		int64;
	typedef GLuint64	uint64;
	typedef GLsync		sync;
	typedef GLchar		character;

	inline const gl::ptr* bufferOffset(size_t pointer)
	{
		return reinterpret_cast<const gl::ptr*>(pointer);
	}

} // namespace gl
