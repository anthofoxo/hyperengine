#include "he_mesh.hpp"

namespace hyperengine {
	Mesh::Mesh(CreateInfo const& info) {
		if (GLAD_GL_ARB_direct_state_access) {
			glCreateVertexArrays(1, &mVao);

			constexpr GLuint bindingIndex = 0;

			if (info.vertices.size_bytes() > 0 && info.attributes.size() > 0) {
				glCreateBuffers(1, &mVbo);
				glNamedBufferStorage(mVbo, info.vertices.size_bytes(), info.vertices.data(), 0);
				glVertexArrayVertexBuffer(mVao, bindingIndex, mVbo, 0, info.vertexStride);
			}

			if (info.elements.size_bytes() > 0) {
				glCreateBuffers(1, &mEbo);
				glNamedBufferStorage(mEbo, info.elements.size_bytes(), info.elements.data(), 0);
				glVertexArrayElementBuffer(mVao, mEbo);
			}

			if (info.vertices.size_bytes() > 0) {
				for (size_t i = 0; i < info.attributes.size(); ++i) {
					auto const& attribute = info.attributes[i];
					glEnableVertexArrayAttrib(mVao, static_cast<GLuint>(i));
					glVertexArrayAttribFormat(mVao, static_cast<GLuint>(i), attribute.size, attribute.type, GL_FALSE, attribute.offset);
					glVertexArrayAttribBinding(mVao, static_cast<GLuint>(i), bindingIndex);
				}
			}
		}
		else {
			// Push state
			GLint prevVao;
			GLint prevVbo;
			GLint prevEbo;
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
			glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevVbo);
			glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prevEbo);
			//

			glGenVertexArrays(1, &mVao);
			glBindVertexArray(mVao);

			if (info.vertices.size_bytes() > 0) {
				glGenBuffers(1, &mVbo);
				glBindBuffer(GL_ARRAY_BUFFER, mVbo);
				glBufferData(GL_ARRAY_BUFFER, info.vertices.size() * sizeof(decltype(info.vertices)::value_type), info.vertices.data(), GL_STATIC_DRAW);
			}

			if (info.elements.size_bytes() > 0) {
				glGenBuffers(1, &mEbo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEbo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, info.elements.size() * sizeof(decltype(info.elements)::value_type), info.elements.data(), GL_STATIC_DRAW);
			}

			if (info.vertices.size_bytes() > 0) {
				for (size_t i = 0; i < info.attributes.size(); ++i) {
					auto const& attribute = info.attributes[i];
					glEnableVertexAttribArray(static_cast<GLuint>(i));
					glVertexAttribPointer(static_cast<GLuint>(i), attribute.size, attribute.type, GL_FALSE, info.vertexStride, (void const*)(uintptr_t)attribute.offset);
				}
			}

			// Pop state
			glBindVertexArray(static_cast<GLuint>(prevVao));
			glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(prevVbo));
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(prevEbo));
			//
		}

		if (info.elements.size_bytes() > 0) {
			switch (info.elementStride) {
			case 1:
				mType = GL_UNSIGNED_BYTE;
				break;
			case 2:
				mType = GL_UNSIGNED_SHORT;
				break;
			default:
				mType = GL_UNSIGNED_INT;
			}
		}
		else {
			mType = 0;
		}


		if (info.elements.size_bytes() > 0) {
			mCount = static_cast<GLsizei>(info.elements.size_bytes() / info.elementStride);
		}
		else if (info.vertices.size_bytes() > 0) {
			mCount = static_cast<GLsizei>(info.vertices.size_bytes() / info.vertexStride);
		}
		else
			mCount = 0;
			
		mOrigin = std::string(info.origin);
	}

	Mesh& Mesh::operator=(Mesh&& other) noexcept {
		std::swap(mVao, other.mVao);
		std::swap(mVbo, other.mVbo);
		std::swap(mEbo, other.mEbo);
		std::swap(mCount, other.mCount);
		std::swap(mType, other.mType);
		std::swap(mOrigin, other.mOrigin);
		return *this;
	}

	Mesh::~Mesh() noexcept {
		if (mVbo)
			glDeleteBuffers(1, &mVbo);
		if (mEbo)
			glDeleteBuffers(1, &mEbo);
		if (mVao)
			glDeleteVertexArrays(1, &mVao);
	}

	void Mesh::draw(GLenum mode, GLint first, GLsizei count) {
		if (count == -1) count = mCount;

		glBindVertexArray(mVao);
		if (mEbo == 0)
			glDrawArrays(mode, first, count);
		else {
			int stride;
			if (mType == GL_UNSIGNED_BYTE) stride = 1;
			else if (mType == GL_UNSIGNED_SHORT) stride = 2;
			else stride = 4;

			glDrawElements(mode, count, mType, (void const*)(uintptr_t)(first * stride));
		}
	}
}