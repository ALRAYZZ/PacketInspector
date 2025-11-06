#include "PingEngine.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#endif

std::string PingResult::GetTimeString() const
{
	auto time = std::chrono::system_clock::to_time_t(timestamp);
	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &time);
#else
	localtime_r(&time, &tm);
#endif
	std::ostringstream oss;
	oss << std::put_time(&tm, "%H:%M:%S");
	return oss.str();
}

std::string PingResult::GetStatusString() const
{
	if (success)
	{
		std::ostringstream oss;
		oss << "Reply from " << targetIP << ": bytes=32 time=" << std::fixed << std::setprecision(1) << latencyMs << "ms TTL=" << ttl;
		return oss.str();
	}
	else
	{
		return "Request timed out.";
	}
}

void PingStatistics::Reset()
{
	packetsSent = 0;
	packetsReceived = 0;
	minLatencyMs = 0.0;
	maxLatencyMs = 0.0;
	avgLatencyMs = 0.0;
	packetLossPercent = 0.0;
}

void PingStatistics::Update(const std::vector<PingResult>& results)
{
	if (results.empty())
	{
		Reset();
		return;
	}

	packetsSent = static_cast<int>(results.size());
	packetsReceived = 0;
	double totalLatency = 0.0;
	minLatencyMs = 999999.0;
	maxLatencyMs = 0.0;

	for (const auto& result : results)
	{
		if (result.success)
		{
			packetsReceived++;
			totalLatency += result.latencyMs;
			minLatencyMs = std::min(minLatencyMs, result.latencyMs);
			maxLatencyMs = std::max(maxLatencyMs, result.latencyMs);
		}
	}

	if (packetsReceived > 0)
	{
		avgLatencyMs = totalLatency / packetsReceived;
	}
	else
	{
		minLatencyMs = 0.0;
		avgLatencyMs = 0.0;
	}

	if (packetsSent > 0)
	{
		packetLossPercent = ((packetsSent - packetsReceived) * 100.0) / packetsSent;
	}
}

PingEngine::PingEngine() : pinging(false), shouldStop(false)
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

PingEngine::~PingEngine()
{
	StopPing();
#ifdef _WIN32
	WSACleanup();
#endif
}

bool PingEngine::StartPing(const std::string& target, int count, int timeoutMs)
{
	if (pinging.load())
	{
		return false;
	}

	if (!ValidateTarget(target))
	{
		return false;
	}

	// Clear previous results
	{
		std::lock_guard<std::mutex> lock(resultMutex);
		results.clear();
		stats.Reset();
	}

	shouldStop.store(false);
	pinging.store(true);
	pingThread = std::thread(&PingEngine::PingThreadFunc, this, target, count, timeoutMs);

	return true;
}

void PingEngine::StopPing()
{
	shouldStop.store(true);
	if (pingThread.joinable())
	{
		pingThread.join();
	}
	pinging.store(false);
}

bool PingEngine::IsPinging() const
{
	return pinging.load();
}

std::vector<PingResult> PingEngine::GetResults()
{
	std::lock_guard<std::mutex> lock(resultMutex);
	return results;
}

PingStatistics PingEngine::GetStatistics() const
{
	std::lock_guard<std::mutex> lock(resultMutex);
	return stats;
}

void PingEngine::ClearResults()
{
	std::lock_guard<std::mutex> lock(resultMutex);
	results.clear();
	stats.Reset();
}

bool PingEngine::ValidateTarget(const std::string& target)
{
	if (target.empty())
	{
		return false;
	}

	// Try to resolve hostname or IP
	std::string resolved;
	PingEngine engine;
	return engine.ResolveHostname(target, resolved);
}

void PingEngine::PingThreadFunc(const std::string& target, int count, int timeoutMs)
{
	std::string resolvedIP;
	if (!ResolveHostname(target, resolvedIP))
	{
		// Failed to resolve
		PingResult failedResult;
		failedResult.timestamp = std::chrono::system_clock::now();
		failedResult.targetIP = target;
		failedResult.success = false;
		failedResult.errorMessage = "Unable to resolve hostname.";

		std::lock_guard<std::mutex> lock(resultMutex);
		results.push_back(failedResult);
		stats.Update(results);
		pinging.store(false);
		return;
	}

	int sequence = 1;
	while (!shouldStop.load())
	{
		PingResult result;
		result.sequenceNumber = sequence;
		result.targetIP = resolvedIP;

		bool success = SendICMPEcho(resolvedIP, sequence, timeoutMs, result);

		{
			std::lock_guard<std::mutex> lock(resultMutex);
			results.push_back(result);
			stats.Update(results);
		}
		sequence++;

		// Check if we reached the count
		if (count > 0 && sequence > count)
		{
			break;
		}

		// Wait 1 second between pings
		for (int i = 0; i < 10 && !shouldStop.load(); ++i)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	pinging.store(false);
}

bool PingEngine::ResolveHostname(const std::string& target, std::string& resolvedIP)
{
	struct addrinfo hints {};
	struct addrinfo* result = nullptr;

	hints.ai_family = AF_INET; // Allow IPv4
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(target.c_str(), nullptr, &hints, &result) != 0)
	{
		return false;
	}

	if (result != nullptr)
	{
		struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
		resolvedIP = ip;
		freeaddrinfo(result);
		return true;
	}

	freeaddrinfo(result);
	return false;
}

