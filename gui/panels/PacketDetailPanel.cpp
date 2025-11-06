#include "PacketDetailPanel.h"
#include <imgui.h>
#include <sstream>
#include <iomanip>

PacketDetailPanel::PacketDetailPanel(std::function<std::optional<PacketInfo>()> selectedGetter)
	: getSelectedPacket(std::move(selectedGetter))
{
}

void PacketDetailPanel::Render()
{
	ImGui::SeparatorText("Packet Details");

	auto opt = getSelectedPacket();
	if (!opt.has_value())
	{
		ImGui::TextUnformatted("Select a packet (n the list or flow) to view details.");
		return;
	}

	const PacketInfo& pkt = *opt;

	ImGui::BeginChild("PacketDetailChild", ImVec2(0, 300), true, ImGuiWindowFlags_HorizontalScrollbar);

	// Header info
	RenderHeaderInfo(pkt);

	ImGui::Separator();
	ImGui::Spacing();

	// Collapsible structured layers
	if (ImGui::CollapsingHeader("Ethernet Layer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BulletText("Source MAC: %s", pkt.etherSrc.c_str());
		ImGui::BulletText("Destination MAC: %s", pkt.etherDst.c_str());
		ImGui::BulletText("EtherType: %s", pkt.etherTypeStr.c_str());
	}

	if (ImGui::CollapsingHeader("IPv4 Layer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BulletText("Source IP: %s", pkt.srcIP.c_str());
		ImGui::BulletText("Destination IP: %s", pkt.dstIP.c_str());
		ImGui::BulletText("TTL: %u", pkt.ttl);
		ImGui::BulletText("Protocol: %s", pkt.transportProtocol.c_str());
	}

	if (ImGui::CollapsingHeader("Transport Layer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BulletText("Transport Protocol: %s", pkt.transportProtocol.c_str());
		ImGui::BulletText("Source Port: %u", pkt.srcPort);
		ImGui::BulletText("Destination Port: %u", pkt.dstPort);
		ImGui::BulletText("Direction: %s", pkt.incoming ? "Incoming" : "Outgoing");
	}

	if (ImGui::CollapsingHeader("Raw Data (Hex / ASCII Dump)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		RenderHexDump(pkt);
	}

	ImGui::EndChild();
}

void PacketDetailPanel::RenderHeaderInfo(const PacketInfo& pkt)
{
	ImGui::Text("Timestamp: %s", pkt.GetTimeString().c_str());
	ImGui::Text("Length: %u bytes", pkt.length);
	ImGui::Spacing();

	ImGui::TextUnformatted("Addresses:");
	ImGui::BulletText("Source: %s:%u", pkt.srcIP.c_str(), pkt.srcPort);
	ImGui::BulletText("Destination: %s:%u", pkt.dstIP.c_str(), pkt.dstPort);

	ImGui::Spacing();
	ImGui::TextUnformatted("Notes:");
	ImGui::TextWrapped("If fields are empty, packet parsing may not have detected the layer.");
	ImGui::Spacing();
}

void PacketDetailPanel::RenderHexDump(const PacketInfo& pkt, size_t bytesPerLine)
{
	const auto& buf = pkt.data;
	size_t size = buf.size();

	// Show up to the captured snippet
	ImGui::Text("Hex / ASCII Dump (first %zu bytes):", size);

	ImGui::BeginChild("HexDumpChild", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
	const int addrWidth = 8;

	ImGui::PushFont(ImGui::GetFont());
	for (size_t i = 0; i < size; i += bytesPerLine)
	{
		// Offset
		char offsetBuf[64];
		std::snprintf(offsetBuf, sizeof(offsetBuf), "%08zx: ", i);
		ImGui::TextUnformatted(offsetBuf);
		ImGui::SameLine();

		// Hex bytes
		std::ostringstream hex;
		std::ostringstream ascii;
		for (size_t j = 0; j < bytesPerLine; ++j)
		{
			size_t idx = i + j;
			if (idx < size)
			{
				uint8_t byte = buf[idx];
				hex << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
				ascii << (isprint(byte) ? (char)byte : '.');
			}
			else
			{
				hex << "   "; // Padding for missing bytes
				ascii << " "; 
			}
		}

		ImGui::TextUnformatted(hex.str().c_str());
		ImGui::SameLine();
		ImGui::TextUnformatted(ascii.str().c_str());
	}

	ImGui::PopFont();
	ImGui::PopStyleVar();

	ImGui::EndChild();
}