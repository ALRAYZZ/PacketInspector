#pragma once
#include <memory>
#include "core/CaptureEngine.h"
#include <optional>

class PacketListPanel
{
public:
	explicit PacketListPanel(std::shared_ptr<CaptureEngine> engine);
	void Render();
	void RenderDetailView();
	std::optional<PacketInfo> GetSelectedPacket() const;


	std::string selectedFlowKey;
private:
	bool showGroupedView = true;
	std::shared_ptr<CaptureEngine> captureEngine;
	std::optional<PacketInfo> selectedPacket;

	void RenderGroupedView();
	void RenderAllPacketsView();
};