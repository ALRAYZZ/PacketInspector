#pragma once
#include <string>
#include <vector>
#include <pcap.h>
#include "PacketInfo.h"
#include <mutex>
#include <deque>

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

	bool IsCapturing() const { return capturing.load(); }

	std::vector<PacketInfo> GetRecentPackets();

private:
	pcap_if_t* allDevices;
	pcap_t* handle;
	std::atomic<bool> capturing;
	std::thread	captureThread;

	std::vector<std::string> deviceNames;
	std::deque<PacketInfo> packetBuffer;
	std::recursive_mutex packetMutex;
	const size_t maxPackets = 200;

	void FreeDeviceList();
	static void PacketHandler(u_char* user, const pcap_pkthdr* header, const u_char* bytes);
};