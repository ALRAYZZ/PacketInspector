#include "PacketCrafterPanel.h"
#include <imgui.h>
#include <cstring>

PacketCrafterPanel::PacketCrafterPanel(std::shared_ptr<PacketCrafterEngine> engine)
	: crafterEngine(std::move(engine))
{
	// Initialize with defaults
	AutoFillLocalMAC();
}

void PacketCrafterPanel::Render()
{
	ImGui::SeparatorText("Packet Crafter - Build and Send Custom Packets");
	ImGui::TextWrapped("Craft custom packets with full control over each layer.");
	ImGui::Spacing();

	// Status message
	if (!statusMessage.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, statusIsError ?
			ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
		ImGui::TextWrapped("%s", statusMessage.c_str());
		ImGui::PopStyleColor();
		ImGui::Spacing();
	}

	// Two column layout
	float leftWidth = ImGui::GetContentRegionAvail().x * 0.5f;

	ImGui::BeginChild("LeftColumn", ImVec2(leftWidth, 0), true);
	{
		RenderEthernetSection();
		ImGui::Spacing();
		RenderIPv4Section();
		ImGui::Spacing();
		RenderTransportSection();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("RightColumn", ImVec2(0, 0), true);
	{
		RenderPayloadSection();
		ImGui::Spacing();
		RenderControlSection();
	}
	ImGui::EndChild();
}

void PacketCrafterPanel::RenderEthernetSection()
{
	if (ImGui::CollapsingHeader("Ethernet Layer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		ImGui::Text("Source MAC:");
		ImGui::SameLine();
		if (ImGui::Button("Auto##SrcMAC"))
		{
			AutoFillLocalMAC();
		}
		ImGui::InputText("##SrcMAC", srcMACBuf, sizeof(srcMACBuf));

		ImGui::Text("Destination MAC:");
		ImGui::InputText("##DstMAC", dstMACBuf, sizeof(dstMACBuf));
		ImGui::TextDisabled("Use ff:ff:ff:ff:ff:ff for broadcast");

		ImGui::Unindent();
	}
}

void PacketCrafterPanel::RenderIPv4Section()
{
	if (ImGui::CollapsingHeader("IPv4 Layer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		ImGui::Text("Source IP:");
		ImGui::InputText("##SrcIP", srcIPBuf, sizeof(srcIPBuf));

		ImGui::Text("Destination IP:");
		ImGui::InputText("##DstIP", dstIPBuf, sizeof(dstIPBuf));

		ImGui::SliderInt("TTL", reinterpret_cast<int*>(&config.ttl), 1, 255);
		ImGui::SliderInt("ToS", reinterpret_cast<int*>(&config.tos), 0, 255);
		ImGui::SliderInt("ID", reinterpret_cast<int*>(&config.identification), 0, 65535);
		ImGui::Checkbox("Don't Fragment", &config.dontFragment);

		ImGui::Unindent();
	}
}

void PacketCrafterPanel::RenderTransportSection()
{
	if (ImGui::CollapsingHeader("Transport Layer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		const char* protocols[] = { "TCP", "UDP", "ICMP" };
		ImGui::Combo("Protocol", &selectedProtocol, protocols, 3);

		if (selectedProtocol == 0) // TCP
		{
			config.protocol = TransportProtocol::TCP;

			ImGui::InputInt("Source Port", reinterpret_cast<int*>(&config.srcPort));
			ImGui::InputInt("Destination Port", reinterpret_cast<int*>(&config.dstPort));
			ImGui::InputInt("Sequence Number", reinterpret_cast<int*>(&config.seqNumber));
			ImGui::InputInt("ACK Number", reinterpret_cast<int*>(&config.ackNumber));
			ImGui::InputInt("Window Size", reinterpret_cast<int*>(&config.windowSize));

			ImGui::Text("TCP Flags:");
			ImGui::Checkbox("SYN", &config.flagSYN); ImGui::SameLine();
			ImGui::Checkbox("ACK", &config.flagACK); ImGui::SameLine();
			ImGui::Checkbox("FIN", &config.flagFIN);
			ImGui::Checkbox("RST", &config.flagRST); ImGui::SameLine();
			ImGui::Checkbox("PSH", &config.flagPSH); ImGui::SameLine();
			ImGui::Checkbox("URG", &config.flagURG);
		}
		else if (selectedProtocol == 1) // UDP
		{
			config.protocol = TransportProtocol::UDP;

			ImGui::InputInt("Source Port", reinterpret_cast<int*>(&config.srcPort));
			ImGui::InputInt("Destination Port", reinterpret_cast<int*>(&config.dstPort));
		}
		else // ICMP
		{
			config.protocol = TransportProtocol::ICMP;

			ImGui::SliderInt("Type", reinterpret_cast<int*>(&config.icmpType), 0, 255);
			ImGui::SliderInt("Code", reinterpret_cast<int*>(&config.icmpCode), 0, 255);
			ImGui::InputInt("ID", reinterpret_cast<int*>(&config.icmpId));
			ImGui::InputInt("Sequence", reinterpret_cast<int*>(&config.icmpSequence));
			ImGui::TextDisabled("Type 8 = Echo Request, Type 0 = Echo Reply");
		}

		ImGui::Unindent();
	}
}

void PacketCrafterPanel::RenderPayloadSection()
{
	if (ImGui::CollapsingHeader("Payload", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		const char* payloadTypes[] = { "Text", "Hex", "Random", "Pattern", "Numbers" };
		ImGui::Combo("Payload Type", &selectedPayloadType, payloadTypes, 5);

		switch (selectedPayloadType)
		{
		case 0: // Text
			config.payloadType = PayloadType::Text;
			ImGui::InputTextMultiline("##TextPayload", textPayloadBuf, sizeof(textPayloadBuf),
				ImVec2(-1, 150));
			config.payloadText = textPayloadBuf;
			break;

		case 1: // Hex
			config.payloadType = PayloadType::Hex;
			ImGui::TextDisabled("Enter hex bytes (e.g., 48656C6C6F for 'Hello')");
			ImGui::InputTextMultiline("##HexPayload", hexPayloadBuf, sizeof(hexPayloadBuf),
				ImVec2(-1, 150));
			config.payloadHex = hexPayloadBuf;
			break;

		case 2: // Random
			config.payloadType = PayloadType::Random;
			ImGui::SliderInt("Size (bytes)", reinterpret_cast<int*>(&config.payloadSize), 1, 1024);
			ImGui::TextDisabled("Generates random bytes");
			break;

		case 3: // Pattern
			config.payloadType = PayloadType::Pattern;
			ImGui::SliderInt("Size (bytes)", reinterpret_cast<int*>(&config.payloadSize), 1, 1024);
			ImGui::SliderInt("Pattern Byte", reinterpret_cast<int*>(&config.patternByte), 0, 255);
			ImGui::TextDisabled("Repeats the same byte");
			break;

		case 4: // Numbers
			config.payloadType = PayloadType::Numbers;
			ImGui::SliderInt("Size (bytes)", reinterpret_cast<int*>(&config.payloadSize), 1, 1024);
			ImGui::InputInt("Start Number", &config.numberStart);
			ImGui::InputInt("Increment", &config.numberIncrement);
			ImGui::TextDisabled("Sequential numbers (mod 256)");
			break;
		}

		ImGui::Unindent();
	}
}

void PacketCrafterPanel::RenderControlSection()
{
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Button("Send Packet", ImVec2(-1, 40)))
	{
		UpdateConfigFromUI();

		std::string errorMsg;
		if (crafterEngine->CraftAndSendPacket(config, errorMsg))
		{
			statusMessage = "Packet sent successfully!";
			statusIsError = false;
		}
		else
		{
			statusMessage = "Error: " + errorMsg;
			statusIsError = true;
		}
	}

	ImGui::Spacing();

	if (ImGui::Button("Reset to Defaults", ImVec2(-1, 0)))
	{
		config = PacketCraftConfig();
		SyncUIFromConfig();
		statusMessage.clear();
	}

	ImGui::Spacing();
	ImGui::TextWrapped("Tip: Send packets to 127.0.0.1 and capture them with the Packet Inspector tab for testing!");
}

void PacketCrafterPanel::UpdateConfigFromUI()
{
	config.srcMAC = srcMACBuf;
	config.dstMAC = dstMACBuf;
	config.srcIP = srcIPBuf;
	config.dstIP = dstIPBuf;
}

void PacketCrafterPanel::SyncUIFromConfig()
{
	strncpy(srcMACBuf, config.srcMAC.c_str(), sizeof(srcMACBuf) - 1);
	strncpy(dstMACBuf, config.dstMAC.c_str(), sizeof(dstMACBuf) - 1);
	strncpy(srcIPBuf, config.srcIP.c_str(), sizeof(srcIPBuf) - 1);
	strncpy(dstIPBuf, config.dstIP.c_str(), sizeof(dstIPBuf) - 1);
	strncpy(textPayloadBuf, config.payloadText.c_str(), sizeof(textPayloadBuf) - 1);
}

void PacketCrafterPanel::AutoFillLocalMAC()
{
	std::string mac = PacketCrafterEngine::GetLocalMACAddress();
	strncpy(srcMACBuf, mac.c_str(), sizeof(srcMACBuf) - 1);
	config.srcMAC = mac;
}