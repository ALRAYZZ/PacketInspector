#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>

struct PingResult
{
	std::chrono::system_clock::time_point timestamp;
	std::string targetIP;
	int sequenceNumber = 0;
	double latencyMs = 0.0;
	int ttl = 0;
	bool success = false;
	std::string errorMessage;

	std::string GetTimeString() const;
	std::string GetStatusString() const;
};

struct PingStatistics
{
	int packetsSent = 0;
	int packetsReceived = 0;
	double minLatencyMs = 0.0;
	double maxLatencyMs = 0.0;
	double avgLatencyMs = 0.0;
	double packetLossPercent = 0.0;

	void Reset();
	void Update(const std::vector<PingResult>& results);
};

class PingEngine
{
public:
	PingEngine();
	~PingEngine();

	// Start ping with count
	bool StartPing(const std::string& target, int count = 4, int timeoutMs = 1000);
	void StopPing();
	bool IsPinging() const;

	// Get results
	std::vector<PingResult> GetResults();
	PingStatistics GetStatistics() const;
	void ClearResults();

	// Validate IP
	static bool ValidateTarget(const std::string& target);

private:
	std::atomic<bool> pinging;
	std::atomic<bool> shouldStop;
	std::thread pingThread;
	std::vector<PingResult> results;
	mutable std::mutex resultMutex;
	PingStatistics stats;

	void PingThreadFunc(const std::string& target, int count, int timeoutMs);
	bool SendICMPEcho(const std::string& target, int sequence, int timeoutMs, PingResult& result);
	bool ResolveHostname(const std::string& target, std::string& resolvedIP);
};