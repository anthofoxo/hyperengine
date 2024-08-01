#if defined __has_include && __has_include(<debugging>)
#	include <debugging>
#	if __cpp_lib_debugging >= 202311L
#		define HE_IMPL_USING_STD_DEBUGGING
#	endif
#endif

#ifndef HE_IMPL_USING_STD_DEBUGGING
#	include <debug_trap.h>
#endif

#ifdef _WIN32
#	include "he_platform_win32.inl"
#else
#	include "he_platform_unix.inl"
#endif

namespace hyperengine {
#ifdef HE_IMPL_USING_STD_DEBUGGING
	bool isDebuggerPresent() {
		return std::is_debugger_present();
	}

	void breakpoint() {
		std::breakpoint();
	}

	void breakpointIfDebugging() {
		std::breakpoint_if_debugging();
	}
#else
	void breakpoint() {
		psnip_trap();
	}

	void breakpointIfDebugging() {
		if (isDebuggerPresent())
			breakpoint();
	}
#endif
}

