#include "PacketListPanel.h"
#include <imgui.h>

PacketListPanel::PacketListPanel(std::shared_ptr<CaptureEngine> engine)
	: captureEngine(std::move(engine))
{
}

void PacketListPanel::Render()
{
	ImGui::SeparatorText("Captured Packets");

	const auto packets = captureEngine->GetRecentPackets();

	if (packets.empty())
	{
		ImGui::Text("No packets yet.");
		return;
	}

	if (ImGui::BeginTable("PacketsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 400)))
	{
		ImGui::TableSetupScrollFreeze(0, 1); // Header row always visible
		ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupColumn("Protocol", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Destination", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Length", ImGuiTableColumnFlags_WidthFixed, 60.0f);
		ImGui::TableHeadersRow();

		for (const auto& pkt : packets)
		{
			ImGui::TableNextRow();

			// Format timestamp
			auto timeT = std::chrono::system_clock::to_time_t(pkt.timestamp);
			std::tm tm;

#ifdef _WIN32
			localtime_s(&tm, &timeT);
#else
			localtime_r(&timeT, &tm);
#endif
			std::ostringstream timeStr;
			timeStr << std::put_time(&tm, "%H:%M:%S");

			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(timeStr.str().c_str());

			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(pkt.protocol.c_str());

			ImGui::TableSetColumnIndex(2);
			ImGui::TextUnformatted(pkt.srcAddr.c_str());

			ImGui::TableSetColumnIndex(3);
			ImGui::TextUnformatted(pkt.dstAddr.c_str());

			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%u", pkt.length);
		}

		ImGui::EndTable();
	}
}