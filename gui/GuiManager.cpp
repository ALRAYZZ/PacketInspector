#include "GuiManager.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "core/CaptureEngine.h"
#include "panels/CaptureControlPanel.h"
#include "panels/PacketListPanel.h"
#include "panels/PacketDetailPanel.h"
#include "core/PingEngine.h"
#include "panels/PingToolPanel.h"
#include "core/PacketCrafterEngine.h"
#include "panels/PacketCrafterPanel.h"


#ifdef _WIN32
#include <SDL_syswm.h>
#include <Windows.h>
#endif


// The difference between shared and unique ptr here is that the CaptureEngine is shared between multiple panels
// Unique is only when one owner exists, its the recommended default, but shared is needed for shared ownership
std::shared_ptr<CaptureEngine> captureEngine;
std::shared_ptr<PingEngine> pingEngine;
std::shared_ptr<PacketCrafterEngine> packetCrafterEngine;

std::unique_ptr<CaptureControlPanel> capturePanel;
std::unique_ptr<PacketListPanel> packetListPanel;
std::unique_ptr<PacketDetailPanel> packetDetailPanel;
std::unique_ptr<PingToolPanel> pingToolPanel;
std::unique_ptr<PacketCrafterPanel> packetCrafterPanel;

GuiManager::GuiManager()
	: window(nullptr), glContext(nullptr), running(true), activeTab(AppTab::PacketInspector)
#ifdef _WIN32
	, hwnd(nullptr)
#endif
{}

GuiManager::~GuiManager()
{
	Shutdown();
}

// Initialize SDL, create window and OpenGL context, and setup ImGui
bool GuiManager::Initialize()
{
	// Initialize SDL subsystems (video, timer, game controller)
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		return false;
	}

	// Create window with OpenGL context
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	// Get screen dimensions for centering
	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(0, &displayMode);
	int screenWidth = displayMode.w;
	int screenHeight = displayMode.h;

	// Create larger centered window (80% of screen size)
	int windowWidth = static_cast<int>(screenWidth * 0.8f);
	int windowHeight = static_cast<int>(screenHeight * 0.8f);
	int xPos = (screenWidth - windowWidth) / 2;
	int yPos = (screenHeight - windowHeight) / 2;

	window = SDL_CreateWindow("PacketInspector",
		xPos,
		yPos,
		windowWidth,
		windowHeight,
		SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI);
	if (!window)
	{
		return false;
	}

	// Create OpenGL context and enable vsync
	glContext = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, glContext);
	SDL_GL_SetSwapInterval(1); // Enable vsync

#ifdef _WIN32
	// Get Windows handle
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	hwnd = wmInfo.info.win.window;

	HWND hwndWin = (HWND)hwnd;
	
	// Remove all window styles to hide the window frame completely
	LONG style = GetWindowLong(hwndWin, GWL_STYLE);
	SetWindowLong(hwndWin, GWL_STYLE, style & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU));
	
	// Set layered window for transparency (removed WS_EX_TOPMOST)
	LONG exStyle = GetWindowLong(hwndWin, GWL_EXSTYLE);
	SetWindowLong(hwndWin, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
	
	// Make black (RGB 0,0,0) transparent using color key
	SetLayeredWindowAttributes(hwndWin, RGB(0, 0, 0), 0, LWA_COLORKEY);
	
	// Update window position and size (changed HWND_TOPMOST to HWND_NOTOPMOST)
	SetWindowPos(hwndWin, HWND_NOTOPMOST, xPos, yPos, windowWidth, windowHeight, 
		SWP_FRAMECHANGED | SWP_SHOWWINDOW);
#endif

	// Setup ImGui context, core and apply dark theme
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	// Setup ImGui backend
	ImGui_ImplSDL2_InitForOpenGL(window, glContext);
	ImGui_ImplOpenGL3_Init("#version 330");

	captureEngine = std::make_shared<CaptureEngine>();
	capturePanel = std::make_unique<CaptureControlPanel>(captureEngine);
	packetListPanel = std::make_unique<PacketListPanel>(captureEngine);
	// Lambda passes the selected packet from PacketListPanel to PacketDetailPanel
	packetDetailPanel = std::make_unique<PacketDetailPanel>([this]() -> std::optional<PacketInfo>
		{
			return packetListPanel ? packetListPanel->GetSelectedPacket() : std::optional<PacketInfo>{};
		});

	pingEngine = std::make_shared<PingEngine>();
	pingToolPanel = std::make_unique<PingToolPanel>(pingEngine);

	packetCrafterEngine = std::make_shared<PacketCrafterEngine>();
	packetCrafterPanel = std::make_unique<PacketCrafterPanel>(packetCrafterEngine);

	return true;
}

