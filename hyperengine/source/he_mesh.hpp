#include <span>
#include <glad/gl.h>

namespace hyperengine {
	class Mesh final {
	public:
		struct Attribute final {
			GLint size;
			GLenum type;
			GLuint offset;
		};

		struct CreateInfo final {
			std::span<const std::byte> vertices;
			GLsizei vertexStride;
			std::span<const std::byte> elements;
			size_t elementStride;
			std::span<const Attribute> attributes;
		};

		constexpr Mesh() noexcept = default;
		Mesh(CreateInfo const& info);
		Mesh(Mesh const&) = delete;
		Mesh& operator=(Mesh const&) = delete;
		inline Mesh(Mesh&& other) noexcept { *this = std::move(other); }
		Mesh& operator=(Mesh&& other) noexcept;
		~Mesh() noexcept;

		void draw();
	private:
		GLuint mVao = 0, mVbo = 0, mEbo = 0;
		GLsizei mCount = 0;
		GLenum mType = 0;
	};
}