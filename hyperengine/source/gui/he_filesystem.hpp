#pragma once

#include <string>
#include <set>
#include <chrono>
#include <filesystem>

#include "he_resourcemanager.hpp"

namespace hyperengine::gui {
	class Filesystem final {
	public:
		void draw(bool* pOpen, ResourceManager& resourceManager);
	private:
		std::set<std::u8string> mDirectories;
		std::set<std::filesystem::path> mFiles;
		std::chrono::system_clock::duration mLastTime = std::chrono::system_clock::now().time_since_epoch();
		std::filesystem::path mBrowsingDirectory = ".";
		bool mAwaitingUpdate = true;
	};
}