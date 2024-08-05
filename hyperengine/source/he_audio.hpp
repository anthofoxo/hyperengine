#pragma once

#include <miniaudio.h>
#include <string>

namespace hyperengine {
	struct AudioEngine final {
		void init(std::string_view preferredDevice);
		void uninit();

		std::string mDeviceName;
		ma_context mContext;
		ma_device mDevice;
		ma_engine mEngine;
	};
}