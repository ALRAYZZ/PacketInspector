#pragma once
#include <memory>
#include <string>
#include <vector>
#include "core/CaptureEngine.h"


class CaptureControlPanel
{
public:
	explicit CaptureControlPanel(std::shared_ptr<CaptureEngine> engine);

	void Render();

private:
	std::shared_ptr<CaptureEngine> captureEngine;
	int selectedDevice;
	std::vector<std::string> devices;
	bool initialized;
};