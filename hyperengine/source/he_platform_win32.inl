#include <Windows.h>

#include "he_util.hpp" // split

namespace hyperengine {
	bool isWsl() { return false; }

#ifndef HE_IMPL_USING_STD_DEBUGGING
	bool isDebuggerPresent() {
		return ::IsDebuggerPresent();
	}
#endif

	std::vector<std::string> getEnvironmentPaths() {
		std::vector<std::string> pathList;
		char* value = nullptr;
		size_t len = 0;
		if (_dupenv_s(&value, &len, "PATH")) return {};
		if (value == nullptr) return {};
		std::string paths = std::string(value, len);
		free(value);
		split(paths, pathList, ';');
		return pathList;
	}
}

#ifdef HE_ENTRY_WINMAIN
#include <stdlib.h> // __argc, __argv

extern int main(int, char* []);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	return main(__argc, __argv);
}
#endif