#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include "he_util.hpp"
#include "graphics/he_texture.hpp"
#include "graphics/he_mesh.hpp"
#include "graphics/he_shader.hpp"

struct ResourceManager final {
	std::unordered_map<std::shared_ptr<hyperengine::Texture>, int> mTexturesAsserted;
	hyperengine::UnorderedStringMap<std::weak_ptr<hyperengine::Mesh>> mMeshes;
	hyperengine::UnorderedStringMap<std::weak_ptr<hyperengine::Texture>> mTextures;
	hyperengine::UnorderedStringMap<std::weak_ptr<hyperengine::ShaderProgram>> mShaders;

	void update();
	std::shared_ptr<hyperengine::Texture> assertTextureLifetime(std::shared_ptr<hyperengine::Texture> const& texture, int frames = 360);
	std::shared_ptr<hyperengine::Mesh> getMesh(std::string_view path);
	std::shared_ptr<hyperengine::Texture> getTexture(std::string_view path);
	void reloadShader(std::string const& pathStr, hyperengine::ShaderProgram& program, std::unordered_map<std::u8string, std::string>& fileErrors);
	std::shared_ptr<hyperengine::ShaderProgram> getShaderProgram(std::string_view path, std::unordered_map<std::u8string, std::string>& fileErrors);
};