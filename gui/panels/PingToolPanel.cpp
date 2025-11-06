#include "PingToolPanel.h"
#include "core/PingEngine.h"
#include <imgui.h>

PingToolPanel::PingToolPanel(std::shared_ptr<PingEngine> engine)
	: pingEngine(engine)
{ }

void PingToolPanel::Render()
{
	RenderControlSection();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImVec2 availableRegion = ImGui::GetContentRegionAvail();
	float statsHeight = 120.0f;
	float resultsHeight = availableRegion.y - statsHeight - 20.0f;

	// Results section
	ImGui::BeginChild("PingResults", ImVec2(0, resultsHeight), true);
	RenderResultsSection();
	ImGui::EndChild();

	ImGui::Spacing();

	// Statistics section
	ImGui::BeginChild("PingStatistics", ImVec2(0, statsHeight), true);
	RenderStatisticsSection();
	ImGui::EndChild();
}

void PingToolPanel::RenderControlSection()
{
	ImGui::Text("Ping Configuration");
	ImGui::Spacing();

	// Target input
	ImGui::PushItemWidth(300.0f);
	if (pingEngine->IsPinging())
	{
		ImGui::BeginDisabled();
	}
	ImGui::InputText("Target IP/Hostname", targetAddress, sizeof(targetAddress));
	if (pingEngine->IsPinging())
	{
		ImGui::EndDisabled();
	}
	ImGui::PopItemWidth();

	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Enter a valid IP address or hostname to ping.");
	}

	ImGui::Spacing();

	// Ping options
	if (pingEngine->IsPinging())
	{
		ImGui::BeginDisabled();
	}

	ImGui::Checkbox("Continuous Ping", &continuousPing);
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("If enabled, ping will continue until stopped.");
	}

	if (!continuousPing)
	{
		ImGui::SameLine();
		ImGui::PushItemWidth(100.0f);
		ImGui::SliderInt("Count", &pingCount, 1, 100);
		ImGui::PopItemWidth();
	}

	ImGui::PushItemWidth(100.0f);
	ImGui::SliderInt("Timeout (ms)", &timeoutMs, 100, 5000);
	ImGui::PopItemWidth();

	if (pingEngine->IsPinging())
	{
		ImGui::EndDisabled();
	}

	ImGui::Spacing();

	// Control buttons
	bool isPinging = pingEngine->IsPinging();

	if (isPinging)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

		if (ImGui::Button("Stop ping", ImVec2(120, 0)))
		{
			pingEngine->StopPing();
		}

		ImGui::PopStyleColor(3);
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.2f, 1.0f));

		if (ImGui::Button("Start ping", ImVec2(120, 0)))
		{
			int count = continuousPing ? -1 : pingCount;
			if (!pingEngine->StartPing(targetAddress, count, timeoutMs))
			{
				ImGui::OpenPopup("Ping Error");
			}
		}
		ImGui::PopStyleColor(3);
	}

	ImGui::SameLine();

	if (ImGui::Button("Clear Results", ImVec2(120, 0)))
	{
		pingEngine->ClearResults();
	}

	ImGui::SameLine();
	ImGui::Checkbox("Auto Scroll", &autoScroll);

	// Error popup
	if (ImGui::BeginPopupModal("Ping Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Failed to start ping. Please check the target address.");
		ImGui::Spacing();
		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void PingToolPanel::RenderResultsSection()
{
	ImGui::Text("Ping Results");
	ImGui::Separator();
	ImGui::Spacing();

	auto results = pingEngine->GetResults();

	if (results.empty())
	{
		ImGui::TextDisabled("No ping results available. Click 'Start Ping' to begin.");
		return;
	}

	// Table for results
	if (ImGui::BeginTable("PingResultsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
	{
		ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Seq#", ImGuiTableColumnFlags_WidthFixed, 50.0f);
		ImGui::TableSetupColumn("Target", ImGuiTableColumnFlags_WidthFixed, 150.0f);
		ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Latency (ms)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		for (const auto& result : results)
		{
			ImGui::TableNextRow();

			// Time
			ImGui::TableNextColumn();
			ImGui::Text("%s", result.GetTimeString().c_str());

			// Sequence
			ImGui::TableNextColumn();
			ImGui::Text("%d", result.sequenceNumber);

			// Target
			ImGui::TableNextColumn();
			ImGui::Text("%s", result.targetIP.c_str());

			// Status
			ImGui::TableNextColumn();
			if (result.success)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
				ImGui::Text("Success (TTL=%d)", result.ttl);
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
				ImGui::Text("Failed");
				ImGui::PopStyleColor();
			}

			// Latency
			ImGui::TableNextColumn();
			if (result.success)
			{
				ImGui::Text("%.2f", result.latencyMs);
			}
			else
			{
				ImGui::TextDisabled("N/A");
			}
		}

		// Auto-scroll to bottom
		if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		{
			ImGui::SetScrollHereY(1.0f);
		}

		ImGui::EndTable();
	}
}

void PingToolPanel::RenderStatisticsSection()
{
	ImGui::Text("Statistics");
	ImGui::Separator();
	ImGui::Spacing();

	auto stats = pingEngine->GetStatistics();

	if (stats.packetsSent == 0)
	{
		ImGui::TextDisabled("No statistics available.");
		return;
	}

	// Display statistics in colums
	ImGui::Columns(2, nullptr, false);

	// Left column
	ImGui::Text("Packets Sent:");
	ImGui::Text("Packets Received:");
	ImGui::Text("Packet Loss:");

	ImGui::NextColumn();

	// Right column
	ImGui::Text("%d", stats.packetsSent);
	ImGui::Text("%d", stats.packetsReceived);

	if (stats.packetLossPercent > 0.0f)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
		ImGui::Text("%.1f%%", stats.packetLossPercent);
		ImGui::PopStyleColor();
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
		ImGui::Text("0.0%%");
		ImGui::PopStyleColor();
	}

	ImGui::Columns(1);
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Latency Statistics
	if (stats.packetsReceived > 0)
	{
		ImGui::Columns(3, nullptr, false);

		ImGui::Text("Min Latenct:");
		ImGui::NextColumn();
		ImGui::Text("Avg Latency:");
		ImGui::NextColumn();
		ImGui::Text("Max Latency:");
		ImGui::NextColumn();

		ImGui::Text("%.2f ms", stats.minLatencyMs);
		ImGui::NextColumn();
		ImGui::Text("%.2f ms", stats.avgLatencyMs);
		ImGui::NextColumn();
		ImGui::Text("%.2f ms", stats.maxLatencyMs);

		ImGui::Columns(1);
	}
}