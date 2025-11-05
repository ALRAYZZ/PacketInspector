#include "PacketListPanel.h"
#include <imgui.h>

PacketListPanel::PacketListPanel(std::shared_ptr<CaptureEngine> engine)
	: captureEngine(std::move(engine))
{
}

void PacketListPanel::Render()
{
	ImGui::SeparatorText("Captured Packets");

	// Toggle between groupes and all packets view
	ImGui::Checkbox("Group by Flow", &showGroupedView);
	ImGui::SameLine();
	if (ImGui::Button("Clear Selection"))
	{
		selectedFlowKey.clear();
	}

	if (showGroupedView)
	{
		RenderGroupedView();
	}
	else
	{
		RenderAllPacketsView();
	}
}

void PacketListPanel::RenderGroupedView()
{
	ImGui::BeginChild("GroupedPackets", ImVec2(0, 300), true);

	// Get grouped incoming packets
	auto incomingGroups = captureEngine->GetGroupedPackets(true);

	ImGui::Text("Incoming Flows (%zu)", incomingGroups.size());
	ImGui::Separator();

	if (ImGui::BeginTable("IncomingFlowsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Source Address", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Source Port", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Packet Count", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupColumn("Protocol", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableHeadersRow();

		for (const auto& [key, packets] : incomingGroups)
		{
			const auto& [srcAddr, srcPort] = key;

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			// Create unique ID for this flow
			std::string flowKey = srcAddr + ":" + std::to_string(srcPort);

			// Make row selectable
			if (ImGui::Selectable(srcAddr.c_str(), selectedFlowKey == flowKey, ImGuiSelectableFlags_SpanAllColumns))
			{
				selectedFlowKey = flowKey;
			}

			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%u", srcPort);

			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%zu", packets.size());

			ImGui::TableSetColumnIndex(3);
			if (!packets.empty())
			{
				ImGui::TextUnformatted(packets[0].protocol.c_str());
			}
		}

		ImGui::EndTable();
	}
	ImGui::EndChild();

	// Show outgoing flows separately
	ImGui::Spacing();
	ImGui::Text("Outgoing Packets");
	ImGui::Separator();

	auto outgoingGroups = captureEngine->GetGroupedPackets(false);

	if (ImGui::BeginTable("OutgoingFlowsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 150)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Destination", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Destination Port", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Packet Count", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupColumn("Protocol", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableHeadersRow();

		for (const auto& [key, packets] : outgoingGroups)
		{
			const auto& [dstAddr, dstPort] = key;

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(dstAddr.c_str());

			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%u", dstPort);

			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%zu packets", packets.size());

			ImGui::TableSetColumnIndex(3);
			if (!packets.empty())
			{
				ImGui::TextUnformatted(packets[0].protocol.c_str());
			}
		}
		ImGui::EndTable();
	}

	// Show detail view for selected flow
	if (!selectedFlowKey.empty())
	{
		ImGui::Spacing();
		RenderDetailView();
	}
}

void PacketListPanel::RenderDetailView()
{
	ImGui::SeparatorText(("Flow Details: " + selectedFlowKey).c_str());

	auto incomingGroups = captureEngine->GetGroupedPackets(true);

	// Find the selected flow
	for (const auto& [key, packets] : incomingGroups)
	{
		const auto& [srcAddr, srcPort] = key;
		std::string flowKey = srcAddr + ":" + std::to_string(srcPort);

		if (flowKey != selectedFlowKey) continue;

		if (ImGui::BeginTable("FlowDetailsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 200)))
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
				ImGui::Text("%s:%u", pkt.srcAddr.c_str(), pkt.srcPort);

				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%s:%u", pkt.dstAddr.c_str(), pkt.dstPort);

				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%u", pkt.length);
			}

			ImGui::EndTable();
		}
		break;
	}
}

void PacketListPanel::RenderAllPacketsView()
{
	const auto packets = captureEngine->GetRecentPackets();

	if (packets.empty())
	{
		ImGui::Text("No packets captured.");
		return;
	}

	if (ImGui::BeginTable("PacketsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 400)))
	{
		ImGui::TableSetupScrollFreeze(0, 1); // Header row always visible
		ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupColumn("Direction", ImGuiTableColumnFlags_WidthFixed, 70.0f);
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
			ImGui::TextColored(pkt.incoming ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.8f, 0.2f, 0.2f, 1.0f),
									pkt.incoming ? "IN" : "OUT");

			ImGui::TableSetColumnIndex(2);
			ImGui::TextUnformatted(pkt.protocol.c_str());

			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%s:%u", pkt.srcAddr.c_str(), pkt.srcPort);

			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%s:%u", pkt.dstAddr.c_str(), pkt.dstPort);

			ImGui::TableSetColumnIndex(5);
			ImGui::Text("%u", pkt.length);
		}
	}
}