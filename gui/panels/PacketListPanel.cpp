#include "PacketListPanel.h"
#include <imgui.h>

PacketListPanel::PacketListPanel(std::shared_ptr<CaptureEngine> engine)
	: captureEngine(std::move(engine))
{
}

void PacketListPanel::Render()
{
	ImGui::SeparatorText("Captured Packets");

	auto packets = captureEngine->GetRecentPackets();

	if (packets.empty())
	{
		ImGui::Text("No packets yet.");
		return;
	}

	ImGui::BeginChild("PacketTable", ImVec2(0, 300), true, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::Columns(5, "packets");
	ImGui::Text("Time"); ImGui::NextColumn();
	ImGui::Text("Source"); ImGui::NextColumn();
	ImGui::Text("Destination"); ImGui::NextColumn();
	ImGui::Text("Protocol"); ImGui::NextColumn();
	ImGui::Text("Length"); ImGui::NextColumn();
	ImGui::Separator();

	for (const auto& pkt : packets)
	{
		ImGui::Text("%s", pkt.GetTimeString().c_str()); ImGui::NextColumn();

		std::string src = pkt.srcAddr.empty() ? "-" : pkt.srcAddr;
		if (pkt.srcPort) src += ":" + std::to_string(pkt.srcPort);
		ImGui::Text("%s", src.c_str()); ImGui::NextColumn();

		std::string dst = pkt.dstAddr.empty() ? "-" : pkt.dstAddr;
		if (pkt.dstPort) dst += ":" + std::to_string(pkt.dstPort);
		ImGui::Text("%s", dst.c_str()); ImGui::NextColumn();

		ImGui::Text("%s", pkt.protocol.c_str()); ImGui::NextColumn();
		ImGui::Text("%u", pkt.length); ImGui::NextColumn();
	}

	ImGui::Columns(1);
	ImGui::EndChild();
}