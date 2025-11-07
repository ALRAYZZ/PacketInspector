#include "PacketCrafterEngine.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#define NOMINMAX
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#include <pcap.h>
#else
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pcap.h>
#endif

#pragma pack(push, 1)
struct EthernetHeader
{
	uint8_t dst[6];
	uint8_t src[6];
	uint16_t type;
};

struct IPv4Header
{
	uint8_t ihl_version; // Version (4 bits) + IHL (4 bits)
	uint8_t tos;
	uint16_t tot_len;
	uint16_t id;
	uint16_t frag_off;
	uint8_t ttl;
	uint8_t protocol; // 6=TCP, 17=UDP, 1=ICMP
	uint16_t check;
	uint32_t saddr;
	uint32_t daddr;
};

struct TCPHeader
{
	uint16_t src_port;
	uint16_t dst_port;
	uint32_t seq;
	uint32_t ack_seq;
	uint8_t offset_reserved;
	uint8_t flags;
	uint16_t window;
	uint16_t check;
	uint16_t urg_ptr;
};

struct UDPHeader
{
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t len;
	uint16_t check;
};

struct ICMPHeader
{
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t id;
	uint16_t sequence;
};
#pragma pack(pop)

PacketCrafterEngine::PacketCrafterEngine()
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

PacketCrafterEngine::~PacketCrafterEngine()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

bool PacketCrafterEngine::CraftAndSendPacket(const PacketCraftConfig& config, std::string& errorMsg)
{
	// Validate configuration
	if (!ValidateConfig(config, errorMsg))
	{
		return false;
	}

	// Build packet
	std::vector<uint8_t> packet = BuildPacket(config);
	if (packet.empty())
	{
		errorMsg = "Failed to build packet.";
		return false;
	}

	// Send using pcap
	char errbuf[PCAP_ERRBUF_SIZE];

	// Open default device for sending
	pcap_if_t* alldevs = nullptr;
	if (pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		errorMsg = "Error finding devices: " + std::string(errbuf);
		return false;
	}

	if (!alldevs)
	{
		errorMsg = "No devices found for sending.";
		return false;
	}

	// Use the first device
	pcap_t* handle = pcap_open_live(alldevs->name, 65536, 1, 1000, errbuf);
	pcap_freealldevs(alldevs);

	if (!handle)
	{
		errorMsg = "Error opening device: " + std::string(errbuf);
		return false;
	}

	// Send packet
	if (pcap_sendpacket(handle, packet.data(), static_cast<int>(packet.size())) != 0)
	{
		errorMsg = "Error sending packet: " + std::string(pcap_geterr(handle));
		pcap_close(handle);
		return false;
	}

	pcap_close(handle);
	return true;
}

