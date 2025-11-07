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

	// Device selection list
	for (int i = 0; i < static_cast<int>(devices.size()); ++i)
	{
		ImGui::PushID(i); // Ensure unique ID for each selectable

		bool isSelected = (selectedDevice == i);
		if (ImGui::Selectable(devices[i].c_str(), isSelected))
		{
			selectedDevice = i;
		}

		// Only show controls for the selected device
		if (isSelected)
		{
			ImGui::Spacing();

			if (!captureEngine->IsCapturing())
			{
				if (ImGui::Button("Start capture"))
				{
					if (captureEngine->OpenDevice(i))
					{
						captureEngine->StartCapture();
					}
				}
			}
			else
			{
				if (ImGui::Button("Stop Capture"))
				{
					captureEngine->StopCapture();
				}
			}

			// Allow dump controls regardless of capture state
			ImGui::Separator();
			ImGui::TextUnformatted("Capture Dump:");

			static char dumpFilename[256] = "capture_dump.pcap";

			// Only allow changing filename when not dumping
			if (!captureEngine->IsDumping())
			{
				ImGui::InputText("File Name", dumpFilename, IM_ARRAYSIZE(dumpFilename));
			}
			else
			{
				ImGui::TextDisabled("File Name: %s", dumpFilename);
			}

			if (!captureEngine->IsDumping())
			{
				if (ImGui::Button("Start Dump"))
				{
					captureEngine->StartDump(dumpFilename);
				}
			}
			else
			{
				if (ImGui::Button("Stop Dump"))
				{
					captureEngine->StopDump();
				}
			}

			ImGui::SameLine();
			ImGui::Text(captureEngine->IsCapturing() ? "Status: Capturing..." : "Status: Idle.");
		}

		ImGui::PopID();
	}
}
