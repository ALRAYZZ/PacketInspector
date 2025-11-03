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

	bool ShouldClose();


private:
	SDL_Window* window;
	void* glContext;
	bool running;
};