#include "he_audio.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace hyperengine {
	void AudioEngine::init(std::string_view preferredDevice) {
		if (ma_context_init(nullptr, 0, nullptr, &mContext) != MA_SUCCESS) {
			// Error.
		}

		ma_device_info* pPlaybackInfos;
		ma_uint32 playbackCount;
		if (ma_context_get_devices(&mContext, &pPlaybackInfos, &playbackCount, nullptr, nullptr) != MA_SUCCESS) {
			// Error.
		}

		bool isDeviceChosen = false;
		ma_uint32 chosenPlaybackDeviceIndex = 0;

		for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1) {
			if (std::string_view(pPlaybackInfos[iDevice].name) == preferredDevice) {
				chosenPlaybackDeviceIndex = iDevice;
				isDeviceChosen = true;
			}
		}

		if (!isDeviceChosen) {
			for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1) {
				if (pPlaybackInfos[iDevice].isDefault) {
					chosenPlaybackDeviceIndex = iDevice;
					isDeviceChosen = true;
					break;
				}
			}
		}

		mDeviceName = pPlaybackInfos[chosenPlaybackDeviceIndex].name;

		ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
		deviceConfig.playback.pDeviceID = &pPlaybackInfos[chosenPlaybackDeviceIndex].id;
		deviceConfig.pUserData = &mEngine;

		deviceConfig.dataCallback = [](ma_device* pDevice, void* pOutput, void const* pInput, ma_uint32 frameCount) {
			ma_engine_read_pcm_frames(static_cast<ma_engine*>(pDevice->pUserData), pOutput, frameCount, nullptr);
			};

		if (ma_device_init(&mContext, &deviceConfig, &mDevice) != MA_SUCCESS) {
			// Error.
		}

		ma_engine_config engineConfig = ma_engine_config_init();
		engineConfig.pDevice = &mDevice;

		if (ma_engine_init(&engineConfig, &mEngine) != MA_SUCCESS) {
			// Error.
		}
	}

	void AudioEngine::uninit() {
		ma_engine_uninit(&mEngine);
		ma_device_uninit(&mDevice);
		ma_context_uninit(&mContext);
	}
}