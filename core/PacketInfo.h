#pragma once
#include <string>
#include <vector>
#include <chrono>

struct PacketInfo
{
	std::chrono::system_clock::time_point timestamp;
	uint32_t length = 0;
	std::vector<uint8_t> data;

	std::string srcAddr;
	std::string dstAddr;
	std::string protocol;
	uint16_t srcPort = 0;
	uint16_t dstPort = 0;

	std::string GetTimeString() const;
	std::string GetHexPreview(size_t maxBytes = 16) const;

	bool incoming = false; // true = received, false = sent
};