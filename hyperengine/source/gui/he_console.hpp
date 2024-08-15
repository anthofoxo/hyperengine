#pragma once

#include <vector>
#include <string>
#include <string_view>

#include <imgui.h>

namespace hyperengine {
	class Console final {
	public:
		Console();

		void clearLog();
		void addLog(std::string_view string, ImVec4 color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
		void draw(bool* pOpen);
	private:
		struct Log final {
			std::string string;
			ImVec4 color;
		};

		std::vector<Log> mItems;
		bool mAutoScroll = true;
		bool mScrollToBottom = true;
	};

	Console& getConsole();
}