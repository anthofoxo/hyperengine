#include <vector>
#include <iostream>
#include <fstream>
#include <cstdint>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// ImHex Pattern Matcher
#if 0
#pragma array_limit 2000000
#pragma pattern_limit 2000000

struct Vertex {
	float data[8];
};

struct HeMesh {
	u8 header[16];
	u32 vertexCount;
	Vertex vertices[vertexCount];
	u32 elementCount;
	match(vertexCount) {
		(0x0000 ... 0x00FF) : u8 elements[elementCount];
		(0x0100 ... 0xFFFF) : u16 elements[elementCount];
		(_) : u32 elements[elementCount];
	}
};
#endif

template<class T>
void writeBytes(std::ostream& stream, T const& t) {
	char bytes[sizeof(T)];
	memcpy(bytes, &t, sizeof(T));
	stream.write(bytes, sizeof(T));
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cout << "Usage: meshconverter [SRC] [DST]\n";
		return 0;
	}

	Assimp::Importer import;
	aiScene const* scene = import.ReadFile(argv[1], aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes | aiProcess_ImproveCacheLocality);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << import.GetErrorString() << '\n';
		return 1;
	}

	if (scene->mNumMeshes != 1) {
		std::cerr << "Only one mesh can be exported currently\n";
	}

	std::ofstream file;
	file.open(argv[2], std::ios::out | std::ios::binary);

	struct Header final {
		unsigned char data[16] = { 'H', 'E', 'M', 'E', 'S', 'H', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	} header;

	writeBytes(file, header);

	aiMesh* mesh = scene->mMeshes[0];

	writeBytes(file, mesh->mNumVertices);

	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
		auto& textureCoord = mesh->mTextureCoords[0][i];
		writeBytes(file, mesh->mVertices[i]);
		writeBytes(file, mesh->mNormals[i]);
		writeBytes(file, textureCoord.x);
		writeBytes(file, textureCoord.y);
	}

	std::vector<unsigned int> elements;
	elements.reserve(mesh->mNumFaces * 3);

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		if (face.mNumIndices != 3) continue;

		elements.push_back(face.mIndices[0]);
		elements.push_back(face.mIndices[1]);
		elements.push_back(face.mIndices[2]);
	}

	writeBytes(file, static_cast<uint32_t>(elements.size()));

	for (unsigned int i : elements) {
		if (mesh->mNumVertices <= std::numeric_limits<uint8_t>::max())
			writeBytes(file, static_cast<uint8_t>(i));
		else if (mesh->mNumVertices <= std::numeric_limits<uint16_t>::max())
			writeBytes(file, static_cast<uint16_t>(i));
		else 
			writeBytes(file, static_cast<uint32_t>(i));
	}

}