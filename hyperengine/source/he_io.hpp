#include <optional>
#include <string>
#include <vector>

namespace hyperengine {
	std::optional<std::string> readFileString(char const* path);
	std::optional<std::vector<char>> readFileBinary(char const* path);
}