#include "PacketParser.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif
#include <sstream>
#include <cstring>


#pragma pack(push, 1)
struct EthernetHeader
{
	uint8_t dst[6];
	uint8_t src[6];
	uint16_t type;
};

struct IPv4Header
{
	uint8_t ihl_version;
	uint8_t tos;
	uint16_t tot_len;
	uint16_t id;
	uint16_t frag_off;
	uint8_t ttl;
	uint8_t protocol;
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
#pragma pack(pop)

static std::string macToStr(const uint8_t* mac)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return buf;
}

static bool IsLocalAddress(const std::string& ip)
{
	// Check for common private IP ranges
	if (ip.rfind("10.", 0) == 0) return true;
	if (ip.rfind("192.168.", 0) == 0) return true;
	if (ip.rfind("127.", 0) == 0) return true;
	if (ip.rfind("172.", 0) == 0)
	{
		// Check if its in 172.16.0.0 to 172.31.255.255 range
		size_t dot = ip.find('.', 4);
		if (dot != std::string::npos)
		{
			int second = std::stoi(ip.substr(4, dot - 4));
			if (second >= 16 && second <= 31) return true;
		}
	}
	return false;
}

void PacketParser::ParsePacket(PacketInfo& pkt)
{
	if (pkt.data.size() < sizeof(EthernetHeader)) return;

	const auto* eth = reinterpret_cast<const EthernetHeader*>(pkt.data.data());
	uint16_t etherType = ntohs(eth->type);

	// If not IPv4, return (for now we only handle IPv4)
	if (etherType != 0x0800) return;

	const size_t ipOffset = sizeof(EthernetHeader);
	if (pkt.data.size() < ipOffset + sizeof(IPv4Header)) return;

	const auto* ip = reinterpret_cast<const IPv4Header*>(pkt.data.data() + ipOffset);
	const uint8_t ipHeaderLen = (ip->ihl_version & 0x0F) * 4;
	const uint8_t protocol = ip->protocol;

	char src[INET_ADDRSTRLEN];
	char dst[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &ip->saddr, src, sizeof(src));
	inet_ntop(AF_INET, &ip->daddr, dst, sizeof(dst));

	pkt.srcAddr = src;
	pkt.dstAddr = dst;
	pkt.protocol = "IP";

	size_t transportOffset = ipOffset + ipHeaderLen;
	if (pkt.data.size() < transportOffset + 4) return;

	switch (protocol)
	{
		case 6: // TCP
		{
			pkt.protocol = "TCP";
			if (pkt.data.size() < transportOffset + sizeof(TCPHeader)) return;
			const auto* tcp = reinterpret_cast<const TCPHeader*>(pkt.data.data() + transportOffset);
			pkt.srcPort = ntohs(tcp->src_port);
			pkt.dstPort = ntohs(tcp->dst_port);
			break;
		}
		case 17: // UDP
		{
			pkt.protocol = "UDP";
			if (pkt.data.size() < transportOffset + sizeof(UDPHeader)) return;
			const auto* udp = reinterpret_cast<const UDPHeader*>(pkt.data.data() + transportOffset);
			pkt.srcPort = ntohs(udp->src_port);
			pkt.dstPort = ntohs(udp->dst_port);
			break;
		}
		default:
			pkt.protocol = "Other";
			break;
	}

	// Determine packet direction: incoming if source IP is not local
	bool srcIsLocal = IsLocalAddress(pkt.srcAddr);
	bool dstIsLocal = IsLocalAddress(pkt.dstAddr);

	// If source is external and destination local -> incoming
	// If source is local and destination external -> outgoing

	pkt.incoming = !srcIsLocal && dstIsLocal;
}

