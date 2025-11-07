#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class PayloadType
{
	Text,		// ASCII/UTF8 Text
	Hex,		// Hexadecimal Bytes
	Random,		// Random Bytes
	Pattern,	// Repeating Pattern (e.g., 0xAA, 0xBB, 0xCC...)
	Numbers		// Sequential Numbers
};

enum class TransportProtocol
{
	TCP,
	UDP,
	ICMP
};

struct PacketCraftConfig
{
	// Ethernet Layer
	std::string srcMAC = "00:00:00:00:00:00"; // Auto-fill from interface
	std::string dstMAC = "FF:FF:FF:FF:FF:FF"; // Default to broadcast

	// IPv4 Layer
	std::string srcIP = "127.0.0.1";
	std::string dstIP = "127.0.0.1";
	uint8_t ttl = 64;
	uint8_t tos = 0;
	uint16_t identification = 0; // Auto-increment
	bool dontFragment = false;

	// Transport Layer
	TransportProtocol protocol = TransportProtocol::UDP;
	uint16_t srcPort = 12345;
	uint16_t dstPort = 54321;

	// TCP specific
	uint32_t seqNumber = 0;
	uint32_t ackNumber = 0;
	bool flagSYN = false;
	bool flagACK = false;
	bool flagFIN = false;
	bool flagRST = false;
	bool flagPSH = false;
	bool flagURG = false;
	uint16_t windowSize = 65535;

	// ICMP specific
	uint8_t icmpType = 8; // Echo Request
	uint8_t icmpCode = 0;
	uint16_t icmpId = 1;
	uint16_t icmpSequence = 1;

	// Payload
	PayloadType payloadType = PayloadType::Text;
	std::string payloadText = "Hello, Packet Inspector! made by ALRAYZZ";
	std::string payloadHex = "48656C6C6F2C205061636B657420496E73706563746F7221206D61646520627920414C5241595A5A";
	uint8_t patternByte = 0x00;
	size_t payloadSize = 32; // For Random/Pattern/Numbers
	int numberStart = 0; // For Numbers
	int numberIncrement = 1;
};

class PacketCrafterEngine
{
public:
	PacketCrafterEngine();
	~PacketCrafterEngine();

	// Build and send packet
	bool CraftAndSendPacket(const PacketCraftConfig& config, std::string& errorMsg);

	// Helper functions
	static std::vector<uint8_t> BuildPacket(const PacketCraftConfig& config);
	static std::vector<uint8_t> GeneratePayload(const PacketCraftConfig& config);

	// Validation
	static bool ValidateConfig(const PacketCraftConfig& config, std::string& errorMsg);
	static bool ValidateIPAddress(const std::string& ip);
	static bool ValidateMACAddress(const std::string& mac);

	// MAC address utilities
	static std::string GetLocalMACAddress();
	static std::string GetGatewayMACAddress(const std::string& gatewayIP);

private:
	static uint16_t CalculateChecksum(const uint16_t* buf, size_t len);
	static uint16_t CalculateTCPChecksum(const std::vector<uint8_t>& packet, size_t ipHeaderOffset, size_t tcpHeaderOffset);
	static uint16_t CalculateUDPChecksum(const std::vector<uint8_t>& packet, size_t ipHeaderOffset, size_t udpHeaderOffset);
};