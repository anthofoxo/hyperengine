#pragma once

#include <string>
#include <vector>

namespace hyperengine {
	bool isWsl();
	bool isDebuggerPresent();
	void breakpoint();
	void breakpointIfDebugging();

	std::vector<std::string> getEnvironmentPaths();
}