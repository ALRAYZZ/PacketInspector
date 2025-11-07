#pragma once
#include <string>
#include <vector>
#include <pcap.h>
#include "PacketInfo.h"
#include <mutex>
#include <deque>
#include <map>
#include <tuple>
#include <atomic>
#include <thread>

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

	// Dumping control
	bool StartDump(const std::string& filename);
	void StopDump();
	bool IsDumping() const { return dumpingEnabled; }

	std::vector<PacketInfo> GetRecentPackets();
	std::map<std::tuple<std::string, uint16_t>, std::vector<PacketInfo>> GetGroupedPackets(bool incomingOnly);

private:
	pcap_if_t* allDevices;
	pcap_t* handle;
	std::atomic<bool> capturing;
	std::thread	captureThread;

	std::vector<std::string> deviceNames;
	std::deque<PacketInfo> packetBuffer;
	std::recursive_mutex packetMutex;
	const size_t maxPackets = 200;

	pcap_dumper_t* dumper = nullptr;
	bool dumpingEnabled = false;

	void FreeDeviceList();
	static void PacketHandler(u_char* user, const pcap_pkthdr* header, const u_char* bytes);
};