std::vector<uint8_t> PacketCrafterEngine::BuildPacket(const PacketCraftConfig& config)
{
	std::vector<uint8_t> packet;

	// Generate payload first
	std::vector<uint8_t> payload = GeneratePayload(config);

	// Calculate total packet size
	size_t ethSize = sizeof(EthernetHeader);
	size_t ipSize = sizeof(IPv4Header);
	size_t transportSize = 0;

	switch (config.protocol)
	{
	case TransportProtocol::TCP:
		transportSize = sizeof(TCPHeader);
		break;
	case TransportProtocol::UDP:
		transportSize = sizeof(UDPHeader);
		break;
	case TransportProtocol::ICMP:
		transportSize = sizeof(ICMPHeader);
		break;
	}

	size_t totalSize = ethSize + ipSize + transportSize + payload.size();
	packet.resize(totalSize, 0);

	// Build Ethernet Header
	EthernetHeader* eth = reinterpret_cast<EthernetHeader*>(packet.data());

	// Parse MAC addresses
	unsigned int dstMAC[6], srcMAC[6];
	sscanf(config.dstMAC.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
		&dstMAC[0], &dstMAC[1], &dstMAC[2], &dstMAC[3], &dstMAC[4], &dstMAC[5]);
	sscanf(config.srcMAC.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
		&srcMAC[0], &srcMAC[1], &srcMAC[2], &srcMAC[3], &srcMAC[4], &srcMAC[5]);

	for (int i = 0; i < 6; ++i)
	{
		eth->dst[i] = static_cast<uint8_t>(dstMAC[i]);
		eth->src[i] = static_cast<uint8_t>(srcMAC[i]);
	}
	eth->type = htons(0x0800); // IPv4

	// Build IPv4 Header
	IPv4Header* ip = reinterpret_cast<IPv4Header*>(packet.data() + ethSize);
	ip->ihl_version = 0x45;
	ip->tos = config.tos;
	ip->tot_len = htons(static_cast<uint16_t>(ipSize + transportSize + payload.size()));
	ip->id = htons(config.identification);
	ip->frag_off = config.dontFragment ? htons(0x4000) : 0;
	ip->ttl = config.ttl;

	// Set protocol
	switch (config.protocol)
	{
	case TransportProtocol::TCP: ip->protocol = 6; break;
	case TransportProtocol::UDP: ip->protocol = 17; break;
	case TransportProtocol::ICMP: ip->protocol = 1; break;
	}

	ip->saddr = inet_addr(config.srcIP.c_str());
	ip->daddr = inet_addr(config.dstIP.c_str());
	ip->check = 0;
	ip->check = CalculateChecksum(reinterpret_cast<uint16_t*>(ip), ipSize);

	// Build Transport Layer
	size_t transportOffset = ethSize + ipSize;

	switch (config.protocol)
	{
	case TransportProtocol::TCP:
	{
		TCPHeader* tcp = reinterpret_cast<TCPHeader*>(packet.data() + transportOffset);
		tcp->src_port = htons(config.srcPort);
		tcp->dst_port = htons(config.dstPort);
		tcp->seq = htonl(config.seqNumber);
		tcp->ack_seq = htonl(config.ackNumber);
		tcp->offset_reserved = 0x50; // Data offset: 5 * 4 = 20 bytes
		tcp->flags = 0;
		if (config.flagSYN) tcp->flags |= 0x02;
		if (config.flagACK) tcp->flags |= 0x10;
		if (config.flagFIN) tcp->flags |= 0x01;
		if (config.flagRST) tcp->flags |= 0x04;
		if (config.flagPSH) tcp->flags |= 0x08;
		if (config.flagURG) tcp->flags |= 0x20;
		tcp->window = htons(config.windowSize);
		tcp->check = 0;
		tcp->urg_ptr = 0;

		// Copy payload
		if (!payload.empty())
		{
			memcpy(packet.data() + transportOffset + transportSize, payload.data(), payload.size());
		}

		// Calculate TCP checksum
		tcp->check = CalculateTCPChecksum(packet, ethSize, transportOffset);
		break;
	}
	case TransportProtocol::UDP:
	{
		UDPHeader* udp = reinterpret_cast<UDPHeader*>(packet.data() + transportOffset);
		udp->src_port = htons(config.srcPort);
		udp->dst_port = htons(config.dstPort);
		udp->len = htons(static_cast<uint16_t>(transportSize + payload.size()));
		udp->check = 0;

		// Copy payload
		if (!payload.empty())
		{
			memcpy(packet.data() + transportOffset + transportSize, payload.data(), payload.size());
		}

		// Calculate UDP checksum (optional, can be 0)
		udp->check = CalculateUDPChecksum(packet, ethSize, transportOffset);
		break;
	}
	case TransportProtocol::ICMP:
	{
		ICMPHeader* icmp = reinterpret_cast<ICMPHeader*>(packet.data() + transportOffset);
		icmp->type = config.icmpType;
		icmp->code = config.icmpCode;
		icmp->id = htons(config.icmpId);
		icmp->sequence = htons(config.icmpSequence);
		icmp->checksum = 0;

		// Copy payload
		if (!payload.empty())
		{
			memcpy(packet.data() + transportOffset + transportSize, payload.data(), payload.size());
		}

		// Calculate ICMP checksum
		icmp->checksum = CalculateChecksum(
			reinterpret_cast<uint16_t*>(icmp),
			transportSize + payload.size()
		);
		break;
	}
	}
	return packet;
}

