#pragma once
#include <memory>
#include "core/CaptureEngine.h"
#include <optional>

class PacketListPanel
{
public:
	explicit PacketListPanel(std::shared_ptr<CaptureEngine> engine);
	void Render();

	std::optional<PacketInfo> GetSelectedPacket() const;

	bool showGroupedView = true;
private:
	std::shared_ptr<CaptureEngine> captureEngine;

	// Track selected flow for detail view
	std::string selectedFlowKey;

	std::optional<PacketInfo> selectedPacket;

	void RenderGroupedView();
	void RenderDetailView();
	void RenderAllPacketsView();
};