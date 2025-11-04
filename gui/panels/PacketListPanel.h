#pragma once
#include <memory>
#include "core/CaptureEngine.h"

class PacketListPanel
{
public:
	explicit PacketListPanel(std::shared_ptr<CaptureEngine> engine);
	void Render();

private:
	std::shared_ptr<CaptureEngine> captureEngine;
};