std::vector<uint8_t> PacketCrafterEngine::GeneratePayload(const PacketCraftConfig& config)
{
	std::vector<uint8_t> payload;

	switch (config.payloadType)
	{
	case PayloadType::Text:
	{
		const std::string& text = config.payloadText;
		payload.assign(text.begin(), text.end());
		break;
	}
	case PayloadType::Hex:
	{
		std::string hex = config.payloadHex;
		// Remove spaces and non-hex characters
		hex.erase(std::remove_if(hex.begin(), hex.end(),
			[](char c) { return !isxdigit(c); }), hex.end());

		// Convert hex string to bytes
		for (size_t i = 0; i + 1 < hex.length(); i += 2)
		{
			std::string byteString = hex.substr(i, 2);
			uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
			payload.push_back(byte);
		}
		break;
	}
	case PayloadType::Random:
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, 255);

		payload.resize(config.payloadSize);
		for (size_t i = 0; i < config.payloadSize; i++)
		{
			payload[i] = static_cast<uint8_t>(dis(gen));
		}
		break;
	}
	case PayloadType::Pattern:
	{
		payload.resize(config.payloadSize, config.patternByte);
		break;
	}
	case PayloadType::Numbers:
	{
		int num = config.numberStart;
		payload.reserve(config.payloadSize);
		for (size_t i = 0; i < config.payloadSize; i++)
		{
			payload.push_back(static_cast<uint8_t>(num & 0xFF));
			num += config.numberIncrement;
		}
		break;
	}
	}
	return payload;
}

bool PacketCrafterEngine::ValidateConfig(const PacketCraftConfig& config, std::string& errorMsg)
{
	if (!ValidateIPAddress(config.srcIP))
	{
		errorMsg = "Invalid source IP address.";
		return false;
	}

	if (!ValidateIPAddress(config.dstIP))
	{
		errorMsg = "Invalid destination IP address.";
		return false;
	}

	if (!ValidateMACAddress(config.srcMAC))
	{
		errorMsg = "Invalid source MAC address.";
		return false;
	}

	if (config.protocol == TransportProtocol::TCP || config.protocol == TransportProtocol::UDP)
	{
		if (config.srcPort == 0 || config.dstPort == 0)
		{
			errorMsg = "Source and destination ports must be non-zero for TCP/UDP.";
			return false;
		}
	}
	return true;
}

bool PacketCrafterEngine::ValidateIPAddress(const std::string& ip)
{
	struct sockaddr_in sa;
	return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) != 0;
}

bool PacketCrafterEngine::ValidateMACAddress(const std::string& mac)
{
	unsigned int bytes[6];
	int count = sscanf(mac.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
		&bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5]);
	return count == 6;
}

std::string PacketCrafterEngine::GetLocalMACAddress()
{
#ifdef _WIN32
	IP_ADAPTER_INFO adapterInfo[16];
	DWORD bufLen = sizeof(adapterInfo);

	DWORD status = GetAdaptersInfo(adapterInfo, &bufLen);
	if (status == ERROR_SUCCESS)
	{
		PIP_ADAPTER_INFO adapter = adapterInfo;
		if (adapter)
		{
			char mac[32];
			snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
				adapter->Address[0], adapter->Address[1], adapter->Address[2],
				adapter->Address[3], adapter->Address[4], adapter->Address[5]);
			return mac;
		}
	}
#endif
	return "00:00:00:00:00:00";
}

std::string PacketCrafterEngine::GetGatewayMACAddress(const std::string& gatewayIP)
{
	// This function would require ARP requests to get the MAC address of the gateway.
	return "ff:ff:ff:ff:ff:ff";
}

