#include <string_view>
#include <cstdio>

#include <sys/utsname.h>

namespace hyperengine {
	bool isWsl() {
		utsname uname_info;
		uname(&uname_info);

		std::string_view result = uname_info.release;
		return (result.find("microsoft") != std::string_view::npos) && (result.find("WSL2") != std::string_view::npos);
	}
}
