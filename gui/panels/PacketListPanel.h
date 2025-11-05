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

	// Track selected flow for detail view
	std::string selectedFlowKey;
	bool showGroupedView = true;

	void RenderGroupedView();
	void RenderDetailView();
	void RenderAllPacketsView();
};