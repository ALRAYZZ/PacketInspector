#pragma once
#include <string>
#include <vector>
#include <pcap.h>

// Forward declaration of PacketInfo
struct PacketInfo;

class CaptureEngine
{
public:
	CaptureEngine();
	~CaptureEngine();

	// Initialization and device handling
	bool Initialize();
	std::vector<std::string> GetAvailableDevices() const;
	bool OpenDevice(int index);
	void CloseDevice();

	// Capture control
	bool StartCapture();
	void StopCapture();

	bool IsCapturing() const { return capturing; }

private:
	pcap_if_t* allDevices;
	pcap_t* handle;
	bool capturing;
	std::vector<std::string> deviceNames;

	void FreeDeviceList();
	static void PacketHandler(u_char* user, const pcap_pkthdr* header, const u_char* bytes);
};