#include "CaptureEngine.h"
#include "PacketParser.h"
#include <iostream>
#include <algorithm>

CaptureEngine::CaptureEngine()
	: allDevices(nullptr), handle(nullptr), capturing(false)
{
}

CaptureEngine::~CaptureEngine()
{
	StopCapture();
	CloseDevice();
	FreeDeviceList();
}

// Initialize the capture engine and retrieve available devices
bool CaptureEngine::Initialize()
{
	char errbuf[PCAP_ERRBUF_SIZE];

	if (pcap_findalldevs(&allDevices, errbuf) == -1)
	{
		std::cerr << "Error finding devices: " << errbuf << std::endl;
		return false;
	}

	// Store device names
	deviceNames.clear();
	for (pcap_if_t* d = allDevices; d; d = d->next)
	{
		if (d->name)
		{
			deviceNames.push_back(d->description ? d->description : d->name);
		}
	}

	// Return true if at least one device was found 
	return !deviceNames.empty();
}

// Get the list of available device names
std::vector<std::string> CaptureEngine::GetAvailableDevices() const
{
	return deviceNames;
}

// Open a device by its index in the device list
bool CaptureEngine::OpenDevice(int index)
{
	if (!allDevices)
	{
		return false;
	}

	pcap_if_t* dev = allDevices;
	for (int i = 0; dev && i < index; ++i)
	{
		// Iterate until the desired index
		dev = dev->next;
	}

	if (!dev)
	{
		return false;
	}

	char errbuf[PCAP_ERRBUF_SIZE];
	handle = pcap_open_live(dev->name, 65536, 1, 1000, errbuf);
	if (!handle)
	{
		std::cerr << "Could not open device: " << errbuf << std::endl;
		return false;
	}

	std::cout << "Opened device: " << (dev->description ? dev->description : dev->name) << std::endl;
	return true;
}

// Close the currently opened device
void CaptureEngine::CloseDevice()
{
	// Ensure capture is stopped before closing the handle
	StopCapture();

	if (handle)
	{
		pcap_close(handle);
		handle = nullptr;
	}
}

// Start capturing packets on the opened device
bool CaptureEngine::StartCapture()
{
	if (!handle || capturing.load())
	{
		return false;
	}

	capturing.store(true);

	// Run the pcap loop on a background thread so the GUI stays responsive
	captureThread = std::thread([this]()
	{
		std::cout << "Starting capture..." << std::endl;
		const int res = pcap_loop(handle, 0, PacketHandler, reinterpret_cast<u_char*>(this));
		// pcap_loop returns -2 when pcap_breakloop is called, which is not an error
		if (res < 0 && res != -2)
		{
			std::cerr << "Error during capture: " << pcap_geterr(handle) << std::endl;
		}
		capturing.store(false);
	});

	return true;
}

// Stop capturing packets
void CaptureEngine::StopCapture()
{
	bool wasCapturing = capturing.load();

	if (handle)
	{
		pcap_breakloop(handle);
	}

	if (captureThread.joinable())
	{
		captureThread.join();
	}

	capturing.store(false);

	// Log message based on whether capturing was active
	if (wasCapturing)
	{
		std::cout << "Capture stopped." << std::endl;
	}
}

// Free the device list allocated by pcap_findalldevs
void CaptureEngine::FreeDeviceList()
{
	if (allDevices)
	{
		pcap_freealldevs(allDevices);
		allDevices = nullptr;
	}
}

// Static packet handler called by pcap for each captured packet
void CaptureEngine::PacketHandler(u_char* user, const pcap_pkthdr* header, const u_char* bytes)
{
	auto* self = reinterpret_cast<CaptureEngine*>(user);
	if (!self || !header || !bytes)
	{
		return;
	}

	PacketInfo packet;

	using namespace std::chrono;
	const auto secs = seconds{ static_cast<long long>(header->ts.tv_sec) };
	const auto usecs = microseconds{ header->ts.tv_usec };
	packet.timestamp = system_clock::time_point{ duration_cast<system_clock::duration>(secs + usecs)};

	packet.length = header->len;

	const size_t available = static_cast<size_t>(header->caplen);
	const size_t toCopy = std::min<size_t>(available, 64u);
	packet.data.assign(bytes, bytes + toCopy);
	PacketParser::ParsePacket(packet);
	{
		// Prevent concurrent access to packetBuffer
		std::scoped_lock lock(self->packetMutex);
		self->packetBuffer.push_back(std::move(packet));
		if (self->packetBuffer.size() > self->maxPackets)
		{
			self->packetBuffer.pop_front();
		}
	}
}

// Retrieve a copy of recent captured packets
std::vector<PacketInfo> CaptureEngine::GetRecentPackets()
{
	std::scoped_lock lock(packetMutex);
	return std::vector<PacketInfo>(packetBuffer.begin(), packetBuffer.end());
}