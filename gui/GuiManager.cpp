#include "GuiManager.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"


GuiManager::GuiManager()
	: window(nullptr), glContext(nullptr), running(true)
{}

GuiManager::~GuiManager()
{
	Shutdown();
}

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

	// Create app window
	window = SDL_CreateWindow("PacketInspector", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!window)
	{
		return false;
	}

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

void GuiManager::Render()
{
	// Finalize ImGui draw data
	ImGui::Render();

	// Clear the screen with dark gray color and render ImGui
	glViewport(0, 0, 1280, 720);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Swap front and back buffers to display rendered frame
	SDL_GL_SwapWindow(window);
}

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

bool GuiManager::ShouldClose() const
{
	return !running;
}