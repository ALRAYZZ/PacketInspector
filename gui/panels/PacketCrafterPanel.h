#pragma once
#include <memory>
#include "core/PacketCrafterEngine.h"

class PacketCrafterPanel
{
public:
	explicit PacketCrafterPanel(std::shared_ptr<PacketCrafterEngine> engine);
	void Render();

private:
	std::shared_ptr<PacketCrafterEngine> crafterEngine;
	PacketCraftConfig config;

	// UI state
	int selectedProtocol = 1; // 0=TCP,1=UDP,2=ICMP
	int selectedPayloadType = 0; // 0=Text,1=Hex,2=Random,3=Pattern,4=Numbers
	char textPayloadBuf[2048] = "Hello, Packet Inspector! made by ALRAYZZ";
	char hexPayloadBuf[4096] = "48656C6C6F20776F726C6421";
	char srcIPBuf[64] = "127.0.0.1";
	char dstIPBuf[64] = "127.0.0.1";
	char srcMACBuf[32] = "00:00:00:00:00:00";
	char dstMACBuf[32] = "FF:FF:FF:FF:FF:FF";

	std::string statusMessage;
	bool statusIsError = false;

	// Rendering sections
	void RenderEthernetSection();
	void RenderIPv4Section();
	void RenderTransportSection();
	void RenderPayloadSection();
	void RenderControlSection();

	// Helpers
	void UpdateConfigFromUI();
	void SyncUIFromConfig();
	void AutoFillLocalMAC();
};