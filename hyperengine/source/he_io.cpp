#include "he_io.hpp"

#include <fstream>
#include <iostream>
#include <array>

#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

	void writeFile(char const* path, void const* data, size_t size) {
		std::ofstream file(path, std::ofstream::out | std::ofstream::binary);
		file.write(static_cast<char const*>(data), size);
	}

	std::optional<hyperengine::Mesh> readMesh(char const* path) {
		struct Vertex final {
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec2 uv;
		};

		static_assert(sizeof(aiVector3D) == sizeof(glm::vec3));

		Assimp::Importer import;
		aiScene const* scene = import.ReadFile(path, aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_RemoveComponent | aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes | aiProcess_ImproveCacheLocality | aiProcess_FixInfacingNormals);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cerr << import.GetErrorString() << '\n';
			return std::nullopt;
		}

		if (scene->mNumMeshes != 1) {
			std::cerr << "Only one mesh can be exported currently\n";
			return std::nullopt;
		}

		aiMesh* mesh = scene->mMeshes[0];

		std::vector<Vertex> vertices;
		vertices.reserve(mesh->mNumVertices);

		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
			auto& textureCoord = mesh->mTextureCoords[0][i];
			vertices.emplace_back(std::bit_cast<glm::vec3>(mesh->mVertices[i]), std::bit_cast<glm::vec3>(mesh->mNormals[i]), glm::vec2(textureCoord.x, textureCoord.y));
		}

		unsigned char elementPrimitiveWidth;
		if (mesh->mNumVertices <= std::numeric_limits<uint8_t>::max()) elementPrimitiveWidth = sizeof(uint8_t);
		else if (mesh->mNumVertices <= std::numeric_limits<uint16_t>::max()) elementPrimitiveWidth = sizeof(uint16_t);
		else elementPrimitiveWidth = sizeof(uint32_t);

		std::vector<char> elements;
		elements.reserve(mesh->mNumFaces * 3 * elementPrimitiveWidth);

		GLsizei count = 0;

		for (unsigned int iFace = 0; iFace < mesh->mNumFaces; iFace++) {
			aiFace face = mesh->mFaces[iFace];
			if (face.mNumIndices != 3) continue;

			for (unsigned int iElement = 0; iElement < 3; ++iElement) {
				++count;

				for (unsigned int iReserve = 0; iReserve < elementPrimitiveWidth; ++iReserve)
					elements.push_back(0);

				// This method is okay for little endian systems, should verify for big endian
				uint32_t element = face.mIndices[iElement];
				memcpy(elements.data() + elements.size() - elementPrimitiveWidth, &element, elementPrimitiveWidth);
			}
		}

		std::array<hyperengine::Mesh::Attribute, 3> attributes;
		attributes[0] = { .size = 3, .type = GL_FLOAT, .offset = static_cast<GLuint>(offsetof(Vertex, position)) };
		attributes[1] = { .size = 3, .type = GL_FLOAT, .offset = static_cast<GLuint>(offsetof(Vertex, normal)) };
		attributes[2] = { .size = 2, .type = GL_FLOAT, .offset = static_cast<GLuint>(offsetof(Vertex, uv)) };

		return hyperengine::Mesh{{
				.vertices = std::as_bytes(std::span(vertices)),
				.vertexStride = sizeof(Vertex),
				.elements = std::as_bytes(std::span(elements)),
				.elementStride = elementPrimitiveWidth,
				.attributes = attributes,
				.origin = path
			}};
	}

	std::optional<hyperengine::Texture> readTextureImage(char const* filepath) {
		stbi_set_flip_vertically_on_load(true);

		hyperengine::Texture texture;

		int x, y;
		stbi_uc* pixels = stbi_load(filepath, &x, &y, nullptr, 4);

		if (!pixels) return std::nullopt;

		texture = {{
				.width = x,
				.height = y,
				.format = hyperengine::PixelFormat::kRgba8,
				.minFilter = GL_LINEAR,
				.magFilter = GL_LINEAR,
				.wrap = GL_REPEAT,
				.label = filepath,
				.origin = filepath
			}};

		texture.upload({
				.xoffset = 0,
				.yoffset = 0,
				.width = x,
				.height = y,
				.format = hyperengine::PixelFormat::kRgba8,
				.pixels = pixels
			});

		stbi_image_free(pixels);

		return texture;
	}
}