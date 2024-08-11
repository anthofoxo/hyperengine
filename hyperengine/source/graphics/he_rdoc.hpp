#pragma once

namespace hyperengine {
	void setupRenderDoc(bool load);
	bool isRenderDocRunning();

	namespace rdoc {
		bool isTargetControlConnected();
		void triggerCapture();
		bool isFrameCapturing();
	}
}