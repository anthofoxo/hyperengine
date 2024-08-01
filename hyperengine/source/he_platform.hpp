#pragma once

namespace hyperengine {
	bool isWsl();
	bool isDebuggerPresent();
	void breakpoint();
	void breakpointIfDebugging();
}