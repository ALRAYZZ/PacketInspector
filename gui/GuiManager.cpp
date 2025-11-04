#include "GuiManager.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"


#ifdef _WIN32
#include <SDL_syswm.h>
#include <Windows.h>
#endif


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

	// Enable window to be resizable and allow screensaver
	SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");

	// Create app window
	window = SDL_CreateWindow("PacketInspector",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		1280,
		720,
		SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP);
	if (!window)
	{
		return false;
	}

#ifdef _WIN32
	// Get Windows handle
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	hwnd = wmInfo.info.win.window;

	// Set window styles for transparency with click-through on transparent areas
	HWND hwndWin = (HWND)hwnd;
	SetWindowLong(hwndWin, GWL_EXSTYLE, GetWindowLong(hwndWin, GWL_EXSTYLE) | WS_EX_LAYERED);

	// Make black color transparent
	SetLayeredWindowAttributes(hwndWin, RGB(0, 0, 0), 0, LWA_COLORKEY);
#endif

	// Create OpenGL context and enable vsync
	glContext = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, glContext);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Setup ImGui context, core and apply dark theme
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	// Setup ImGui backend
	ImGui_ImplSDL2_InitForOpenGL(window, glContext);
	ImGui_ImplOpenGL3_Init("#version 330");

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

	// Display ImGui demo window (for testing purposes)
	ImGui::ShowDemoWindow();
}

// Render ImGui draw data and swap buffers
void GuiManager::Render()
{
	// Finalize ImGui draw data
	ImGui::Render();

	// Clear the screen with dark gray color and render ImGui
	glViewport(0, 0, 1280, 720);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Swap front and back buffers to display rendered frame
	SDL_GL_SwapWindow(window);
}

// Shutdown ImGui and SDL subsystems and clean up resources
void GuiManager::Shutdown()
{
	// Clean up ImGui and backends and context
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	// Clean up SDL resources
	if (glContext) SDL_GL_DeleteContext(glContext);
	if (window) SDL_DestroyWindow(window);

	SDL_Quit();
}

// Check if the GUI should close
bool GuiManager::ShouldClose() const
{
	return !running;
}