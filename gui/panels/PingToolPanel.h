#pragma once
#include <memory>
#include <string>

class PingEngine;

class PingToolPanel
{
public:
	explicit PingToolPanel(std::shared_ptr<PingEngine> engine);
	void Render();

private:
	std::shared_ptr<PingEngine> pingEngine;


	// UI state
	char targetAddress[256] = "8.8.8.8";
	int pingCount = 4;
	int timeoutMs = 1000;
	bool continuousPing = false;
	bool autoScroll = true;


	void RenderControlSection();
	void RenderResultsSection();
	void RenderStatisticsSection();
};