#include "CaptureEngine.h"
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

std::vector<std::string> CaptureEngine::GetAvailableDevices() const
{
	return deviceNames;
}

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

void CaptureEngine::CloseDevice()
{
	if (handle)
	{
		pcap_close(handle);
		handle = nullptr;
	}
}

bool CaptureEngine::StartCapture()
{
	if (!handle)
	{
		return false;
	}

	capturing = true;

	// Start async capture loop in same thread for now
	std::cout << "Starting capture..." << std::endl;
	if (pcap_loop(handle, 0, PacketHandler, reinterpret_cast<u_char*>(this)) < 0)
	{
		std::cerr << "Error during capture: " << pcap_geterr(handle) << std::endl;
		capturing = false;
		return false;
	}

	return true;
}

void CaptureEngine::StopCapture()
{
	if (capturing && handle)
	{
		pcap_breakloop(handle);
		capturing = false;
		std::cout << "Capture stopped." << std::endl;
	}
}

void CaptureEngine::FreeDeviceList()
{
	if (allDevices)
	{
		pcap_freealldevs(allDevices);
		allDevices = nullptr;
	}
}

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
	{
		// Prevent concurrent access to packetBuffer
		std::scoped_lock(self->packetMutex);
		self->packetBuffer.push_back(std::move(packet));
		if (self->packetBuffer.size() > self->maxPackets)
		{
			self->packetBuffer.pop_front();
		}
	}

}

std::vector<PacketInfo> CaptureEngine::GetRecentPackets()
{
	std::scoped_lock lock(packetMutex);
	return std::vector<PacketInfo>(packetBuffer.begin(), packetBuffer.end());
}