#ifdef _WIN32
bool PingEngine::SendICMPEcho(const std::string& target, int sequence, int timeoutMs, PingResult& result)
{
	result.timestamp = std::chrono::system_clock::now();

	// Create ICMP handle
	HANDLE hIcmpFile = IcmpCreateFile();
	if (hIcmpFile == INVALID_HANDLE_VALUE)
	{
		result.success = false;
		result.errorMessage = "Failed to create ICMP handle";
		return false;
	}

	unsigned long ipaddr = inet_addr(target.c_str());
	if (ipaddr == INADDR_NONE)
	{
		IcmpCloseHandle(hIcmpFile);
		result.success = false;
		result.errorMessage = "Invalid IP address";
		return false;
	}

	char sendData[32] = "PingTool Ping ALRAYZZ";
	DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
	void* replyBuffer = malloc(replySize);

	if (!replyBuffer)
	{
		IcmpCloseHandle(hIcmpFile);
		result.success = false;
		result.errorMessage = "Memory allocation failed";
		return false;
	}

	auto startTime = std::chrono::high_resolution_clock::now();
	
	DWORD dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, sendData, sizeof(sendData), nullptr, replyBuffer, replySize, timeoutMs);

	auto endTime = std::chrono::high_resolution_clock::now();
	double latency = std::chrono::duration<double, std::milli>(endTime - startTime).count();

	if (dwRetVal != 0)
	{
		PICMP_ECHO_REPLY pEchoReply = static_cast<PICMP_ECHO_REPLY>(replyBuffer);
		result.success = true;
		result.latencyMs = latency;
		result.ttl = pEchoReply->Options.Ttl;
	}
	else
	{
		result.success = false;
		result.errorMessage = "Request timed out.";
	}

	free(replyBuffer);
	IcmpCloseHandle(hIcmpFile);

	return result.success;
}
#else
// Linux/Unix implementation using raw sockets
bool PingEngine::SendICMPEcho(const std::string& target, int sequence, int timeoutMs, PingResult& result)
{
	result.timestamp = std::chrono::system_clock::now();

	int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sock < 0)
	{
		result.success = false;
		result.errorMessage = "Failed to create socket (requires root/admin)";
		return false;
	}

	// Set timeout
	struct timeval tv;
	tv.tv_sec = timeoutMs / 1000;
	tv.tv_usec = (timeoutMs % 1000) * 1000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(target.c_str());

	// Build ICMP packet
	char packet[64];
	memset(packet, 0, sizeof(packet));
	struct icmp* icmpHdr = reinterpret_cast<struct icmp*>(packet);
	icmpHdr->icmp_type = ICMP_ECHO;
	icmpHdr->icmp_code = 0;
	icmpHdr->icmp_id = getpid();
	icmpHdr->icmp_seq = sequence;

	// Calculate checksum
	icmpHdr->icmp_cksum = 0;
	unsigned short* buf = reinterpret_cast<unsigned short*>(packet);
	unsigned int sum = 0;
	for (int i = 0; i < 32; i++)
	{
		sum += buf[i];
	}
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	icmpHdr->icmp_cksum = ~sum;

	auto startTime = std::chrono::high_resolution_clock::now();

	// Send packet
	if (sendto(sock, packet, sizeof(packet), 0, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) <= 0)
	{
		close(sock);
		result.success = false;
		result.errorMessage = "Failed to send packet";
		return false;
	}

	// Receive reply
	char recvBuf[1024];
	socklen_t addrLen = sizeof(addr);
	int bytesReceived = recvfrom(sock, recvBuf, sizeof(recvBuf), 0,
		reinterpret_cast<struct sockaddr*>(&addr), &addrLen);

	auto endTime = std::chrono::high_resolution_clock::now();
	double latency = std::chrono::duration<double, std::milli>(endTime - startTime).count();

	close(sock);

	if (bytesReceived > 0)
	{
		struct ip* ipHdr = reinterpret_cast<struct ip*>(recvBuf);
		result.success = true;
		result.latencyMs = latency;
		result.ttl = ipHdr->ip_ttl;
	}
	else
	{
		result.success = false;
		result.errorMessage = "Request timed out";
	}

	return result.success;
}
#endif

