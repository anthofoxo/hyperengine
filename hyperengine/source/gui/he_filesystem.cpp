#include "he_filesystem.hpp"

#include "he_io.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include "graphics/he_texture.hpp"

#include "he_texteditor.hpp"

namespace hyperengine::gui {
	void Filesystem::queueUpdate() {
		mAwaitingUpdate = true;
	}

	void Filesystem::draw(bool* pOpen, ResourceManager& resourceManager) {
		if (*pOpen == false) return;

		constexpr float targetThumbnailSize = 64.0f;
		constexpr float padding = 16.0f;

		if (ImGui::Begin("Filesystem", pOpen)) {
			// Update directory caches
			if (std::filesystem::last_write_time(mBrowsingDirectory).time_since_epoch() > mLastTime)
				mAwaitingUpdate = true;

			if (mAwaitingUpdate) {
				spdlog::trace("Filesystem now rooted at {}", (char const*)mBrowsingDirectory.generic_u8string().c_str());
				mAwaitingUpdate = false;
				mLastTime = std::filesystem::last_write_time(mBrowsingDirectory).time_since_epoch();

				mDirectories.clear();
				mFiles.clear();
				for (auto const& entry : std::filesystem::directory_iterator(mBrowsingDirectory)) {
					if (entry.is_directory())
						mDirectories.emplace(entry.path().filename().generic_u8string());
					else
						mFiles.emplace(entry.path());
				}
			}

			// Draw gui
			if (ImGui::BeginTable("FileSystemTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {

				// Present directory options
				ImGui::TableNextColumn();
				if (mBrowsingDirectory != "." && ImGui::Button("..")) {
					mBrowsingDirectory = mBrowsingDirectory.parent_path();
					mAwaitingUpdate = true;
				}
				for (auto const& directory : mDirectories) {
					if (ImGui::Button((char const*)directory.c_str())) {
						mBrowsingDirectory /= directory;
						mAwaitingUpdate = true;
					}
				}

				ImGui::TableNextColumn();

				// Calculate column count
				float availWidth = ImGui::GetContentRegionAvail().x;
				int colCount = glm::max(static_cast<int>(availWidth / (targetThumbnailSize + padding)), 1);

				// Current browsing path
				ImGui::TextUnformatted((char const*)mBrowsingDirectory.generic_u8string().c_str());
				ImGui::Separator();

				std::shared_ptr<hyperengine::Texture> defaultTex = resourceManager.assertTextureLifetime(resourceManager.getTexture("icons/file.png"));

				if (ImGui::BeginTable("FileSystemFileTable", colCount, ImGuiTableFlags_ScrollY)) {
					for (auto const& file : mFiles) {
						ImGui::PushID(&file);

						std::u8string path = file.generic_u8string();
						std::u8string string = file.filename().generic_u8string();
						std::u8string extension = file.extension().generic_u8string();

						// Default
						auto texture = defaultTex;

						if (extension == u8".png") {
							auto tex = resourceManager.getTexture((char const*)path.c_str() + 2); // ignore the "./" prefix
							resourceManager.assertTextureLifetime(tex);
							if (tex) texture = tex;
						}

						ImGui::TableNextColumn();

						ImGui::PushStyleColor(ImGuiCol_Button, { 1.0f, 1.0f, 1.0f, 0.0f });
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 1.0f, 1.0f, 1.0f, 0.4f });
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 1.0f, 1.0f, 1.0f, 0.2f });
						if (ImGui::ImageButton((ImTextureID)(uintptr_t)texture->handle(), { targetThumbnailSize, targetThumbnailSize }, { 0, 1 }, { 1, 0 })) {
							std::string target = std::string((char const*)path.c_str() + 2, path.size() - 2);
							std::string stdextension = std::string((char const*)extension.c_str(), extension.size());
							hyperengine::gui::spawnTextEditor(target, stdextension);
						}
						ImGui::PopStyleColor(3);

						if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
							ImGui::SetDragDropPayload("FilesystemFile", path.c_str() + 2, path.size() - 2);

							ImGui::Image((ImTextureID)(uintptr_t)texture->handle(), { targetThumbnailSize, targetThumbnailSize }, { 0, 1 }, { 1, 0 });
							ImGui::TextUnformatted((char const*)path.c_str() + 2);

							ImGui::EndDragDropSource();
						}

						ImGui::TextUnformatted((char const*)string.c_str());

						ImGui::PopID();
					}
					ImGui::EndTable();
				}

				ImGui::EndTable();
			}
		}
		ImGui::End();

		constexpr std::string_view kInternalTextureBlackName = "internal://black.png";
		constexpr std::string_view kInternalTextureWhiteName = "internal://white.png";
		constexpr std::string_view kInternalTextureUvName = "internal://uv.png";
		constexpr std::string_view kInternalTextureCheckerboardName = "internal://checkerboard.png";
		constexpr std::array<std::string_view, 4> kInternalTextures = { kInternalTextureBlackName, kInternalTextureWhiteName, kInternalTextureUvName, kInternalTextureCheckerboardName };


		if (ImGui::Begin("Internal Files", pOpen)) {
			float availWidth = ImGui::GetContentRegionAvail().x;
			int colCount = glm::max(static_cast<int>(availWidth / (targetThumbnailSize + padding)), 1);

			if (ImGui::BeginTable("FileSystemFileTable", colCount, ImGuiTableFlags_ScrollY)) {
				for (auto const& file : kInternalTextures) {
					ImGui::PushID(&file);

					auto tex = resourceManager.getTexture(file);

					ImGui::TableNextColumn();

					ImGui::PushStyleColor(ImGuiCol_Button, { 1.0f, 1.0f, 1.0f, 0.0f });
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 1.0f, 1.0f, 1.0f, 0.4f });
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 1.0f, 1.0f, 1.0f, 0.2f });
					ImGui::ImageButton((ImTextureID)(uintptr_t)tex->handle(), { targetThumbnailSize, targetThumbnailSize }, { 0, 1 }, { 1, 0 });
					ImGui::PopStyleColor(3);

					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						ImGui::SetDragDropPayload("FilesystemFile", file.data(), file.size());

						ImGui::Image((ImTextureID)(uintptr_t)tex->handle(), { targetThumbnailSize, targetThumbnailSize }, { 0, 1 }, { 1, 0 });
						ImGui::TextUnformatted((char const*)file.data());

						ImGui::EndDragDropSource();
					}

					ImGui::TextUnformatted((char const*)file.data());

					ImGui::PopID();
				}
				ImGui::EndTable();
			}

		}
		ImGui::End();
	}
}