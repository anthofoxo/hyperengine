#include "he_pixelformat.hpp"

#include <utility>

namespace hyperengine {
	using enum PixelFormat;

	GLenum pixelFormatToInternalFormat(PixelFormat format) {
		switch (format) {
		case kRgba8: return GL_RGBA8;
		case kD24: return GL_DEPTH_COMPONENT24;
		case kRgba32f: return GL_RGBA32F;
		default: std::unreachable();
		}
	}

	GLenum pixelFormatToFormat(PixelFormat format) {
		switch (format) {
		case kRgba32f:
		case kRgba8: return GL_RGBA;
		case kD24: return GL_DEPTH_COMPONENT;
		default: std::unreachable();
		}
	}

	GLenum pixelFormatToType(PixelFormat format) {
		switch (format) {
		case kRgba8: return GL_UNSIGNED_BYTE;
		case kRgba32f:
		case kD24: return GL_FLOAT;
		default: std::unreachable();
		}
	}
}