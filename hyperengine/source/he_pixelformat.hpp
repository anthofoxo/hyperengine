#pragma once

#include <glad/gl.h>

namespace hyperengine {
	enum struct PixelFormat {
		kRgba8,
		kD24
	};

	GLenum pixelFormatToInternalFormat(PixelFormat format);
	GLenum pixelFormatToFormat(PixelFormat format);
	GLenum pixelFormatToType(PixelFormat format);
}