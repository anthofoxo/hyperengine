#include <Windows.h>

namespace hyperengine {
	bool isWsl() { return false; }

#ifndef HE_IMPL_USING_STD_DEBUGGING
	bool isDebuggerPresent() {
		return ::IsDebuggerPresent();
	}
#endif
}

#ifdef HE_ENTRY_WINMAIN
#include <stdlib.h> // __argc, __argv

extern int main(int, char* []);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	return main(__argc, __argv);
}
#endif