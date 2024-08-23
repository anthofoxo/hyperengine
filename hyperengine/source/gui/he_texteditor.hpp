#pragma once

#include <TextEditor.h>

namespace hyperengine::gui {
	struct TextEditor final {
		::TextEditor editor;
		int undoIndexInDisc = 0;
	};

	void spawnTextEditor(std::string const& filepath, std::string const& extension);
	void drawTextEditors(ImFont* monoFont, bool* pUpdateFilesystems);
}