// Start a new ImGui frame and process SDL events
void GuiManager::NewFrame()
{
	// Process all SDL events (input, window events, etc.)
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_QUIT)
		{
			running = false;
		}
	}
	// Start new ImGui frame for rendering UI
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	// Get SDL window size
	int displayW, displayH;
	SDL_GetWindowSize(window, &displayW, &displayH);

	// Create a fullscreen ImGui window that matches SDL window size with custom styling
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2((float)displayW, (float)displayH), ImGuiCond_Always);
	
	// Custom window styling for minimal design
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15.0f, 15.0f));
	
	// Begin main window with no title bar, resize, move, or collapse
	ImGui::Begin("Packet Analyzer", nullptr, 
		ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus);

	// Render the tab bar
	RenderTabBar(displayW);

	// Visual separator with spacing for better hierarchy
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Spacing();

	// Render content based on active tab
	switch (activeTab)
	{
		case AppTab::PacketInspector:
			RenderPacketInspectorTab();
			break;
		case AppTab::PingTool:
			RenderPingToolTab();
			break;
		case AppTab::PacketCrafter:
			RenderPacketCrafterTab();
			break;
	}

	ImGui::End();

	ImGui::PopStyleVar(3);
}

// Render the tab bar with application tabs
void GuiManager::RenderTabBar(int displayW)
{
	// Padding
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.0f, 10.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

	// Get button colors
	ImVec4 buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
	ImVec4 buttonHoveredColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
	ImVec4 buttonActiveColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
	
	// UI colors for active/inactive tabs
	ImVec4 activeTabColor = ImVec4(buttonActiveColor.x * 1.3f, buttonActiveColor.y * 1.3f, buttonActiveColor.z * 1.3f, 1.0f);
	ImVec4 inactiveTabTextColor = ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
	ImVec4 activeTabTextColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	ImVec4 inactiveHoverColor = ImVec4(0.22f, 0.22f, 0.22f, 0.7f);

	// Packet Inspector Tab
	if (activeTab == AppTab::PacketInspector)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, activeTabColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, activeTabColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeTabColor);
		ImGui::PushStyleColor(ImGuiCol_Text, activeTabTextColor);
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.12f, 0.3f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, inactiveHoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonActiveColor);
		ImGui::PushStyleColor(ImGuiCol_Text, inactiveTabTextColor);
	}

	if (ImGui::Button("Packet Inspector"))
	{
		activeTab = AppTab::PacketInspector;
	}
	ImGui::PopStyleColor(4);

	ImGui::SameLine();

	// Ping Tool Tab
	if (activeTab == AppTab::PingTool)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, activeTabColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, activeTabColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeTabColor);
		ImGui::PushStyleColor(ImGuiCol_Text, activeTabTextColor);
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.12f, 0.3f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, inactiveHoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonActiveColor);
		ImGui::PushStyleColor(ImGuiCol_Text, inactiveTabTextColor);
	}

	if (ImGui::Button("Ping Tool"))
	{
		activeTab = AppTab::PingTool;
	}
	ImGui::PopStyleColor(4);

	ImGui::SameLine();

	// Packet Crafter Tab
	if (activeTab == AppTab::PacketCrafter)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, activeTabColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, activeTabColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeTabColor);
		ImGui::PushStyleColor(ImGuiCol_Text, activeTabTextColor);
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.12f, 0.3f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, inactiveHoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonActiveColor);
		ImGui::PushStyleColor(ImGuiCol_Text, inactiveTabTextColor);
	}
	
	if (ImGui::Button("Packet Crafter"))
	{
		activeTab = AppTab::PacketCrafter;
	}
	ImGui::PopStyleColor(4);

	// Exit button on the far right with normal styling
	ImGui::SameLine(displayW - 95.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));
	if (ImGui::Button("Exit", ImVec2(60, 0)))
	{
		running = false;
	}
	ImGui::PopStyleVar();

	ImGui::PopStyleVar(3);
}

