#include <cstdio>
#include <iostream>
#include "windowmanager.h"
#include "scenemanager.h"

int main()
{
	WindowManager* manager = new SceneManager();
	manager->start_window();
	return 0;
}