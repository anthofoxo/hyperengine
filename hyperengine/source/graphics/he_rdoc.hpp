#pragma once

namespace hyperengine::rdoc {
	void setup(bool load);
	bool isRunning();
	bool isTargetControlConnected();
	void triggerCapture();
	bool isFrameCapturing();
}