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
	ImGui::Columns(3, "packets");
	ImGui::Text("Time"); ImGui::NextColumn();
	ImGui::Text("Length"); ImGui::NextColumn();
	ImGui::Text("Data (first bytes)"); ImGui::NextColumn();
	ImGui::Separator();

	for (const auto& pkt : packets)
	{
		ImGui::Text("%s", pkt.GetTimeString().c_str()); ImGui::NextColumn();
		ImGui::Text("%u", pkt.length); ImGui::NextColumn();
		ImGui::Text("%s", pkt.GetHexPreview(16).c_str()); ImGui::NextColumn();
	}

	ImGui::Columns(1);
	ImGui::EndChild();
}