uint16_t PacketCrafterEngine::CalculateChecksum(const uint16_t* buf, size_t len)
{
	uint32_t sum = 0;
	size_t count = len / 2;

	while (count--)
	{
		sum += *buf++;
		if (sum & 0xFFFF0000)
		{
			sum &= 0xFFFF;
			sum++;
		}
	}
	return static_cast<uint16_t>(~sum);
}

uint16_t PacketCrafterEngine::CalculateTCPChecksum(const std::vector<uint8_t>& packet, size_t ipHeaderOffset, size_t tcpOffset)
{
	const IPv4Header* ip = reinterpret_cast<const IPv4Header*>(packet.data() + ipHeaderOffset);
	size_t tcpLen = packet.size() - tcpOffset;

	// Pseudo-header for checksum
	std::vector<uint8_t> pseudoPacket;
	pseudoPacket.reserve(12 + tcpLen);

	// Source IP
	pseudoPacket.push_back((ip->saddr >> 0) & 0xFF);
	pseudoPacket.push_back((ip->saddr >> 8) & 0xFF);
	pseudoPacket.push_back((ip->saddr >> 16) & 0xFF);
	pseudoPacket.push_back((ip->saddr >> 24) & 0xFF);

	// Dest IP
	pseudoPacket.push_back((ip->daddr >> 0) & 0xFF);
	pseudoPacket.push_back((ip->daddr >> 8) & 0xFF);
	pseudoPacket.push_back((ip->daddr >> 16) & 0xFF);
	pseudoPacket.push_back((ip->daddr >> 24) & 0xFF);

	// Zero + Protocol
	pseudoPacket.push_back(0);
	pseudoPacket.push_back(6); // TCP

	// TCP Length
	pseudoPacket.push_back((tcpLen >> 8) & 0xFF);
	pseudoPacket.push_back(tcpLen & 0xFF);

	// TCP header + data
	pseudoPacket.insert(pseudoPacket.end(), packet.begin() + tcpOffset, packet.end());

	return CalculateChecksum(reinterpret_cast<const uint16_t*>(pseudoPacket.data()),
		pseudoPacket.size());
}

uint16_t PacketCrafterEngine::CalculateUDPChecksum(const std::vector<uint8_t>& packet,
	size_t ipHeaderOffset, size_t udpOffset)
{
	const IPv4Header* ip = reinterpret_cast<const IPv4Header*>(packet.data() + ipHeaderOffset);
	size_t udpLen = packet.size() - udpOffset;

	// Pseudo-header for checksum (similar to TCP)
	std::vector<uint8_t> pseudoPacket;
	pseudoPacket.reserve(12 + udpLen);

	// Source IP
	pseudoPacket.push_back((ip->saddr >> 0) & 0xFF);
	pseudoPacket.push_back((ip->saddr >> 8) & 0xFF);
	pseudoPacket.push_back((ip->saddr >> 16) & 0xFF);
	pseudoPacket.push_back((ip->saddr >> 24) & 0xFF);

	// Dest IP
	pseudoPacket.push_back((ip->daddr >> 0) & 0xFF);
	pseudoPacket.push_back((ip->daddr >> 8) & 0xFF);
	pseudoPacket.push_back((ip->daddr >> 16) & 0xFF);
	pseudoPacket.push_back((ip->daddr >> 24) & 0xFF);

	// Zero + Protocol
	pseudoPacket.push_back(0);
	pseudoPacket.push_back(17); // UDP

	// UDP Length
	pseudoPacket.push_back((udpLen >> 8) & 0xFF);
	pseudoPacket.push_back(udpLen & 0xFF);

	// UDP header + data
	pseudoPacket.insert(pseudoPacket.end(), packet.begin() + udpOffset, packet.end());

	return CalculateChecksum(reinterpret_cast<const uint16_t*>(pseudoPacket.data()),
		pseudoPacket.size());
}


