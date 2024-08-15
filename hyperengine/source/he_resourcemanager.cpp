#include "he_resourcemanager.hpp"

#include <set>
#include "he_io.hpp"

void ResourceManager::update() {
	for (auto it = mTexturesAsserted.begin(); it != mTexturesAsserted.end();) {
		if (--it->second == 0) {
			it = mTexturesAsserted.erase(it);
		}
		else
			++it;
	}

	for (auto it = mTextures.begin(); it != mTextures.end();) {
		if (it->second.expired()) {
			it = mTextures.erase(it);
		}
		else
			++it;
	}

	for (auto it = mMeshes.begin(); it != mMeshes.end();) {
		if (it->second.expired()) {
			it = mMeshes.erase(it);
		}
		else
			++it;
	}

	for (auto it = mShaders.begin(); it != mShaders.end();) {
		if (it->second.expired()) {
			it = mShaders.erase(it);
		}
		else
			++it;
	}
}

// Enforces the provided tetxure to stay for the next x frames
std::shared_ptr<hyperengine::Texture> ResourceManager::assertTextureLifetime(std::shared_ptr<hyperengine::Texture> const& texture, int frames) {
	if (frames > mTexturesAsserted[texture]) mTexturesAsserted[texture] = frames;
	return texture;
}

std::shared_ptr<hyperengine::Mesh> ResourceManager::getMesh(std::string_view path) {
	auto it = mMeshes.find(path);

	if (it != mMeshes.end())
		if (std::shared_ptr<hyperengine::Mesh> ptr = it->second.lock())
			return ptr;

	std::string pathStr(path);

	auto optMesh = hyperengine::readMesh(pathStr.c_str());
	if (!optMesh.has_value()) return nullptr;

	std::shared_ptr<hyperengine::Mesh> mesh = std::make_shared<hyperengine::Mesh>(std::move(optMesh.value()));
	mMeshes[std::move(pathStr)] = mesh;
	return mesh;
}

std::shared_ptr<hyperengine::Texture> ResourceManager::getTexture(std::string_view path) {
	auto it = mTextures.find(path);

	if (it != mTextures.end())
		if (std::shared_ptr<hyperengine::Texture> ptr = it->second.lock())
			return ptr;

	std::string pathStr(path);

	auto opt = hyperengine::readTextureImage(pathStr.c_str());
	if (!opt.has_value()) return nullptr;

	std::shared_ptr<hyperengine::Texture> texture = std::make_shared<hyperengine::Texture>(std::move(opt.value()));
	mTextures[std::move(pathStr)] = texture;
	return texture;
}

void ResourceManager::reloadShader(std::string const& pathStr, hyperengine::ShaderProgram& program, std::unordered_map<std::u8string, std::string>& fileErrors) {
	auto shader = hyperengine::readFileString(pathStr.c_str());
	if (!shader.has_value()) return;

	// reload shader, will repopulate errors
	program = {{ .source = shader.value(), .origin = pathStr }};

	if (!program.errors().empty()) {
		std::string errorTotal;
		for (auto const& error : program.errors()) {
			errorTotal += error + "\n";
		}
		fileErrors[std::u8string((char8_t const*)pathStr.c_str())] = errorTotal;
	}
}

std::shared_ptr<hyperengine::ShaderProgram> ResourceManager::getShaderProgram(std::string_view path, std::unordered_map<std::u8string, std::string>& fileErrors) {
	auto it = mShaders.find(path);

	if (it != mShaders.end())
		if (std::shared_ptr<hyperengine::ShaderProgram> ptr = it->second.lock())
			return ptr;

	std::string pathStr(path);

	hyperengine::ShaderProgram program;
	reloadShader(pathStr, program, fileErrors);

	std::shared_ptr<hyperengine::ShaderProgram> obj = std::make_shared<hyperengine::ShaderProgram>(std::move(program));
	mShaders[std::move(pathStr)] = obj;
	return obj;
}