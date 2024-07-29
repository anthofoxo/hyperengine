#include "he_mesh.hpp"

namespace hyperengine {
	Mesh::Mesh(CreateInfo const& info) {
		if (GLAD_GL_ARB_direct_state_access) {
			glCreateVertexArrays(1, &mVao);

			glCreateBuffers(1, &mVbo);
			glNamedBufferStorage(mVbo, info.vertices.size_bytes(), info.vertices.data(), 0);

			glCreateBuffers(1, &mEbo);
			glNamedBufferStorage(mEbo, info.elements.size_bytes(), info.elements.data(), 0);

			constexpr GLuint bindingIndex = 0;
			glVertexArrayVertexBuffer(mVao, bindingIndex, mVbo, 0, info.vertexStride);
			glVertexArrayElementBuffer(mVao, mEbo);

			for (size_t i = 0; i < info.attributes.size(); ++i) {
				auto const& attribute = info.attributes[i];
				glEnableVertexArrayAttrib(mVao, static_cast<GLuint>(i));
				glVertexArrayAttribFormat(mVao, static_cast<GLuint>(i), attribute.size, attribute.type, GL_FALSE, attribute.offset);
				glVertexArrayAttribBinding(mVao, static_cast<GLuint>(i), bindingIndex);
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

			glGenBuffers(1, &mVbo);
			glBindBuffer(GL_ARRAY_BUFFER, mVbo);
			glBufferData(GL_ARRAY_BUFFER, info.vertices.size() * sizeof(decltype(info.vertices)::value_type), info.vertices.data(), GL_STATIC_DRAW);

			glGenBuffers(1, &mEbo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEbo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, info.elements.size() * sizeof(decltype(info.elements)::value_type), info.elements.data(), GL_STATIC_DRAW);

			for (size_t i = 0; i < info.attributes.size(); ++i) {
				auto const& attribute = info.attributes[i];
				glEnableVertexAttribArray(static_cast<GLuint>(i));
				glVertexAttribPointer(static_cast<GLuint>(i), attribute.size, attribute.type, GL_FALSE, info.vertexStride, (void const*)(uintptr_t)attribute.offset);
			}

			// Pop state
			glBindVertexArray(static_cast<GLuint>(prevVao));
			glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(prevVbo));
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(prevEbo));
			//
		}

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

		mCount = info.elements.size();
	}

	Mesh& Mesh::operator=(Mesh&& other) noexcept {
		std::swap(mVao, other.mVao);
		std::swap(mVbo, other.mVbo);
		std::swap(mEbo, other.mEbo);
		std::swap(mCount, other.mCount);
		std::swap(mType, other.mType);
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

	void Mesh::draw() {
		glBindVertexArray(mVao);
		glDrawElements(GL_TRIANGLES, mCount, mType, nullptr);
	}
}