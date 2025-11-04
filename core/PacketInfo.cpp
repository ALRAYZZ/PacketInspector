#include "PacketInfo.h"
#include <iomanip>
#include <sstream>

std::string PacketInfo::GetTimeString() const
{
	auto t = std::chrono::system_clock::to_time_t(timestamp);
	std::tm tmBuf{};
#ifdef _WIN32
	localtime_s(&tmBuf, &t);
#else
	localtime_r(&t, &tmBuf);
#endif

	std::ostringstream oss;
	oss << std::put_time(&tmBuf, "%H:%M:%S");
	return oss.str();
}

std::string PacketInfo::GetHexPreview(size_t maxBytes) const
{
	std::ostringstream oss;
	for (size_t i = 0; i < std::min(maxBytes, data.size()); ++i)
	{
		oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << " ";
	}
	return oss.str();
}