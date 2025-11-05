#pragma once
#include "PacketInfo.h"
#include <cstdint>

class PacketParser
{
public:
	static void ParsePacket(PacketInfo& pkt);
};