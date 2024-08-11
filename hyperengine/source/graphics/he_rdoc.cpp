#include "he_rdoc.hpp"

#ifdef _WIN32
#	include <Windows.h>
#	include <shlobj_core.h>
#else
#	include <dlfcn.h>
#endif

#include <format>

#include <renderdoc_app.h>

namespace {
	RENDERDOC_API_1_0_0* gApi = nullptr;

	void loadSharedLibrary(bool load) {
#ifdef _WIN32
		HMODULE library = GetModuleHandleA("renderdoc.dll");
		if (load && library == nullptr) {
			CHAR pf[MAX_PATH];
			SHGetSpecialFolderPathA(nullptr, pf, CSIDL_PROGRAM_FILES, false);
			library = LoadLibraryA(std::format("{}/RenderDoc/renderdoc.dll", pf).c_str());
		}
		if (library == nullptr) return;

		pRENDERDOC_GetAPI getApi = (pRENDERDOC_GetAPI)GetProcAddress(library, "RENDERDOC_GetAPI");
		if (getApi == nullptr) return;
		getApi(eRENDERDOC_API_Version_1_0_0, (void**)&gApi);
#else
		void* library = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
		if (load && library == nullptr) library = dlopen("librenderdoc.so", RTLD_NOW);
		if (library == nullptr) return;

		pRENDERDOC_GetAPI getApi = (pRENDERDOC_GetAPI)dlsym(library, "RENDERDOC_GetAPI");
		if (getApi == nullptr) return;
		getApi(eRENDERDOC_API_Version_1_0_0, (void**)&gApi);
#endif
	}
}

namespace hyperengine {
	void setupRenderDoc(bool load) {
		loadSharedLibrary(load);
		if (!gApi) return;

		gApi->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
		gApi->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 0);
	}

	bool isRenderDocRunning() {
		return gApi;
	}

	namespace rdoc {
		bool isTargetControlConnected() {
			if (!gApi) return false;
			return gApi->IsTargetControlConnected();
		}

		void triggerCapture() {
			if (!gApi) return;
			gApi->TriggerCapture();
		}

		bool isFrameCapturing() {
			if (!gApi) return false;
			return gApi->IsFrameCapturing();
		}
	}
}