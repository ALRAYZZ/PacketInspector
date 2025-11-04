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
	for (int i = 0; i < devices.size(); ++i)
	{
		bool isSelected = (selectedDevice == i);
		if (ImGui::Selectable(devices[i].c_str(), isSelected))
		{
			selectedDevice = i;
		}

		ImGui::Spacing();

		if (selectedDevice >= 0 && !captureEngine->IsCapturing())
		{
			if (ImGui::Button("Start Capture"))
			{
				if (captureEngine->OpenDevice(selectedDevice))
				{
					captureEngine->StartCapture();
				}
			}
		}
		else if (captureEngine->IsCapturing())
		{
			if (ImGui::Button("Stop Capture"))
			{
				captureEngine->StopCapture();
			}
		}

		ImGui::SameLine();
		ImGui::Text(captureEngine->IsCapturing() ? "Status: Capturing..." : "Status: Idle.");
	}
}
