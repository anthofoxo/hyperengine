#include "he_io.hpp"

#include <fstream>

namespace hyperengine {
	std::optional<std::string> readFileString(char const* path) {
		std::ifstream file;
		file.open(path, std::ios::in | std::ios::binary);
		if (!file) return std::nullopt;

		std::string content;
		file.seekg(0, std::ios::end);
		auto size = file.tellg();
		content.resize(size);
		file.seekg(0, std::ios::beg);
		file.read(content.data(), content.size());
		file.close();
		return content;
	}

	std::optional<std::vector<char>> readFileBinary(char const* path) {
		std::ifstream file;
		file.open(path, std::ios::in | std::ios::binary);
		if (!file) return std::nullopt;

		std::vector<char> content;
		file.seekg(0, std::ios::end);
		auto size = file.tellg();
		content.resize(size);
		file.seekg(0, std::ios::beg);
		file.read(content.data(), content.size());
		file.close();
		return content;
	}
}