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

}