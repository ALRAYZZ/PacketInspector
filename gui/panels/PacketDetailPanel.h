#pragma once
#include <functional>
#include <optional>
#include "core/PacketInfo.h"



class PacketDetailPanel
{
public:
	explicit PacketDetailPanel(std::function<std::optional<PacketInfo>()> selectedGetter);

	void Render();

private:
	std::function<std::optional<PacketInfo>()> getSelectedPacket;

	void RenderHeaderInfo(const PacketInfo& pkt);
	void RenderHexDump(const PacketInfo& pkt, size_t bytesPerLine = 16);
};