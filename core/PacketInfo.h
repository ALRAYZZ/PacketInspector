#pragma once
#include <string>
#include <vector>
#include <chrono>

struct PacketInfo
{
	std::chrono::system_clock::time_point timestamp;
	uint32_t length = 0;
	std::vector<uint8_t> data;

	// Parsed layer info
	std::string etherSrc;
	std::string etherDst;
	std::string etherTypeStr;

	std::string srcIP;
	std::string dstIP;
	uint8_t ttl = 0;
	uint8_t protocol = 0;

	std::string transportProtocol; // e.g., "TCP", "UDP", "ICMP"
	uint16_t srcPort = 0;
	uint16_t dstPort = 0;

	std::vector<std::pair<std::string, std::string>> extraFields; // e.g., TCP flags, ICMP type/code

	std::string GetTimeString() const;
	std::string GetHexPreview(size_t maxBytes = 16) const;

	bool incoming = false; // true = received, false = sent
};