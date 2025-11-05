#include "GuiManager.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "core/CaptureEngine.h"
#include "panels/CaptureControlPanel.h"
#include "panels/PacketListPanel.h"


#ifdef _WIN32
#include <SDL_syswm.h>
#include <Windows.h>
#endif


// The difference between shared and unique ptr here is that the CaptureEngine is shared between multiple panels
// Unique is only when one owner exists, its the recommended default, but shared is needed for shared ownership
std::shared_ptr<CaptureEngine> captureEngine;
std::unique_ptr<CaptureControlPanel> capturePanel;
std::unique_ptr<PacketListPanel> packetListPanel;

GuiManager::GuiManager()
	: window(nullptr), glContext(nullptr), running(true)
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

	// Custom title bar area with button color
	ImVec4 buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
	ImGui::TextColored(buttonColor, "PacketInspector");
	ImGui::PopFont();
	
	ImGui::SameLine(displayW - 80.0f);
	if (ImGui::Button("Exit", ImVec2(60, 0)))
	{
		running = false;
	}

	ImGui::Separator();

	// UI Content
	capturePanel->Render();
	ImGui::Separator();
	packetListPanel->Render();
	
	ImGui::End();
	
	ImGui::PopStyleVar(3);

	// Optionally show demo window for reference (remove this in production)
	// ImGui::ShowDemoWindow();
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