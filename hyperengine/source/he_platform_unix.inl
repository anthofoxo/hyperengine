#include <string_view>
#include <cstdio>

namespace hyperengine {
	bool isWsl() {
		FILE* fp = popen("/bin/uname -r", "r");
		char output[64];

		if (!fp) {
			printf("Failed to run uname\n");
			return false;
		}

		if (!fgets(output, sizeof(output), fp))
			return false;

		std::string_view result = output;
		return (result.find("microsoft") != std::string_view::npos) && (result.find("WSL2") != std::string_view::npos);
	}
}