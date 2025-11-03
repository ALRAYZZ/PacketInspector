#include "gui/GuiManager.h"

int main(int, char**)
{
	GuiManager gui;

	if (!gui.Initialize())
	{
		return -1;
	}

	while (!gui.ShouldClose())
	{
		gui.NewFrame();
		gui.Render();
	}

	gui.Shutdown();
	return 0;
}