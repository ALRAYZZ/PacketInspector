#pragma once
#include <memory>
struct SDL_Window;

class GuiManager
{
public:
	GuiManager();
	~GuiManager();

	bool Initialize();
	void NewFrame();
	void Render();
	void Shutdown();

	bool ShouldClose() const;


private:
	enum class AppTab
	{
		PacketInspector,
		PingTool,
		PacketCrafter
	};

	SDL_Window* window;
	void* glContext;
	bool running;
	AppTab activeTab;

	void RenderTabBar(int displayW);
	void RenderPacketInspectorTab();
	void RenderPingToolTab();
	void RenderPacketCrafterTab();


#ifdef _WIN32
	void* hwnd; // Windows-specific window handle
#endif
};