#include "he_console.hpp"

namespace hyperengine {
	namespace {
		Console gConsole;
	}

	Console& getConsole() {
		return gConsole;
	}

	Console::Console() {
	}

	void Console::clearLog() {
		mItems.clear();
	}

	void Console::addLog(std::string_view string, ImVec4 color) {
		mItems.emplace_back(std::string(string), color);
	}

	void Console::draw(bool* pOpen) {
		if (*pOpen == false) return;

		ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Console", pOpen)) {
			ImGui::End();
			return;
		}

		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::MenuItem("Close Console"))
				*pOpen = false;
			ImGui::EndPopup();
		}

		if (ImGui::SmallButton("Clear")) clearLog();

		ImGui::SameLine();

		// Options menu
		if (ImGui::BeginPopup("Options")) {
			ImGui::Checkbox("Auto-scroll", &mAutoScroll);
			ImGui::EndPopup();
		}

		// Options, Filter
		ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_Tooltip);
		if (ImGui::SmallButton("Options"))
			ImGui::OpenPopup("Options");
		ImGui::Separator();

		// Reserve enough left-over height for 1 separator + 1 input text
		const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
		if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar))
		{
			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::Selectable("Clear")) clearLog();
				ImGui::EndPopup();
			}


			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

			for (auto const& item : mItems) {
				ImGui::PushStyleColor(ImGuiCol_Text, item.color);
				ImGui::TextUnformatted(item.string.c_str());
				ImGui::PopStyleColor();
			}


			// Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
			// Using a scrollbar or mouse-wheel will take away from the bottom edge.
			if (mScrollToBottom || (mAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
				ImGui::SetScrollHereY(1.0f);
			mScrollToBottom = false;

			ImGui::PopStyleVar();
		}
		ImGui::EndChild();

		ImGui::End();
	}
}