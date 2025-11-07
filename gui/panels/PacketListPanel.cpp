#include "PacketListPanel.h"
#include <imgui.h>

PacketListPanel::PacketListPanel(std::shared_ptr<CaptureEngine> engine)
	: captureEngine(std::move(engine))
{
}

void PacketListPanel::Render()
{
	ImGui::SeparatorText("Captured Packets");

	// Toggle between grouped and all packets view
	ImGui::Checkbox("Group by Flow", &showGroupedView);
	ImGui::SameLine();
	if (ImGui::Button("Clear Selection"))
	{
		selectedFlowKey.clear();
		selectedPacket.reset();
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
	// Get grouped packets
	auto incomingGroups = captureEngine->GetGroupedPackets(true);
	auto outgoingGroups = captureEngine->GetGroupedPackets(false);

	ImVec2 availableRegion = ImGui::GetContentRegionAvail();

	// Incoming Flows - Top Half
	ImGui::Text("Incoming Flows (%zu)", incomingGroups.size());
	ImGui::Separator();

	ImGui::BeginChild("IncomingFlows", ImVec2(0, availableRegion.y * 0.5f - 10), true);

	if (ImGui::BeginTable("IncomingFlowsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Source Address", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Source Port", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Packet Count", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupColumn("Protocol", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableHeadersRow();

		int rowIndex = 0;
		for (const auto& [key, packets] : incomingGroups)
		{
			const auto& [srcAddr, srcPort] = key;

			ImGui::PushID(rowIndex);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			// Create unique ID for this flow with IN: prefix
			std::string flowKey = "IN:" + srcAddr + ":" + std::to_string(srcPort);

			// Make row selectable
			if (ImGui::Selectable(srcAddr.c_str(), selectedFlowKey == flowKey, ImGuiSelectableFlags_SpanAllColumns))
			{
				selectedFlowKey = flowKey;
				selectedPacket.reset(); // Clear selected packet when changing flow
			}

			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%u", srcPort);

			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%zu", packets.size());

			ImGui::TableSetColumnIndex(3);
			if (!packets.empty())
			{
				ImGui::TextUnformatted(packets[0].transportProtocol.c_str());
			}

			ImGui::PopID();
			rowIndex++;
		}

		ImGui::EndTable();
	}
	ImGui::EndChild();

	// Outgoing Flows - Bottom Half
	ImGui::Spacing();
	ImGui::Text("Outgoing Flows (%zu)", outgoingGroups.size());
	ImGui::Separator();

	ImGui::BeginChild("OutgoingFlows", ImVec2(0, 0), true);

	if (ImGui::BeginTable("OutgoingFlowsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Destination", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Dest Port", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Packet Count", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupColumn("Protocol", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableHeadersRow();

		int rowIndex = 0;
		for (const auto& [key, packets] : outgoingGroups)
		{
			const auto& [dstAddr, dstPort] = key;

			ImGui::PushID(rowIndex);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			// Create unique ID for this flow with OUT: prefix
			std::string flowKey = "OUT:" + dstAddr + ":" + std::to_string(dstPort);

			// Make row selectable
			if (ImGui::Selectable(dstAddr.c_str(), selectedFlowKey == flowKey, ImGuiSelectableFlags_SpanAllColumns))
			{
				selectedFlowKey = flowKey;
				selectedPacket.reset(); // Clear selected packet when changing flow
			}

			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%u", dstPort);

			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%zu", packets.size());

			ImGui::TableSetColumnIndex(3);
			if (!packets.empty())
			{
				ImGui::TextUnformatted(packets[0].transportProtocol.c_str());
			}

			ImGui::PopID();
			rowIndex++;
		}

		ImGui::EndTable();
	}
	ImGui::EndChild();
}

void PacketListPanel::RenderDetailView()
{
	// Determine if incoming or outgoing flow
	bool isIncoming = selectedFlowKey.substr(0, 3) == "IN:";
	std::string displayKey = selectedFlowKey.substr(selectedFlowKey.find(':') + 1);

	ImGui::SeparatorText((std::string(isIncoming ? "Incoming" : "Outgoing") + " Flow: " + displayKey).c_str());

	auto groups = captureEngine->GetGroupedPackets(isIncoming);

	// Find the selected flow
	for (const auto& [key, packets] : groups)
	{
		const auto& [addr, port] = key;
		std::string flowKey = (isIncoming ? "IN:" : "OUT:") + addr + ":" + std::to_string(port);

		if (flowKey != selectedFlowKey) continue;

		ImVec2 availableRegion = ImGui::GetContentRegionAvail();

		// Flow details table - takes available space
		ImGui::BeginChild("FlowDetailsChild", ImVec2(0, 0), false);

		if (ImGui::BeginTable("FlowDetailsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
		{
			ImGui::TableSetupScrollFreeze(0, 1); // Header row always visible
			ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("Protocol", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Destination", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Length", ImGuiTableColumnFlags_WidthFixed, 60.0f);
			ImGui::TableHeadersRow();

			int rowIndex = 0;
			for (const auto& pkt : packets)
			{
				ImGui::TableNextRow();

				ImGui::PushID(rowIndex);

				// Format timestamp
				auto timeT = std::chrono::system_clock::to_time_t(pkt.timestamp);
				std::tm tm;

#ifdef _WIN32
				localtime_s(&tm, &timeT);
#else
				localtime_r(&timeT, &tm);
#endif
				char buf[16];
				strftime(buf, sizeof(buf), "%H:%M:%S", &tm);

				// Column 0: Time (selectable)
				ImGui::TableSetColumnIndex(0);
				bool clicked = ImGui::Selectable(buf, selectedPacket.has_value() && selectedPacket->timestamp == pkt.timestamp, ImGuiSelectableFlags_SpanAllColumns);
				if (clicked)
				{
					selectedPacket = pkt;
				}

				// Column 1: Protocol
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(pkt.transportProtocol.c_str());

				// Column 2: Source
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%s:%u", pkt.srcIP.c_str(), pkt.srcPort);

				// Column 3: Destination
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%s:%u", pkt.dstIP.c_str(), pkt.dstPort);

				// Column 4: Length
				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%u", pkt.length);

				ImGui::PopID();
				rowIndex++;
			}

			ImGui::EndTable();
		}
		ImGui::EndChild();
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
			ImGui::TextUnformatted(pkt.transportProtocol.c_str());

			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%s:%u", pkt.srcIP.c_str(), pkt.srcPort);

			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%s:%u", pkt.dstIP.c_str(), pkt.dstPort);

			ImGui::TableSetColumnIndex(5);
			ImGui::Text("%u", pkt.length);
		}

		ImGui::EndTable();
	}
}

// Returns the currently selected packet, if any
std::optional<PacketInfo> PacketListPanel::GetSelectedPacket() const
{
	return selectedPacket;
}