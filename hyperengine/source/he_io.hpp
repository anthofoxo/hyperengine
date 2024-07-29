#include <optional>
#include <string>
#include <vector>

#include "he_mesh.hpp"
#include "he_texture.hpp"

namespace hyperengine {
	std::optional<std::string> readFileString(char const* path);
	std::optional<std::vector<char>> readFileBinary(char const* path);
	std::optional<hyperengine::Mesh> readMesh(char const* path);
	std::optional<hyperengine::Texture> readTextureImage(char const* filepath);
}