// Render Packet Inspector tab content
void GuiManager::RenderPacketInspectorTab()
{
	capturePanel->Render();
	ImGui::Separator();

	ImVec2 availableRegion = ImGui::GetContentRegionAvail();

	// Define split ratio
	float leftPanelWidth = availableRegion.x * 0.5f;
	float rightPanelWidth = availableRegion.x * 0.5f - ImGui::GetStyle().ItemSpacing.x;

	// Left Panel - Packet List
	ImGui::BeginChild("PacketListRegion", ImVec2(leftPanelWidth, availableRegion.y), true);
	packetListPanel->Render();
	ImGui::EndChild();

	ImGui::SameLine();

	// Right Panel - Flow Details (TOP) and Packet Details (BOTTOM)
	ImGui::BeginChild("DetailsRegion", ImVec2(rightPanelWidth, availableRegion.y), true);
	
	if (!packetListPanel->selectedFlowKey.empty())
	{
		// Flow details in top half
		ImGui::BeginChild("FlowDetailsRegion", ImVec2(0, availableRegion.y * 0.5f - 5), true);
		packetListPanel->RenderDetailView();
		ImGui::EndChild();

		// Packet details in bottom half
		ImGui::BeginChild("PacketDetailsRegion", ImVec2(0, 0), true);
		packetDetailPanel->Render();
		ImGui::EndChild();
	}
	else
	{
		ImGui::TextWrapped("Select a flow from the packet list to view its details here.");
	}

	ImGui::EndChild();
}

// Render Ping Tool tab content
void GuiManager::RenderPingToolTab()
{
	if (pingToolPanel)
	{
		pingToolPanel->Render();
	}
}

// Render Packet Crafter tab content
void GuiManager::RenderPacketCrafterTab()
{
	if (packetCrafterPanel)
	{
		packetCrafterPanel->Render();
	}
}

// Render ImGui draw data and swap buffers
void GuiManager::Render()
{
	// Finalize ImGui draw data
	ImGui::Render();

	// Get current window size
	int displayW, displayH;
	SDL_GetWindowSize(window, &displayW, &displayH);

	// Clear the screen with black color (transparent via color key) and render ImGui
	glViewport(0, 0, displayW, displayH);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Swap front and back buffers to display rendered frame
	SDL_GL_SwapWindow(window);
}

// Shutdown ImGui and SDL subsystems and clean up resources
void GuiManager::Shutdown()
{
	// Clean up panels and capture engine
	packetListPanel.reset();
	capturePanel.reset();
	captureEngine.reset();
	pingToolPanel.reset();
	pingEngine.reset();
	packetDetailPanel.reset();
	packetCrafterPanel.reset();
	packetCrafterEngine.reset();

	// Clean up ImGui and backends and context
	if (ImGui::GetCurrentContext())
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}

	// Clean up SDL resources
	if (glContext)
	{
		SDL_GL_DeleteContext(glContext);
		glContext = nullptr;
	}
	if (window)
	{
		SDL_DestroyWindow(window);
		window = nullptr;
	}

	SDL_Quit();
}

// Check if the GUI should close
bool GuiManager::ShouldClose() const
{
	return !running;
}