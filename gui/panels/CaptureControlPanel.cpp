#include "CaptureControlPanel.h"
#include <imgui.h>

CaptureControlPanel::CaptureControlPanel(std::shared_ptr<CaptureEngine> engine)
	: captureEngine(std::move(engine)), selectedDevice(-1), initialized(false)
{
}

void CaptureControlPanel::Render()
{
	if (!initialized)
	{
		if (captureEngine->Initialize())
		{
			devices = captureEngine->GetAvailableDevices();
			initialized = true;
		}
	}

	ImGui::SeparatorText("Capture Control");

	if (!initialized)
	{
		ImGui::Text("Failed to initialize capture engine.");
		return;
	}

	if (devices.empty())
	{
		ImGui::Text("No interfaces found.");
		return;
	}

	// Two column layout : Device list on left, controls on right
	ImGui::BeginChild("DeviceList", ImVec2(ImGui::GetContentRegionAvail().x * 0.6f, 0), true);
	{
		ImGui::TextUnformatted("Network Interfaces:");
		ImGui::Separator();

		// Device selection list
		for (int i = 0; i < static_cast<int>(devices.size()); ++i)
		{
			bool isSelected = (selectedDevice == i);
			if (ImGui::Selectable(devices[i].c_str(), isSelected))
			{
				selectedDevice = i;
			}
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// Right column: Global controls for selected device
	ImGui::BeginChild("Controls", ImVec2(0, 0), true);
	{
		ImGui::TextUnformatted("Capture Controls:");
		ImGui::Separator();
		ImGui::Spacing();


		if (selectedDevice < 0)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
			ImGui::TextWrapped("Select a network interface to enable capture controls.");
			ImGui::PopStyleColor();
			ImGui::Spacing();
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
			ImGui::TextWrapped("Selected Interface: %s", devices[selectedDevice].c_str());
			ImGui::PopStyleColor();
			ImGui::Spacing();
		}

		ImGui::Separator();
		ImGui::Spacing();


		// Capture controls
		ImGui::TextUnformatted("Packet Capture:");

		if (!captureEngine->IsCapturing())
		{
			if (ImGui::Button("Start Capture", ImVec2(-1, 0)))
			{
				if (selectedDevice < 0)
				{
					ImGui::OpenPopup("NoDeviceError");
				}
				else
				{
					if (captureEngine->OpenDevice(selectedDevice))
					{
						captureEngine->StartCapture();
					}
					else
					{
						ImGui::OpenPopup("DeviceOpenError");
					}
				}
			}
		}
		else
		{
			if (ImGui::Button("Stop Capture", ImVec2(-1, 0)))
			{
				captureEngine->StopCapture();
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Dump controls
		ImGui::TextUnformatted("Capture Dump:");

		static char dumpFilename[256] = "capture_dump.pcap";

		// Only allow changing filename when not dumping
		if (!captureEngine->IsDumping())
		{
			ImGui::InputText("File Name", dumpFilename, IM_ARRAYSIZE(dumpFilename));
		}
		else
		{
			ImGui::BeginDisabled();
			ImGui::InputText("File Name", dumpFilename, IM_ARRAYSIZE(dumpFilename));
			ImGui::EndDisabled();
		}

		if (!captureEngine->IsDumping())
		{
			if (ImGui::Button("Start Dump", ImVec2(-1, 0)))
			{
				if (selectedDevice < 0)
				{
					ImGui::OpenPopup("NoDeviceError");
				}
				else
				{
					if (captureEngine->StartDump(dumpFilename))
					{
						// Dump started successfully
					}
					else
					{
						ImGui::OpenPopup("DumpStartError");
					}
				}
			}
		}
		else
		{
			if (ImGui::Button("Stop Dump", ImVec2(-1, 0)))
			{
				captureEngine->StopDump();
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Status display
		ImGui::TextUnformatted("Status:");
		ImGui::Indent();

		if (captureEngine->IsCapturing())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
			ImGui::TextUnformatted("Capturing packets...");
			ImGui::PopStyleColor();
		}
		else
		{
			ImGui::TextUnformatted("Idle");
		}

		if (captureEngine->IsDumping())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 1.0f, 1.0f));
			ImGui::TextUnformatted("Writting to file...");
			ImGui::PopStyleColor();
		}

		ImGui::Unindent();

		// Error popus
		if (ImGui::BeginPopupModal("NoDeviceError", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("No network interface selected!");
			ImGui::Spacing();
			ImGui::TextWrapped("Please select a network interface from the list to start capturing or dumping packets.");
			ImGui::Spacing();
			ImGui::Separator();
			if (ImGui::Button("OK", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("DeviceOpenError", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Failed to open the selected network interface!");
			ImGui::Spacing();
			ImGui::TextWrapped("Please ensure you have the necessary permissions and that the interface is not in use by another application.");
			ImGui::Spacing();
			ImGui::Separator();
			if (ImGui::Button("OK", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("DumpStartError", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Failed to start dumping to file!");
			ImGui::Spacing();
			ImGui::TextWrapped("Please ensure the file path is valid and that you have write permissions.");
			ImGui::Spacing();
			ImGui::Separator();
			if (ImGui::Button("OK", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	ImGui::EndChild();
}

