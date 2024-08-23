#include "he_texteditor.hpp"

#include "he_io.hpp"
#include <imgui.h>
#include <imgui_internal.h>

namespace hyperengine::gui {
	namespace {
		std::unordered_map<std::string, TextEditor> gEditors;
	}

	void spawnTextEditor(std::string const& filepath, std::string const& extension) {
		auto it = gEditors.find(filepath);
		// File is not opened, open it
		if (it != gEditors.end()) return;

		// Unsupported formats
		if (extension == ".png" || extension == ".ttf") return;

		TextEditor editor;
		editor.editor.SetTabSize(2);

		if (extension == ".lua")
			editor.editor.SetLanguageDefinition(::TextEditor::LanguageDefinitionId::Lua);
		else if (extension == ".glsl")
			editor.editor.SetLanguageDefinition(::TextEditor::LanguageDefinitionId::Glsl);

		auto optContent = readFileString((char const*)filepath.c_str());
		if (!optContent)
			return;

		editor.editor.SetText(optContent.value());
		gEditors[filepath] = editor;
	}

	void drawTextEditors(ImFont* monoFont, bool* pUpdateFilesystems) {
		if (gEditors.empty()) return;

		bool opened = true;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
		ImGui::Begin("Text Editors", &opened, ImGuiWindowFlags_MenuBar);
		ImGui::PopStyleVar();

		{
			bool saveCurrentDoc = false;
			bool saveAll = false;

			if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S)) {
				saveCurrentDoc = true;
			}

			if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S)) {
				saveAll = true;
			}

			int line = 0, column = 0, lineCount = 0;
			bool overwriteEnabled = false;
			std::string defName;

			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					if (ImGui::MenuItem("Save", ImGui::GetKeyChordName(ImGuiMod_Ctrl | ImGuiKey_S))) saveCurrentDoc = true;
					if (ImGui::MenuItem("Save All", ImGui::GetKeyChordName(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S))) saveAll = true;

					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			if (ImGui::BeginTabBar("TextEditorsTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {

				std::string toClose;

				for (auto& [k, v] : gEditors) {
					bool opened = true;

					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

					ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;

					if (v.undoIndexInDisc != v.editor.GetUndoIndex())
						flags |= ImGuiTabItemFlags_UnsavedDocument;

					if (ImGui::BeginTabItem((char const*)k.c_str(), &opened, flags)) {

						v.editor.GetCursorPosition(line, column);
						lineCount = v.editor.GetLineCount();
						overwriteEnabled = v.editor.IsOverwriteEnabled();
						defName = v.editor.GetLanguageDefinitionName();

						ImVec2 availContent = ImGui::GetContentRegionAvail();
						availContent.y -= ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.y * 2.0f;

						ImGui::PushFont(monoFont);
						v.editor.Render((char const*)k.c_str(), false, availContent);
						ImGui::PopFont();

						if (saveCurrentDoc) {
							hyperengine::writeFile((char const*)k.c_str(), v.editor.GetText().data(), v.editor.GetText().size());
							*pUpdateFilesystems = true;
							v.undoIndexInDisc = v.editor.GetUndoIndex();
						}

						ImGui::EndTabItem();
					}

					ImGui::PopStyleVar();

					if (!opened)
						toClose = k;

					if (saveAll) {
						hyperengine::writeFile((char const*)k.c_str(), v.editor.GetText().data(), v.editor.GetText().size());
						*pUpdateFilesystems = true;
						v.undoIndexInDisc = v.editor.GetUndoIndex();
					}
				}

				if (!toClose.empty()) {
					gEditors.erase(toClose);
				}

				ImGui::EndTabBar();
			}

			ImGui::Text("%6d/%-6d %6d lines | %s | %s", line + 1, column + 1, lineCount, overwriteEnabled ? "Ovr" : "Ins", defName.c_str());
		}
		ImGui::End();

		// The text editor panel close was requested
		// Close all editors
		if (!opened)
			gEditors.clear();
	}
}